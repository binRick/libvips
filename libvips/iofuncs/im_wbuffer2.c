/* Double-buffered write.
 * 
 * 19/3/10
 * 	- from im_wbuffer.c
 * 	- move on top of VipsThreadpool, instead of im_threadgroup_t
 */

/*

    This file is part of VIPS.
    
    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /*HAVE_UNISTD_H*/

#include <vips/vips.h>
#include <vips/internal.h>
#include <vips/thread.h>
#include <vips/threadpool.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif /*WITH_DMALLOC*/

/* A buffer we are going to write to disc in a background thread.
 */
typedef struct _WriteBuffer {
	struct _Write *write;

	REGION *region;		/* Pixels */
	Rect area;		/* Part of image this region covers */
        im_semaphore_t go; 	/* Start bg thread loop */
        im_semaphore_t nwrite; 	/* Number of threads writing to region */
        im_semaphore_t done; 	/* Bg thread has done write */
        int write_errno;	/* Save write errors here */
	GThread *thread;	/* BG writer thread */
	gboolean kill;		/* Set to ask thread to exit */
} WriteBuffer;

/* Per-call state.
 */
typedef struct _Write {
	VipsImage *im;

	/* We are current writing tiles to buf, buf_back is in the hands of
	 * the bg write thread.
	 */
	WriteBuffer *buf;
	WriteBuffer *buf_back;

	/* The position we're at in buf.
	 */
	int x;
	int y;

	/* The tilesize we've picked.
	 */
	int tile_width;
	int tile_height;
	int nlines;

	/* The file format write operation.
	 */
	im_wbuffer_fn write_fn;	
	void *a;		
	void *b;
} Write;

/* Enable im_wbuffer2 ... set from the cmd line, tested by our users.
 */
int im__wbuffer2 = 0;

static void
wbuffer_free( WriteBuffer *wbuffer )
{
        /* Is there a thread running this region? Kill it!
         */
        if( wbuffer->thread ) {
                wbuffer->kill = TRUE;
		im_semaphore_up( &wbuffer->go );

		/* Return value is always NULL (see wbuffer_write_thread).
		 */
		(void) g_thread_join( wbuffer->thread );
#ifdef DEBUG
		printf( "wbuffer_free: g_thread_join()\n" );
#endif /*DEBUG*/

		wbuffer->thread = NULL;
        }

	IM_FREEF( im_region_free, wbuffer->region );
	im_semaphore_destroy( &wbuffer->go );
	im_semaphore_destroy( &wbuffer->nwrite );
	im_semaphore_destroy( &wbuffer->done );
	im_free( wbuffer );
}

static void
wbuffer_write( WriteBuffer *wbuffer )
{
	Write *write = wbuffer->write;

#ifdef DEBUG
	static int n = 1;

	printf( "wbuffer_write: %d, %d bytes from wbuffer %p\n", 
		n++, wbuffer->region->bpl * wbuffer->area.height, wbuffer );
#endif /*DEBUG*/

	wbuffer->write_errno = write->write_fn( wbuffer->region, 
		&wbuffer->area, write->a, write->b );

}

#ifdef HAVE_THREADS
/* Run this as a thread to do a BG write.
 */
static void *
wbuffer_write_thread( void *data )
{
	WriteBuffer *wbuffer = (WriteBuffer *) data;

	for(;;) {
		/* Wait to be told to write.
		 */
		im_semaphore_down( &wbuffer->go );

		if( wbuffer->kill )
			break;

		/* Now block until the last worker finishes on this buffer.
		 */
		im_semaphore_downn( &wbuffer->nwrite, 0 );

		wbuffer_write( wbuffer );

		/* Signal write complete.
		 */
		im_semaphore_up( &wbuffer->done );
	}

	return( NULL );
}
#endif /*HAVE_THREADS*/

static WriteBuffer *
wbuffer_new( Write *write )
{
	WriteBuffer *wbuffer;

	if( !(wbuffer = IM_NEW( NULL, WriteBuffer )) )
		return( NULL );
	wbuffer->write = write;
	wbuffer->region = NULL;
	im_semaphore_init( &wbuffer->go, 0, "go" );
	im_semaphore_init( &wbuffer->nwrite, 0, "nwrite" );
	im_semaphore_init( &wbuffer->done, 0, "done" );
	wbuffer->write_errno = 0;
	wbuffer->thread = NULL;
	wbuffer->kill = FALSE;

	if( !(wbuffer->region = im_region_create( write->im )) ) {
		wbuffer_free( wbuffer );
		return( NULL );
	}

	/* The worker threads need to be able to move the buffers around.
	 */
	im__region_no_ownership( wbuffer->region );

#ifdef HAVE_THREADS
	/* Make this last (picks up parts of wbuffer on startup).
	 */
	if( !(wbuffer->thread = g_thread_create( wbuffer_write_thread, wbuffer, 
		TRUE, NULL )) ) {
		im_error( "wbuffer_new", "%s", _( "unable to create thread" ) );
		wbuffer_free( wbuffer );
		return( NULL );
	}
#endif /*HAVE_THREADS*/

	return( wbuffer );
}

/* Write and swap buffers.
 */
static int
wbuffer_flush( Write *write )
{
	WriteBuffer *t;

#ifdef DEBUG
	static int n = 1;

	printf( "wbuffer_flush: %d\n", n++ );
#endif /*DEBUG*/

	/* Block until the other buffer has been written. We have to do this 
	 * before we can set this buffer writing or we'll lose output ordering.
	 */
	if( write->buf->area.top > 0 ) {
		im_semaphore_down( &write->buf_back->done );

		/* Previous write suceeded?
		 */
		if( write->buf_back->write_errno ) {
			im_error_system( write->buf_back->write_errno,
				"wbuffer_write", "%s", _( "write failed" ) );
			return( -1 ); 
		}
	}

	/* Set the background writer going for this buffer.
	 */
#ifdef HAVE_THREADS
	im_semaphore_up( &write->buf->go );
#else
	/* No threads? Write ourselves synchronously.
	 */
	wbuffer_write( write->buf );
#endif /*HAVE_THREADS*/

	/* Swap buffers.
	 */
	t = write->buf; 
	write->buf = write->buf_back; 
	write->buf_back = t;

	return( 0 );
}

/* Move a wbuffer to a position.
 */
static int 
wbuffer_position( WriteBuffer *wbuffer, int top, int height )
{
	Rect image, area;
	int result;

	image.left = 0;
	image.top = 0;
	image.width = wbuffer->write->im->Xsize;
	image.height = wbuffer->write->im->Ysize;

	area.left = 0;
	area.top = top;
	area.width = wbuffer->write->im->Xsize;
	area.height = height;

	im_rect_intersectrect( &area, &image, &wbuffer->area );

	/* The workers take turns to move the buffers.
	 */
	im__region_take_ownership( wbuffer->region );

	result = im_region_buffer( wbuffer->region, &wbuffer->area );

	im__region_no_ownership( wbuffer->region );

	/* This should be an exclusive buffer, hopefully.
	 */
	g_assert( !wbuffer->region->buffer->done );

	return( result );
}

/* Our VipsThreadpoolAllocate function ... move the thread to the next tile
 * that needs doing. If no buffer is available (the bg writer hasn't yet
 * finished with it), we block. If all tiles are done, we return FALSE to end
 * iteration.
 */
static gboolean
wbuffer_allocate_fn( VipsThread *thr, 
	void *a, void *b, void *c, gboolean *stop )
{
	Write *write = (Write *) a;

	Rect image;
	Rect tile;

#ifdef DEBUG
	printf( "wbuffer_allocate_fn\n" );
#endif /*DEBUG*/

	/* Is the state x/y OK? New line or maybe new buffer or maybe even 
	 * all done.
	 */
	if( write->x >= write->buf->area.width ) {
		write->x = 0;
		write->y += write->tile_height;

		if( write->y >= IM_RECT_BOTTOM( &write->buf->area ) ) {
			/* Write and swap buffers.
			 */
			if( wbuffer_flush( write ) )
				return( -1 );

			/* End of image?
			 */
			if( write->y >= write->im->Ysize ) {
				*stop = TRUE;
				return( 0 );
			}

			/* Position buf at the new y.
			 */
			if( wbuffer_position( write->buf, 
				write->y, write->nlines ) )
				return( -1 );
		}
	}

	/* x, y and buf are good: save params for thread.
	 */
	image.left = 0;
	image.top = 0;
	image.width = write->im->Xsize;
	image.height = write->im->Ysize;
	tile.left = write->x;
	tile.top = write->y;
	tile.width = write->tile_width;
	tile.height = write->tile_height;
	im_rect_intersectrect( &image, &tile, &thr->pos );
	thr->a = write->buf;

	/* Add to the number of writers on the buffer.
	 */
	im_semaphore_upn( &write->buf->nwrite, -1 );

	/* Move state on.
	 */
	write->x += write->tile_width;

	return( 0 );
}

/* Our VipsThreadpoolWork function ... generate a tile!
 */
static int
wbuffer_work_fn( VipsThread *thr, 
	REGION *reg, void *a, void *b, void *c )
{
	WriteBuffer *wbuffer = (WriteBuffer *) thr->a;

#ifdef DEBUG
	printf( "wbuffer_work_fn\n" );
#endif /*DEBUG*/

	if( im_prepare_to( reg, wbuffer->region, 
		&thr->pos, thr->pos.left, thr->pos.top ) )
		return( -1 );

	/* Tell the bg write thread we've left.
	 */
	im_semaphore_upn( &wbuffer->nwrite, 1 );

	return( 0 );
}

int
im_wbuffer2( VipsImage *im, im_wbuffer_fn write_fn, void *a, void *b )
{
	Write write;
	int result;

	write.im = im;
	write.buf = wbuffer_new( &write );
	write.buf_back = wbuffer_new( &write );
	write.x = 0;
	write.y = 0;
	write.write_fn = write_fn;
	write.a = a;
	write.b = b;

	vips_get_tile_size( im, 
		&write.tile_width, &write.tile_height, &write.nlines );

	if( im__start_eval( im ) )
		return( -1 );

	result = 0;

	if( !write.buf || 
		!write.buf_back || 
		wbuffer_position( write.buf, 0, write.nlines ) ||
		vips_threadpool_run( im, 
			wbuffer_allocate_fn, wbuffer_work_fn, 
			&write, NULL, NULL ) )  
		result = -1;

	/* We've set all the buffers writing, but not waited for the BG
	 * writer to finish. This can take a while: it has to wait for the
	 * last worker to make the last tile.
	 *
	 * We can't just free the buffers (which will wait for the bg threads 
	 * to finish), since the bg thread might see the kill before it gets a 
	 * chance to write.
	 */
	if( write.buf->area.top > 0 )
		im_semaphore_down( &write.buf_back->done );

	im__end_eval( im );

	/* Free buffers ... this will wait for the final write to finish too.
	 */
	wbuffer_free( write.buf );
	wbuffer_free( write.buf_back );

	return( result );
}

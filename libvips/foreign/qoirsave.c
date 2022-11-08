/*
#define DEBUG_VERBOSE
#define DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>
#include <vips/internal.h>

#include "pforeign.h"

#define QOIR_IMPLEMENTATION
#include "qoir/qoir.h"
char *vips__save_qoir_suffs[] = { ".qoir", NULL };

typedef struct _VipsForeignSaveQoir VipsForeignSaveQoir;

struct _VipsForeignSaveQoir {
	VipsForeignSave parent_object;

	VipsTarget *target;
};

typedef VipsForeignSaveClass VipsForeignSaveQoirClass;

G_DEFINE_ABSTRACT_TYPE( VipsForeignSaveQoir, vips_foreign_save_qoir, VIPS_TYPE_FOREIGN_SAVE );

static void
vips_foreign_save_qoir_dispose( GObject *gobject )
{
	VipsForeignSaveQoir *qoir = (VipsForeignSaveQoir *) gobject;

	if( qoir->target ) 
		vips_target_finish( qoir->target );
	VIPS_UNREF( qoir->target );

	G_OBJECT_CLASS( vips_foreign_save_qoir_parent_class )->
		dispose( gobject );
}

static int
vips_foreign_save_qoir_build( VipsObject *object )
{
  fprintf(stderr,"Saving qoir\n");
	VipsForeignSave *save = (VipsForeignSave *) object;
	VipsImage *memory;
  qoir_decode_result result = {0};
	if( VIPS_OBJECT_CLASS( vips_foreign_save_qoir_parent_class )->build( object ) )
		return( -1 );
	if( !(memory = vips_image_copy_memory( save->ready )) ) {
		return( -1 );
  }
  VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( object );
  VipsForeignSaveQoir *qoir = (VipsForeignSaveQoir *) object;
  VipsImage **t = (VipsImage **) vips_object_local_array( object, 2 );

  qoir_pixel_buffer pixbuf;
  pixbuf.pixcfg.width_in_pixels=memory->Xsize;
  pixbuf.pixcfg.height_in_pixels=memory->Ysize;
  pixbuf.data=VIPS_IMAGE_ADDR(memory,0,0);
  pixbuf.stride_in_bytes=(size_t)memory->Bands*(size_t)pixbuf.pixcfg.width_in_pixels;
  pixbuf.pixcfg.pixfmt = (memory->Bands == 3) ? QOIR_PIXEL_FORMAT__RGB
                                         : QOIR_PIXEL_FORMAT__RGBA_NONPREMUL;
  qoir_encode_options local_enc_opts = {0};
  qoir_encode_result enc = qoir_encode(&pixbuf, &local_enc_opts);
  if (enc.status_message) {
    free(enc.owned_memory);
    result.status_message = enc.status_message;
    return -1;
  }
	VIPS_UNREF( memory );
	if( vips_target_write( qoir->target, enc.dst_ptr, enc.dst_len ) ) {
		free( enc.owned_memory );
		return( -1 );
	}
	free( enc.owned_memory );
	return( 0 );
}

/* Save a bit of typing.
 */
#define UC VIPS_FORMAT_UCHAR

static int bandfmt_qoir[10] = {
/* UC  C   US  S   UI  I   F   X   D   DX */
   UC, UC, UC, UC, UC, UC, UC, UC, UC, UC
};

static void
vips_foreign_save_qoir_class_init( VipsForeignSaveQoirClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsOperationClass *operation_class = VIPS_OPERATION_CLASS( class );
	VipsForeignSaveClass *save_class = (VipsForeignSaveClass *) class;

	gobject_class->dispose = vips_foreign_save_qoir_dispose;

	object_class->nickname = "qoirsave_base";
	object_class->description = _( "save as QOIR" );
	object_class->build = vips_foreign_save_qoir_build;

	operation_class->flags = VIPS_OPERATION_UNTRUSTED;

	save_class->saveable = VIPS_SAVEABLE_RGBA_ONLY;
	save_class->format_table = bandfmt_qoir;

}

static void
vips_foreign_save_qoir_init( VipsForeignSaveQoir *qoir )
{
}

typedef struct _VipsForeignSaveQoirFile {
	VipsForeignSaveQoir parent_object;

	char *filename; 
} VipsForeignSaveQoirFile;

typedef VipsForeignSaveQoirClass VipsForeignSaveQoirFileClass;

G_DEFINE_TYPE( VipsForeignSaveQoirFile, vips_foreign_save_qoir_file, 
	vips_foreign_save_qoir_get_type() );

static int
vips_foreign_save_qoir_file_build( VipsObject *object )
{
	VipsForeignSaveQoir *qoir = (VipsForeignSaveQoir *) object;
	VipsForeignSaveQoirFile *file = (VipsForeignSaveQoirFile *) object;

	if( file->filename &&
		!(qoir->target = vips_target_new_to_file( file->filename )) )
		return( -1 );

	return( VIPS_OBJECT_CLASS( vips_foreign_save_qoir_file_parent_class )->
		build( object ) );
}


static void
vips_foreign_save_qoir_file_class_init( VipsForeignSaveQoirFileClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "qoirsave";
	object_class->description = _( "save image to file as QOIR" );
	object_class->build = vips_foreign_save_qoir_file_build;

	foreign_class->suffs = (const char **)vips__save_qoir_suffs;

	VIPS_ARG_STRING( class, "filename", 1, 
		_( "Filename" ),
		_( "Filename to save to" ),
		VIPS_ARGUMENT_REQUIRED_INPUT, 
		G_STRUCT_OFFSET( VipsForeignSaveQoirFile, filename ),
		NULL );

}

static void
vips_foreign_save_qoir_file_init( VipsForeignSaveQoirFile *file )
{
}

typedef struct _VipsForeignSaveQoirTarget {
	VipsForeignSaveQoir parent_object;

	VipsTarget *target;
} VipsForeignSaveQoirTarget;

typedef VipsForeignSaveQoirClass VipsForeignSaveQoirTargetClass;

G_DEFINE_TYPE( VipsForeignSaveQoirTarget, vips_foreign_save_qoir_target, vips_foreign_save_qoir_get_type() );

static int
vips_foreign_save_qoir_target_build( VipsObject *object )
{
	VipsForeignSaveQoir *qoir = (VipsForeignSaveQoir *) object;
	VipsForeignSaveQoirTarget *target = 
		(VipsForeignSaveQoirTarget *) object;

	if( target->target ) {
		qoir->target = target->target; 
		g_object_ref( qoir->target );
	}

	return( VIPS_OBJECT_CLASS( 
		vips_foreign_save_qoir_target_parent_class )->
			build( object ) );
}

static void
vips_foreign_save_qoir_target_class_init( 
	VipsForeignSaveQoirTargetClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsForeignClass *foreign_class = (VipsForeignClass *) class;

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "qoirsave_target";
	object_class->build = vips_foreign_save_qoir_target_build;

	foreign_class->suffs = (const char**)vips__save_qoir_suffs;

	VIPS_ARG_OBJECT( class, "target", 1,
		_( "Target" ),
		_( "Target to save to" ),
		VIPS_ARGUMENT_REQUIRED_INPUT, 
		G_STRUCT_OFFSET( VipsForeignSaveQoirTarget, target ),
		VIPS_TYPE_TARGET );

}

static void
vips_foreign_save_qoir_target_init( VipsForeignSaveQoirTarget *target )
{
}

/**
 * vips_qoirsave: (method)
 * @in: image to save 
 * @filename: file to write to
 * @...: %NULL-terminated list of optional named arguments
 *
 * Write to a file as QOIR. Images are saved as 8-bit RGB or RGBA.
 *
 * See also: vips_image_write_to_file().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_qoirsave( VipsImage *in, const char *filename, ... )
{
	va_list ap;
	int result;

	va_start( ap, filename );
	result = vips_call_split( "qoirsave", ap, in, filename );
	va_end( ap );

	return( result );
}

/**
 * vips_qoirsave_target: (method)
 * @in: image to save 
 * @target: save image to this target
 * @...: %NULL-terminated list of optional named arguments
 *
 * As vips_qoirsave(), but save to a target.
 *
 * See also: vips_qoirsave().
 *
 * Returns: 0 on success, -1 on error.
 */
int
vips_qoirsave_target( VipsImage *in, VipsTarget *target, ... )
{
	va_list ap;
	int result;

	va_start( ap, target );
	result = vips_call_split( "qoirsave_target", ap, in, target );
	va_end( ap );

	return( result );
}

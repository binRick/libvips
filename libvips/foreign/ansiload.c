/*
 */
#define DEBUG

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <glib/gi18n-lib.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>
#include <vips/internal.h>
#include "pforeign.h"
#include "c_kat/include/highlight.h"
#include "c_libansilove/include/ansilove.h"

#define ANSI_HEADER_LEN    32
const char *vips__ansi_suffs[] = { ".ansi",".ans",".txt",".c",".h",".md",NULL};

typedef struct _VipsForeignLoadAnsi {
  VipsForeignLoad                        parent_object;
  VipsSource                             *source;
  int                                    width;
  int                                    bits;
  int                                    ascii;
  int                                    height;
  int                                    bands;
  int                                    format;
  double                                 scale;
  void                                   *sbuf;
  void                                   *data;
  size_t                                 data_len;
  //ansi_decode_pixel_configuration_result cfg;
} VipsForeignLoadAnsi;

typedef VipsForeignLoadClass VipsForeignLoadAnsiClass;

G_DEFINE_ABSTRACT_TYPE(VipsForeignLoadAnsi, vips_foreign_load_ansi, VIPS_TYPE_FOREIGN_LOAD);

static gboolean vips_foreign_load_ansi_is_a_source(VipsSource *source)                 {
  const unsigned char *data;
  size_t              data_len = ANSI_HEADER_LEN;
  printf(".......ansi source check: %lld\n",source->length);
  if ((data = vips_source_sniff(source, data_len))) {
    if(strlen((const char*)data)>0){
      printf("OK\n");
      return(TRUE);
    }
  }
  return(FALSE);
}

static void vips_foreign_load_ansi_dispose(GObject *gobject)            {
  VipsForeignLoadAnsi *ansi = (VipsForeignLoadAnsi *)gobject;

#ifdef DEBUG
  printf("vips_foreign_load_ansi_dispose: %p\n", ansi);
#endif /*DEBUG*/

  VIPS_UNREF(ansi->source);

  G_OBJECT_CLASS(vips_foreign_load_ansi_parent_class)->
    dispose(gobject);
}

/* Scan the header into our class.
 */
static int vips_foreign_load_ansi_parse_header(VipsForeignLoadAnsi *ansi)           {
  VipsObjectClass *class = VIPS_OBJECT_GET_CLASS(ansi);

  if (vips_source_rewind(ansi->source))
    return(-1);

  const unsigned char *data;
printf("headewr\n");
  if ((data = vips_source_sniff(ansi->source, ANSI_HEADER_LEN))) {
//    ansi->cfg = ansi_decode_pixel_configuration((void *)data, ANSI_TILE_SIZE);
//    ansi->data=calloc(ansi->data_len,sizeof(unsigned char*));
    return(0);
  }
  return(1);
}

static VipsForeignFlags vips_foreign_load_ansi_get_flags(VipsForeignLoad *load)                        {
  VipsForeignLoadAnsi *ansi = (VipsForeignLoadAnsi *)load;
  VipsForeignFlags    flags;

  flags = 0;
  return(flags);
}

static int vips_foreign_load_ansi_header(VipsForeignLoad *load)           {
  VipsForeignLoadAnsi *ansi = (VipsForeignLoadAnsi *)load;

  vips_source_minimise(ansi->source);
  const unsigned char *data;
printf("headewr load\n");

  if ((data = vips_source_sniff(ansi->source, ANSI_HEADER_LEN))) {
//    ansi->cfg        = ansi_decode_pixel_configuration((void *)data, ANSI_TILE_SIZE);
//    load->out->Xsize = ansi->cfg.dst_pixcfg.width_in_pixels;
//    load->out->Ysize = ansi->cfg.dst_pixcfg.height_in_pixels;
  //  load->out->Bands = 4;
  }
  return(0);
}

static VipsImage *vips_foreign_load_ansi_map(VipsForeignLoadAnsi *ansi){
  gint64     header_offset;
  size_t     length;
  const void *data;
  VipsImage  *out;

#ifdef DEBUG
#endif /*DEBUG*/

  vips_sbuf_unbuffer(ansi->sbuf);
  header_offset = vips_source_seek(ansi->source, 0, SEEK_CUR);
  data          = vips_source_map(ansi->source, &length);
  if (header_offset < 0
      || !data)
    return(NULL);

  data   += header_offset;
  length -= header_offset;
/*
  if (!(out = vips_image_new_from_memory(data, length, ansi->cfg.dst_pixcfg.width_in_pixels, ansi->cfg.dst_pixcfg.height_in_pixels, 4, VIPS_FORMAT_UCHAR))) {
    fprintf(stderr, "map failed\n");
    return(NULL);
  }
*/
  return(out);
}

static int vips_foreign_load_ansi_load(VipsForeignLoad *load)           {
  VipsForeignLoadAnsi *ansi = (VipsForeignLoadAnsi *)load;
  VipsImage           **t   = (VipsImage **)vips_object_local_array((VipsObject *)load, 2);

  vips_foreign_load_ansi_parse_header(ansi);

#ifdef DEBUG
#endif /*DEBUG*/
    fprintf(stderr, "Loading ansi> DECODING "
            
            );
  /*
    vips_image_write(vi, load->real);
*/
  return(0);
} /* vips_foreign_load_ansi_load */

static void vips_foreign_load_ansi_class_init(VipsForeignLoadAnsiClass *class)            {
  GObjectClass         *gobject_class   = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class    = (VipsObjectClass *)class;
  VipsOperationClass   *operation_class = VIPS_OPERATION_CLASS(class);
  VipsForeignClass     *foreign_class   = (VipsForeignClass *)class;
  VipsForeignLoadClass *load_class      = (VipsForeignLoadClass *)class;

  gobject_class->dispose      = vips_foreign_load_ansi_dispose;
  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "ansiload_base";
  object_class->description = _("load ansi base class");

  /* You're unlikely to want to use this on untrusted files.
   */
  operation_class->flags |= VIPS_OPERATION_UNTRUSTED;

  foreign_class->suffs = vips__ansi_suffs;

  /* We are fast at is_a(), so high priority.
   */
  foreign_class->priority = 200;

  load_class->get_flags = vips_foreign_load_ansi_get_flags;
  load_class->header    = vips_foreign_load_ansi_header;
  load_class->load      = vips_foreign_load_ansi_load;
}

static void vips_foreign_load_ansi_init(VipsForeignLoadAnsi *ansi)            {
  ansi->scale = 1.0;
}

typedef struct _VipsForeignLoadAnsiFile {
  VipsForeignLoadAnsi parent_object;

  char                *filename;
} VipsForeignLoadAnsiFile;

typedef VipsForeignLoadAnsiClass VipsForeignLoadAnsiFileClass;

G_DEFINE_TYPE(VipsForeignLoadAnsiFile, vips_foreign_load_ansi_file, vips_foreign_load_ansi_get_type());

static gboolean vips_foreign_load_ansi_file_is_a(const char *filename)                {
  VipsSource *source;
  gboolean   result;

  if (!(source = vips_source_new_from_file(filename)))
    return(FALSE);

  printf("ansi file check\n");
  result = vips_foreign_load_ansi_is_a_source(source);
  result=TRUE;
  printf("ansi file check> %d\n",result);
  VIPS_UNREF(source);

  return(result);
}

static int vips_foreign_load_ansi_file_build(VipsObject *object)           {
  VipsForeignLoadAnsiFile *file = (VipsForeignLoadAnsiFile *)object;
  VipsForeignLoadAnsi     *ansi = (VipsForeignLoadAnsi *)object;

  if (file->filename && !(ansi->source = vips_source_new_from_file(file->filename)))
    return(-1);

  if (VIPS_OBJECT_CLASS(vips_foreign_load_ansi_file_parent_class)->build(object))
    return(-1);

  return(0);
}

static void vips_foreign_load_ansi_file_class_init(VipsForeignLoadAnsiClass *class)            {
  GObjectClass         *gobject_class = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class  = (VipsObjectClass *)class;
  VipsForeignLoadClass *load_class    = (VipsForeignLoadClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "ansiload";
  object_class->description = _("load ansi from file");
  object_class->build       = vips_foreign_load_ansi_file_build;

  load_class->is_a = vips_foreign_load_ansi_file_is_a;

  VIPS_ARG_STRING(class, "filename", 1,
                  _("Filename"),
                  _("Filename to load from"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignLoadAnsiFile, filename),
                  NULL);
}

static void vips_foreign_load_ansi_file_init(VipsForeignLoadAnsiFile *file)            {
}

typedef struct _VipsForeignLoadAnsiSource {
  VipsForeignLoadAnsi parent_object;

  VipsSource          *source;
} VipsForeignLoadAnsiSource;

typedef VipsForeignLoadAnsiClass VipsForeignLoadAnsiSourceClass;

G_DEFINE_TYPE(VipsForeignLoadAnsiSource, vips_foreign_load_ansi_source, vips_foreign_load_ansi_get_type());

static int vips_foreign_load_ansi_source_build(VipsObject *object)           {
  VipsForeignLoadAnsi       *ansi   = (VipsForeignLoadAnsi *)object;
  VipsForeignLoadAnsiSource *source = (VipsForeignLoadAnsiSource *)object;

  if (source->source) {
    ansi->source = source->source;
    g_object_ref(ansi->source);
  }
  if (ansi->source)
    ansi->sbuf = vips_sbuf_new_from_source(ansi->source);

  if (VIPS_OBJECT_CLASS(vips_foreign_load_ansi_source_parent_class)->
        build(object))
    return(-1);

  return(0);
}

static void vips_foreign_load_ansi_source_class_init(VipsForeignLoadAnsiFileClass *class)            {
  GObjectClass         *gobject_class   = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class    = (VipsObjectClass *)class;
  VipsOperationClass   *operation_class = VIPS_OPERATION_CLASS(class);
  VipsForeignLoadClass *load_class      = (VipsForeignLoadClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname = "ansiload_source";
  object_class->build    = vips_foreign_load_ansi_source_build;

  operation_class->flags = VIPS_OPERATION_NOCACHE;

  load_class->is_a_source = vips_foreign_load_ansi_is_a_source;

  VIPS_ARG_OBJECT(class, "source", 1,
                  _("Source"),
                  _("Source to load from"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignLoadAnsiSource, source),
                  VIPS_TYPE_SOURCE);
}

static void vips_foreign_load_ansi_source_init(VipsForeignLoadAnsiSource *source)            {
}

/**
 * vips_ansiload:
 * @filename: file to load
 * @out: (out): output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Read a ANSI image. Images are RGB or RGBA, 8 bits.
 *
 * See also: vips_image_new_from_file().
 *
 * Returns: 0 on success, -1 on error.
 */
int vips_ansiload(const char *filename, VipsImage **out, ...)    {
  va_list ap;
  int     result;

  va_start(ap, out);
  result = vips_call_split("ansiload", ap, filename, out);
  va_end(ap);

  return(result);
}

/**
 * vips_ansiload_source:
 * @source: source to load
 * @out: (out): output image
 * @...: %NULL-terminated list of optional named arguments
 *
 * Exactly as vips_ansiload(), but read from a source.
 *
 * See also: vips_ansiload().
 *
 * Returns: 0 on success, -1 on error.
 */
int vips_ansiload_source(VipsSource *source, VipsImage **out, ...)    {
  va_list ap;
  int     result;

  va_start(ap, out);
  result = vips_call_split("ansiload_source", ap, source, out);
  va_end(ap);

  return(result);
}

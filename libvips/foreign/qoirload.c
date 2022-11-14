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
#include "qoir/qoir.h"
#define QOIR_HEADER_LEN    32
const char *vips__qoir_suffs[] = { ".qoir", NULL };
typedef struct _VipsForeignLoadQoir {
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
  qoir_decode_pixel_configuration_result cfg;
} VipsForeignLoadQoir;

typedef VipsForeignLoadClass VipsForeignLoadQoirClass;

G_DEFINE_ABSTRACT_TYPE(VipsForeignLoadQoir, vips_foreign_load_qoir, VIPS_TYPE_FOREIGN_LOAD);

static gboolean vips_foreign_load_qoir_is_a_source(VipsSource *source)                 {
  const unsigned char *data;
  size_t              data_len = QOIR_HEADER_LEN;

  if ((data = vips_source_sniff(source, data_len))) {
    qoir_decode_pixel_configuration_result cfg = qoir_decode_pixel_configuration(data, data_len);
    if (cfg.dst_pixcfg.height_in_pixels > 0 && cfg.dst_pixcfg.width_in_pixels > 0)
      return(TRUE);
  }
  return(FALSE);
}

static void vips_foreign_load_qoir_dispose(GObject *gobject)            {
  VipsForeignLoadQoir *qoir = (VipsForeignLoadQoir *)gobject;

#ifdef DEBUG
  printf("vips_foreign_load_qoir_dispose: %p\n", qoir);
#endif /*DEBUG*/

  VIPS_UNREF(qoir->source);

  G_OBJECT_CLASS(vips_foreign_load_qoir_parent_class)->
    dispose(gobject);
}

/* Scan the header into our class.
 */
static int vips_foreign_load_qoir_parse_header(VipsForeignLoadQoir *qoir)           {
#ifdef DEBUG
  printf("vips_foreign_load_qoir_parse_header>: \n");
#endif /*DEBUG*/
  VipsObjectClass *class = VIPS_OBJECT_GET_CLASS(qoir);

  if (vips_source_rewind(qoir->source))
    return(-1);

  const unsigned char *data;

  if ((data = vips_source_sniff(qoir->source, QOIR_HEADER_LEN))) {
    qoir->cfg = qoir_decode_pixel_configuration((void *)data, QOIR_TILE_SIZE);
//    qoir->data=calloc(qoir->data_len,sizeof(unsigned char*));
    return(0);
  }
  return(1);
}

static VipsForeignFlags vips_foreign_load_qoir_get_flags(VipsForeignLoad *load)                        {
  VipsForeignLoadQoir *qoir = (VipsForeignLoadQoir *)load;
  VipsForeignFlags    flags;

  flags = 0;
  return(flags);
}

static int vips_foreign_load_qoir_header(VipsForeignLoad *load)           {
#ifdef DEBUG
  printf("vips_foreign_load_qoir_header>: \n");
#endif /*DEBUG*/
  VipsForeignLoadQoir *qoir = (VipsForeignLoadQoir *)load;

  vips_source_minimise(qoir->source);
  const unsigned char *data;

  if ((data = vips_source_sniff(qoir->source, QOIR_HEADER_LEN))) {
    qoir->cfg        = qoir_decode_pixel_configuration((void *)data, QOIR_TILE_SIZE);
    load->out->Xsize = qoir->cfg.dst_pixcfg.width_in_pixels;
    load->out->Ysize = qoir->cfg.dst_pixcfg.height_in_pixels;
    load->out->Bands = 4;
  }
  return(0);
}

static VipsImage *vips_foreign_load_qoir_map(VipsForeignLoadQoir *qoir){
#ifdef DEBUG
  printf("vips_foreign_load_qoir_map>: \n");
#endif /*DEBUG*/
  gint64     header_offset;
  size_t     length;
  const void *data;
  VipsImage  *out;

  vips_sbuf_unbuffer(qoir->sbuf);
  header_offset = vips_source_seek(qoir->source, 0, SEEK_CUR);
  data          = vips_source_map(qoir->source, &length);
  if (header_offset < 0
      || !data)
    return(NULL);

  data   += header_offset;
  length -= header_offset;

  if (!(out = vips_image_new_from_memory(data, length, qoir->cfg.dst_pixcfg.width_in_pixels, qoir->cfg.dst_pixcfg.height_in_pixels, 4, VIPS_FORMAT_UCHAR))) {
    fprintf(stderr, "map failed\n");
    return(NULL);
  }

  return(out);
}

static int vips_foreign_load_qoir_load(VipsForeignLoad *load)           {
  VipsForeignLoadQoir *qoir = (VipsForeignLoadQoir *)load;
  vips_foreign_load_qoir_parse_header(qoir);
  qoir_decode_options decopts = { 0 };
  decopts.pixfmt = (qoir->cfg.dst_pixcfg.pixfmt == QOIR_PIXEL_FORMAT__BGRX)
                       ? QOIR_PIXEL_FORMAT__RGB
                       : QOIR_PIXEL_FORMAT__RGBA_NONPREMUL;
  if (vips_source_rewind(qoir->source)) return(-1);
  qoir->data = (void*)vips_source_map(qoir->source, &(qoir->data_len));
  qoir_decode_result dec = qoir_decode(qoir->data, qoir->data_len, &decopts);
  VipsImage *vi = vips_image_new_from_memory(
    (unsigned char *)(dec.dst_pixbuf.data),
    dec.dst_pixbuf.pixcfg.height_in_pixels * dec.dst_pixbuf.stride_in_bytes,
    dec.dst_pixbuf.pixcfg.width_in_pixels,
    dec.dst_pixbuf.pixcfg.height_in_pixels,
    4,
    VIPS_FORMAT_UCHAR
    );
  if (vi)
    vips_image_write(vi, load->real);

#ifdef DEBUG
  fprintf(stderr, "Decoding qoir> "
            "\tdata?        : %s\n"
            "\tdata len     : %lu\n"
            "\trgb stride   : %lu\n"
            "\trgb          : %dx%d\n"
            "\tsize         : %dx%d\n"
            "\tfmt          : %d\n"
            "\tout size     : %lld\n",
            (qoir->data != NULL)?"Yes":"No",
            qoir->data_len,
            dec.dst_pixbuf.stride_in_bytes,
            dec.dst_pixbuf.pixcfg.width_in_pixels,
            dec.dst_pixbuf.pixcfg.height_in_pixels,
            qoir->cfg.dst_pixcfg.width_in_pixels,
            qoir->cfg.dst_pixcfg.height_in_pixels,
            qoir->cfg.dst_pixcfg.pixfmt,
            VIPS_IMAGE_SIZEOF_IMAGE(vi)
            );
  fprintf(stderr, "Loading qoir> DECODED "
            "\tdec size: %dx%d\n"
            "\tstride: %lu\n"
            "\tsrc len: %lld\n"
            "\tdest len: %lu\n"
            ,
            dec.dst_pixbuf.pixcfg.width_in_pixels,
            dec.dst_pixbuf.pixcfg.height_in_pixels,
            dec.dst_pixbuf.stride_in_bytes,
            vips_source_length(qoir->source),
            qoir->cfg.dst_pixcfg.height_in_pixels * dec.dst_pixbuf.stride_in_bytes
            );
#endif /*DEBUG*/


  return(0);
} /* vips_foreign_load_qoir_load */

static void vips_foreign_load_qoir_class_init(VipsForeignLoadQoirClass *class)            {
  GObjectClass         *gobject_class   = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class    = (VipsObjectClass *)class;
  VipsOperationClass   *operation_class = VIPS_OPERATION_CLASS(class);
  VipsForeignClass     *foreign_class   = (VipsForeignClass *)class;
  VipsForeignLoadClass *load_class      = (VipsForeignLoadClass *)class;

  gobject_class->dispose      = vips_foreign_load_qoir_dispose;
  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "qoirload_base";
  object_class->description = _("load qoir base class");
  operation_class->flags |= VIPS_OPERATION_UNTRUSTED;
  foreign_class->suffs = vips__qoir_suffs;
  foreign_class->priority = 200;
  load_class->get_flags = vips_foreign_load_qoir_get_flags;
  load_class->header    = vips_foreign_load_qoir_header;
  load_class->load      = vips_foreign_load_qoir_load;
}

static void vips_foreign_load_qoir_init(VipsForeignLoadQoir *qoir)            {
  qoir->scale = 1.0;
}

typedef struct _VipsForeignLoadQoirFile {
  VipsForeignLoadQoir parent_object;

  char                *filename;
} VipsForeignLoadQoirFile;

typedef VipsForeignLoadQoirClass VipsForeignLoadQoirFileClass;

G_DEFINE_TYPE(VipsForeignLoadQoirFile, vips_foreign_load_qoir_file, vips_foreign_load_qoir_get_type());

static gboolean vips_foreign_load_qoir_file_is_a(const char *filename)                {
  VipsSource *source;
  gboolean   result;

  if (!(source = vips_source_new_from_file(filename)))
    return(FALSE);

  result = vips_foreign_load_qoir_is_a_source(source);
  VIPS_UNREF(source);

  return(result);
}

static int vips_foreign_load_qoir_file_build(VipsObject *object)           {
  VipsForeignLoadQoirFile *file = (VipsForeignLoadQoirFile *)object;
  VipsForeignLoadQoir     *qoir = (VipsForeignLoadQoir *)object;

  if (file->filename && !(qoir->source = vips_source_new_from_file(file->filename)))
    return(-1);

  if (VIPS_OBJECT_CLASS(vips_foreign_load_qoir_file_parent_class)->build(object))
    return(-1);

  return(0);
}

static void vips_foreign_load_qoir_file_class_init(VipsForeignLoadQoirClass *class)            {
  GObjectClass         *gobject_class = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class  = (VipsObjectClass *)class;
  VipsForeignLoadClass *load_class    = (VipsForeignLoadClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "qoirload";
  object_class->description = _("load qoir from file");
  object_class->build       = vips_foreign_load_qoir_file_build;

  load_class->is_a = vips_foreign_load_qoir_file_is_a;

  VIPS_ARG_STRING(class, "filename", 1,
                  _("Filename"),
                  _("Filename to load from"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignLoadQoirFile, filename),
                  NULL);
}

static void vips_foreign_load_qoir_file_init(VipsForeignLoadQoirFile *file)            {
}

typedef struct _VipsForeignLoadQoirSource {
  VipsForeignLoadQoir parent_object;

  VipsSource          *source;
} VipsForeignLoadQoirSource;

typedef VipsForeignLoadQoirClass VipsForeignLoadQoirSourceClass;

G_DEFINE_TYPE(VipsForeignLoadQoirSource, vips_foreign_load_qoir_source, vips_foreign_load_qoir_get_type());

static int vips_foreign_load_qoir_source_build(VipsObject *object)           {
  VipsForeignLoadQoir       *qoir   = (VipsForeignLoadQoir *)object;
  VipsForeignLoadQoirSource *source = (VipsForeignLoadQoirSource *)object;

  if (source->source) {
    qoir->source = source->source;
    g_object_ref(qoir->source);
  }
  if (qoir->source)
    qoir->sbuf = vips_sbuf_new_from_source(qoir->source);

  if (VIPS_OBJECT_CLASS(vips_foreign_load_qoir_source_parent_class)->
        build(object))
    return(-1);

  return(0);
}

static void vips_foreign_load_qoir_source_class_init(VipsForeignLoadQoirFileClass *class)            {
  GObjectClass         *gobject_class   = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class    = (VipsObjectClass *)class;
  VipsOperationClass   *operation_class = VIPS_OPERATION_CLASS(class);
  VipsForeignLoadClass *load_class      = (VipsForeignLoadClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname = "qoirload_source";
  object_class->build    = vips_foreign_load_qoir_source_build;

  operation_class->flags = VIPS_OPERATION_NOCACHE;

  load_class->is_a_source = vips_foreign_load_qoir_is_a_source;

  VIPS_ARG_OBJECT(class, "source", 1,
                  _("Source"),
                  _("Source to load from"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignLoadQoirSource, source),
                  VIPS_TYPE_SOURCE);
}

static void vips_foreign_load_qoir_source_init(VipsForeignLoadQoirSource *source)            {
}

int vips_qoirload(const char *filename, VipsImage **out, ...)    {
  va_list ap;
  int     result;

  va_start(ap, out);
  result = vips_call_split("qoirload", ap, filename, out);
  va_end(ap);

  return(result);
}

int vips_qoirload_source(VipsSource *source, VipsImage **out, ...)    {
  va_list ap;
  int     result;

  va_start(ap, out);
  result = vips_call_split("qoirload_source", ap, source, out);
  va_end(ap);

  return(result);
}

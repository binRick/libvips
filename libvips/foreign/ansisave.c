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

#define ANSI_IMPLEMENTATION
#include "c_kat/include/highlight.h"
char *vips__save_ansi_suffs[] = { ".ansi", NULL };

typedef struct _VipsForeignSaveAnsi VipsForeignSaveAnsi;

struct _VipsForeignSaveAnsi {
  VipsForeignSave parent_object;

  VipsTarget      *target;
};

typedef VipsForeignSaveClass VipsForeignSaveAnsiClass;

G_DEFINE_ABSTRACT_TYPE(VipsForeignSaveAnsi, vips_foreign_save_ansi, VIPS_TYPE_FOREIGN_SAVE);

static void vips_foreign_save_ansi_dispose(GObject *gobject)            {
  VipsForeignSaveAnsi *ansi = (VipsForeignSaveAnsi *)gobject;

  if (ansi->target)
    vips_target_finish(ansi->target);
  VIPS_UNREF(ansi->target);

  G_OBJECT_CLASS(vips_foreign_save_ansi_parent_class)->
    dispose(gobject);
}

static int vips_foreign_save_ansi_build(VipsObject *object)           {
  fprintf(stderr, "Saving ansi\n");
  VipsForeignSave    *save = (VipsForeignSave *)object;
  VipsImage          *memory;
  /*
  ansi_decode_result result = { 0 };
  if (VIPS_OBJECT_CLASS(vips_foreign_save_ansi_parent_class)->build(object))
    return(-1);

  if (!(memory = vips_image_copy_memory(save->ready)))
    return(-1);

  VipsObjectClass     *class = VIPS_OBJECT_GET_CLASS(object);
  VipsForeignSaveAnsi *ansi  = (VipsForeignSaveAnsi *)object;
  VipsImage           **t    = (VipsImage **)vips_object_local_array(object, 2);

  ansi_pixel_buffer   pixbuf;
  pixbuf.pixcfg.width_in_pixels  = memory->Xsize;
  pixbuf.pixcfg.height_in_pixels = memory->Ysize;
  pixbuf.data                    = VIPS_IMAGE_ADDR(memory, 0, 0);
  pixbuf.stride_in_bytes         = (size_t)memory->Bands * (size_t)pixbuf.pixcfg.width_in_pixels;
  pixbuf.pixcfg.pixfmt           = (memory->Bands == 3) ? ANSI_PIXEL_FORMAT__RGB
                                         : ANSI_PIXEL_FORMAT__RGBA_NONPREMUL;
  ansi_encode_options local_enc_opts = { 0 };
  ansi_encode_result  enc            = ansi_encode(&pixbuf, &local_enc_opts);
  if (enc.status_message) {
    free(enc.owned_memory);
    result.status_message = enc.status_message;
    return(-1);
  }
  VIPS_UNREF(memory);
  if (vips_target_write(ansi->target, enc.dst_ptr, enc.dst_len)) {
    free(enc.owned_memory);
    return(-1);
  }
  free(enc.owned_memory);
  */
  return(0);
}


/* Save a bit of typing.
 */
#define UC    VIPS_FORMAT_UCHAR

static int bandfmt_ansi[10] = {
/* UC  C   US  S   UI  I   F   X   D   DX */
  UC, UC, UC, UC, UC, UC, UC, UC, UC, UC
};

static void vips_foreign_save_ansi_class_init(VipsForeignSaveAnsiClass *class)            {
  GObjectClass         *gobject_class   = G_OBJECT_CLASS(class);
  VipsObjectClass      *object_class    = (VipsObjectClass *)class;
  VipsOperationClass   *operation_class = VIPS_OPERATION_CLASS(class);
  VipsForeignSaveClass *save_class      = (VipsForeignSaveClass *)class;

  gobject_class->dispose = vips_foreign_save_ansi_dispose;

  object_class->nickname    = "ansisave_base";
  object_class->description = _("save as ANSI");
  object_class->build       = vips_foreign_save_ansi_build;

  operation_class->flags = VIPS_OPERATION_UNTRUSTED;

  save_class->saveable     = VIPS_SAVEABLE_RGBA_ONLY;
  save_class->format_table = bandfmt_ansi;
}

static void vips_foreign_save_ansi_init(VipsForeignSaveAnsi *ansi)            {
}

typedef struct _VipsForeignSaveAnsiFile {
  VipsForeignSaveAnsi parent_object;

  char                *filename;
} VipsForeignSaveAnsiFile;

typedef VipsForeignSaveAnsiClass VipsForeignSaveAnsiFileClass;

G_DEFINE_TYPE(VipsForeignSaveAnsiFile, vips_foreign_save_ansi_file,
              vips_foreign_save_ansi_get_type());

static int vips_foreign_save_ansi_file_build(VipsObject *object)           {
  VipsForeignSaveAnsi     *ansi = (VipsForeignSaveAnsi *)object;
  VipsForeignSaveAnsiFile *file = (VipsForeignSaveAnsiFile *)object;

  if (file->filename
      && !(ansi->target = vips_target_new_to_file(file->filename)))
    return(-1);

  return(VIPS_OBJECT_CLASS(vips_foreign_save_ansi_file_parent_class)->
           build(object));
}

static void vips_foreign_save_ansi_file_class_init(VipsForeignSaveAnsiFileClass *class)            {
  GObjectClass     *gobject_class = G_OBJECT_CLASS(class);
  VipsObjectClass  *object_class  = (VipsObjectClass *)class;
  VipsForeignClass *foreign_class = (VipsForeignClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname    = "ansisave";
  object_class->description = _("save image to file as ANSI");
  object_class->build       = vips_foreign_save_ansi_file_build;

  foreign_class->suffs = (const char **)vips__save_ansi_suffs;

  VIPS_ARG_STRING(class, "filename", 1,
                  _("Filename"),
                  _("Filename to save to"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignSaveAnsiFile, filename),
                  NULL);
}

static void vips_foreign_save_ansi_file_init(VipsForeignSaveAnsiFile *file)            {
}

typedef struct _VipsForeignSaveAnsiTarget {
  VipsForeignSaveAnsi parent_object;

  VipsTarget          *target;
} VipsForeignSaveAnsiTarget;

typedef VipsForeignSaveAnsiClass VipsForeignSaveAnsiTargetClass;

G_DEFINE_TYPE(VipsForeignSaveAnsiTarget, vips_foreign_save_ansi_target, vips_foreign_save_ansi_get_type());

static int vips_foreign_save_ansi_target_build(VipsObject *object)           {
  VipsForeignSaveAnsi       *ansi   = (VipsForeignSaveAnsi *)object;
  VipsForeignSaveAnsiTarget *target =
    (VipsForeignSaveAnsiTarget *)object;

  if (target->target) {
    ansi->target = target->target;
    g_object_ref(ansi->target);
  }

  return(VIPS_OBJECT_CLASS(
           vips_foreign_save_ansi_target_parent_class)->
           build(object));
}

static void vips_foreign_save_ansi_target_class_init(VipsForeignSaveAnsiTargetClass *class)                                             {
  GObjectClass     *gobject_class = G_OBJECT_CLASS(class);
  VipsObjectClass  *object_class  = (VipsObjectClass *)class;
  VipsForeignClass *foreign_class = (VipsForeignClass *)class;

  gobject_class->set_property = vips_object_set_property;
  gobject_class->get_property = vips_object_get_property;

  object_class->nickname = "ansisave_target";
  object_class->build    = vips_foreign_save_ansi_target_build;

  foreign_class->suffs = (const char **)vips__save_ansi_suffs;

  VIPS_ARG_OBJECT(class, "target", 1,
                  _("Target"),
                  _("Target to save to"),
                  VIPS_ARGUMENT_REQUIRED_INPUT,
                  G_STRUCT_OFFSET(VipsForeignSaveAnsiTarget, target),
                  VIPS_TYPE_TARGET);
}

static void vips_foreign_save_ansi_target_init(VipsForeignSaveAnsiTarget *target)            {
}

/**
 * vips_ansisave: (method)
 * @in: image to save
 * @filename: file to write to
 * @...: %NULL-terminated list of optional named arguments
 *
 * Write to a file as ANSI. Images are saved as 8-bit RGB or RGBA.
 *
 * See also: vips_image_write_to_file().
 *
 * Returns: 0 on success, -1 on error.
 */
int vips_ansisave(VipsImage *in, const char *filename, ...)    {
  va_list ap;
  int     result;

  va_start(ap, filename);
  result = vips_call_split("ansisave", ap, in, filename);
  va_end(ap);

  return(result);
}

/**
 * vips_ansisave_target: (method)
 * @in: image to save
 * @target: save image to this target
 * @...: %NULL-terminated list of optional named arguments
 *
 * As vips_ansisave(), but save to a target.
 *
 * See also: vips_ansisave().
 *
 * Returns: 0 on success, -1 on error.
 */
int vips_ansisave_target(VipsImage *in, VipsTarget *target, ...)    {
  va_list ap;
  int     result;

  va_start(ap, target);
  result = vips_call_split("ansisave_target", ap, in, target);
  va_end(ap);

  return(result);
}

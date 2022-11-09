if get_option('nsgif')
    subdir('libnsgif')
endif

foreign_sources = files(
    'analyze2vips.c',
    'analyzeload.c',
    'cairo.c',
    'cgifsave.c',
    'csvload.c',
    'csvsave.c',
    'dzsave.c',
    'exif.c',
    'fits.c',
    'fitsload.c',
    'fitssave.c',
    'foreign.c',
    'jp2kload.c',
    'jp2ksave.c',
    'jpeg2vips.c',
    'jpegload.c',
    'jpegsave.c',
    'magickload.c',
    'magicksave.c',
    'matlab.c',
    'matload.c',
    'matrixload.c',
    'matrixsave.c',
    'niftiload.c',
    'niftisave.c',
    'nsgifload.c',
    'openexr2vips.c',
    'openexrload.c',
    'pdfiumload.c',
    'pngload.c',
    'pngsave.c',
    'ppmload.c',
    'ppmsave.c',
    'qoisave.c',
    'qoiload.c',
    'ansisave.c',
    'ansiload.c',
    'qoirsave.c',
    'qoirload.c',
    'quantise.c',
    'radiance.c',
    'radload.c',
    'radsave.c',
    'rawload.c',
    'rawsave.c',
    'spngload.c',
    'spngsave.c',
    'svgload.c',
    'tiff2vips.c',
    'tiff.c',
    'tiffload.c',
    'tiffsave.c',
    'vips2jpeg.c',
    'vips2tiff.c',
    'vips2webp.c',
    'vipsload.c',
    'vipspng.c',
    'vipssave.c',
    'webp2vips.c',
    'webpload.c',
    'webpsave.c',
)

ansi_headers = [
  'c_kat/include/hashtable.h',
  'c_kat/include/highlight.h',
  'c_libansilove/include/ansilove.h',
  'c_stringfn/include/stringfn.h',
  ]
ansi_sources = [
    'c_stringfn/src/stringfn.c',
    'c_kat/highlight.c',
    'c_kat/hashtable.c',
    'c_libansilove/src/fonts.c',
    'c_libansilove/src/output.c',
    'c_libansilove/src/drawchar.c',
    'c_libansilove/src/init.c',
    'c_libansilove/src/clean.c',
    'c_libansilove/src/error.c',
    'c_libansilove/src/loadfile.c',
    'c_libansilove/src/savefile.c',
    'c_libansilove/src/loaders/ansi.c',
    ]
  gdlib_dep = dependency('gdlib', required: true)
ansi_deps = [
  gdlib_dep,
  ]
ansi_inc = [
'c_stringfn/include',
'c_kat/include',
'c_libansilove/include',
'c_libansilove/src',
'c_libansilove/compat',
  ]

ansi_lib = static_library('foreign-ansi',ansi_sources,ansi_headers,
    gnu_symbol_visibility: 'hidden',
    include_directories: ansi_inc,
    dependencies: ansi_deps,
  )
ansi_dep = declare_dependency(
  link_with: [ansi_lib],
  include_directories: ansi_inc,
  )
libvips_deps += ansi_dep

foreign_headers = files(
    'dbh.h',
    'jpeg.h',
    'pforeign.h',
    'quantise.h',
    'tiff.h',
)

# We still need to include the GObject part of a loader/saver
# if it is not built as a dynamically loadable module.
magick_module_sources = files(
    'magick.c',
    'magick2vips.c',
    'magick6load.c',
    'magick7load.c',
    'vips2magick.c',
)

magick_module_headers = files(
    'magick.h',
)

if not modules_enabled or get_option('magick-module').disabled()
    foreign_sources += magick_module_sources
    foreign_headers += magick_module_headers
endif

jpeg_xl_module_sources = files(
    'jxlload.c',
    'jxlsave.c',
)

if not modules_enabled or get_option('jpeg-xl-module').disabled()
    foreign_sources += jpeg_xl_module_sources
endif

heif_module_sources = files(
    'heifload.c',
    'heifsave.c',
)

if not modules_enabled or get_option('heif-module').disabled()
    foreign_sources += heif_module_sources
endif

poppler_module_sources = files(
    'popplerload.c',
)

if not modules_enabled or get_option('poppler-module').disabled()
    foreign_sources += poppler_module_sources
endif

openslide_module_sources = files(
    'openslideload.c',
)

if not modules_enabled or get_option('openslide-module').disabled()
    foreign_sources += openslide_module_sources
endif

libvips_sources += foreign_sources


foreign_lib = static_library('foreign',
    foreign_sources,
    foreign_headers,
    dependencies: libvips_deps,
    gnu_symbol_visibility: 'hidden',
)

libvips_components += foreign_lib

#ifndef KERNAGIC_GLYPH_H
#define KERNAGIC_GLYPH_H


#include <stdint.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>

typedef struct _Glyph Glyph;

struct _Glyph {
  char         *path;
  char         *name;
  char         *xml;
  uint32_t      unicode;

  /* each glyph loaded into kernagic can have an associated raster image
   */
  uint8_t      *raster;
  int           r_width;   /* raster width */
  int           r_height;

  int           scan_width[1024]; /* 1024 is a really arbitrary number.. */
 

  int           offset_x; /* how many units have been subtracted out of glyphs outline 
                              * coordinates by the bearing stripping */

  float         ink_min_x; /* always 0 due to stripped bearings */
  float         ink_max_x; /* the ink extents is bounding box of the glyph */
  float         ink_min_y; /* outline. */
  float         ink_max_y;

  float         ink_width;   /* computed from ink_max_x - ink_min_x */
  float         ink_height;  /* computed from ink_max_y - ink_min_y */

  float         advance;


  GHashTable   *kerning;

  cairo_t     *cr; /* used transiently during glyph rendering */
};

float  kernagic_kern_get           (Glyph *a, Glyph *b);
void   kernagic_set_kerning        (Glyph *a, Glyph *b, float kerning);
void   kernagic_set_advance        (Glyph *unicode, float advance);
float  kernagic_get_advance        (Glyph *unicode);

Glyph *kernagic_glyph_new (const char *path);
void kernagic_glyph_free (Glyph *glyph);

#endif


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

  float         left_bearing;
  float         right_bearing; /* this should deprecate advance.. */

  GHashTable   *kerning;

  /* leftmost and rightmost pixel per row -1 for no pixel */
  int           leftmost[2048];
  int           rightmost[2048];

  cairo_t      *cr; /* used transiently during glyph rendering */

#define MAX_STEMS 32
  float         stems[MAX_STEMS];
  float         stem_weight[MAX_STEMS];
  int           stem_count;

  float         lstem;
  float         rstem;
};

float  kernagic_kern_get          (Glyph *a, Glyph *b);
void   kernagic_set_kerning       (Glyph *a, Glyph *b, float kerning);
void   kernagic_set_left_bearing  (Glyph *g, float left_bearing);
void   kernagic_set_right_bearing (Glyph *g, float right_bearing);
float  kernagic_get_advance       (Glyph *unicode);
Glyph *kernagic_glyph_new         (const char *path);
void   kernagic_glyph_free        (Glyph *glyph);

/* resets the bearing information of the glyph
 */
void   kernagic_glyph_reset       (Glyph *glyph);

#endif


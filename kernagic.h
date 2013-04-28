#ifndef KERNAGIC_H
#define KERNAGIC_H


#include <stdint.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>

typedef struct _Glyph Glyph;

struct _Glyph {
  char       *path;
  char       *name;
  char       *xml;
  uint32_t    unicode;
  int         advance;
  GHashTable *kerning;

  uint8_t    *raster;
  int         r_width;
  int         r_height;

  int         scan_width[1024]; /* 1024 is a really arbitrary number.. */
 
  int         strip_offset; /* how many units have been subtracted out of glyphs path
                             * coordinates for bearing stripping  */
  float       width;
  float       height;

  float       min_x; /* bounds detected, and subtracted during parse/render */
  float       max_x;
  float       min_y;
  float       max_y;

  cairo_t    *cr; /* used transiently during glyph rendering */
};

void   kernagic_load_ufo           (const char *ufo_path, gboolean strip_left_bearing);
void   load_ufo_glyph              (Glyph *glyph);
float  kernagic_x_height           (void);

Glyph *kernagic_find_glyph_unicode (unsigned int unicode);
Glyph *kernagic_find_glyph         (const char *name);

float  kernagic_kern_get           (Glyph *a, Glyph *b);
void   kernagic_kern_set           (Glyph *a, Glyph *b, float kerning);
void   kernagic_kern_clear_all     (void);

void   kernagic_save_kerning_info  (void);

void   kernagic_lock               (void);
void   kernagic_unlock             (void);

void   kernagic_compute            (GtkProgressBar *progress);
void   kernagic_compute_bearings   (void);

/* setting a glyph string makes kernagic only kern the involved pairs -
 * permitting */
void   kernagic_set_glyph_string   (const char *utf8);

void   rewrite_ufo_glyph           (Glyph *glyph);


#endif

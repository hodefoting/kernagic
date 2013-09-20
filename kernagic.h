#ifndef KERNAGIC_H
#define KERNAGIC_H

#include <stdint.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>

#include "glyph.h"

void   kernagic_load_ufo           (const char *ufo_path, gboolean strip_left_bearing);
void   load_ufo_glyph              (Glyph *glyph);
float  kernagic_x_height           (void);

Glyph *kernagic_find_glyph_unicode (unsigned int unicode);
Glyph *kernagic_find_glyph         (const char *name);

void   kernagic_kern_clear_all     (void);

void   kernagic_save_kerning_info  (void);

void   kernagic_compute            (GtkProgressBar *progress);
void   kernagic_compute_bearings   (void);

/* setting a glyph string makes kernagic only kern the involved pairs -
 * permitting */
void   kernagic_set_glyph_string   (const char *utf8);

void   rewrite_ufo_glyph           (Glyph *glyph);

#define KERNAGIC_DEFAULT_STRIP_LEFT_BEARING  1
extern gboolean kernagic_strip_left_bearing;

gboolean kernagic_deal_with_glyphs (gunichar left, gunichar right);
gboolean kernagic_deal_with_glyph (gunichar unicode);

GList *kernagic_glyphs (void);

typedef struct _KernagicMethod KernagicMethod;
KernagicMethod *kernagic_method_no (int no);
int             kernagic_active_method_no (void);

struct _KernagicMethod {
  char *name;
  void (*init) (void);
  void (*each) (Glyph *glyph, GtkProgressBar *progress);
  void (*done) (void);
};

typedef struct _Word Word;
struct _Word
{
  gchar *utf8;
  int len;
  int x;
  int y;
  int width;
  int height;
};
const char *detect_word (int x, int y);
float n_distance (void);

#define DEBUG_START_Y 160
#define PREVIEW_WIDTH  900
#define PREVIEW_HEIGHT 600

#endif

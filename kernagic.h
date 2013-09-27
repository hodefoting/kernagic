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

/* ipsum generator is a single c function, relies on state from srandom ()  */
char *ipsumat_generate (const char *dict_path,
                        const char *charset,
                        const char *desired_glyphs,
                        int         max_wordlen,
                        int         max_words);

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

extern int debug_start_y;


typedef struct _KernerSettings KernerSettings;

#define KERNER_DEFAULT_MODE          3
#define KERNER_DEFAULT_MIN          15
#define KERNER_DEFAULT_MAX          50
#define KERNER_DEFAULT_DIVISOR      24
#define KERNER_DEFAULT_TARGET_GRAY  50
#define KERNER_DEFAULT_OFFSET        9
#define KERNER_DEFAULT_TRACKING    100

struct _KernerSettings
{
  KernagicMethod *method;
  float minimum_distance;
  float maximum_distance;
  float divisor;
  float alpha_target;
  float offset;
  float tracking;
};
extern KernerSettings kerner_settings;

float kerner_kern (KernerSettings *settings, Glyph *left, Glyph *right);
void init_kerner (void);
void kerner_debug_ui (void);

void kernagic_set_cadence (const char *cadence_path);

void redraw_test_text (const char *intext, const char *ipsum, int ipsum_no, int debuglevel);
int canvas_width ();
int canvas_height ();
extern float debug_scale;


#define WATERFALL_START   0.04
#define WATERFALL_SCALING 1.2
#define WATERFALL_SPACING 0.6
#define WATERFALL_LEVELS    7
#define PREVIEW_PADDING 5

#endif

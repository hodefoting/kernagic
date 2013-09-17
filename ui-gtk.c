#include <gtk/gtk.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "kernagic.h"
#include "kerner.h"

#define PREVIEW_WIDTH  1024
#define PREVIEW_HEIGHT 500

#define MAX_BIG 128

Glyph *g_entries[MAX_BIG];
int x_entries[MAX_BIG];
int big = 0;

extern char *kernagic_sample_text;

static GtkWidget *preview;
static GtkWidget *test_text;
static GtkWidget *spin_method;
static GtkWidget *spin_min_dist;
static GtkWidget *spin_max_dist;
static GtkWidget *spin_gray_target;
static GtkWidget *spin_tracking;
static GtkWidget *spin_offset;
static GtkWidget *spin_rythm;

static GtkWidget *vbox_options_gray;
static GtkWidget *vbox_options_rythm;

static GtkWidget *progress;
//static GtkWidget *strip_bearing_check;
static GtkWidget *toggle_measurement_lines_check;
static GtkWidget *font_path;

/* the preview canvas should be moved out of the gtk code, it is generic
 * code - that also should be used for the png output option
 */
static uint8_t *preview_canvas = NULL;

extern float  scale_factor;

gboolean toggle_measurement_lines = FALSE;

float place_glyph (Glyph *g, float xo, float opacity)
{
  int x, y;

  if (toggle_measurement_lines)
    {
      for (y = 0; y < g->r_height; y++)
        for (x = 0; x < 1; x++)
          if (x + xo >= 0 && x + xo < PREVIEW_WIDTH && y < PREVIEW_HEIGHT &&
          preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] == 0
              )
          preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] = 64;

      for (y = 0; y < PREVIEW_HEIGHT; y++)
        {
          if (g->lstem > 0)
          {
          x = g->lstem * scale_factor + g->left_bearing * scale_factor;
          if (x + xo < PREVIEW_WIDTH)
            preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
          if (g->rstem > 0)
          {
          x = g->rstem * scale_factor + g->left_bearing * scale_factor;
          if (x + xo < PREVIEW_WIDTH)
            preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
        }
    }

  for (y = 0; y < g->r_height; y++)
    for (x = 0; x < g->r_width; x++)
      if (x + xo + g->left_bearing * scale_factor >= 0 && x + xo + g->left_bearing * scale_factor < PREVIEW_WIDTH && y < PREVIEW_HEIGHT &&
          preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo + g->left_bearing * scale_factor)] <
        g->raster[y * g->r_width + x] * opacity
          )
      preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo + g->left_bearing * scale_factor)] =
        g->raster[y * g->r_width + x] * opacity;

#define SCALE_DOWN 10

  /* draw a smaller preview as well */
  for (y = 0; y < g->r_height; y++)
    for (x = 0; x < g->r_width; x++)
      if ((x + xo + g->left_bearing * scale_factor)/SCALE_DOWN >= 0 && (x + xo + g->left_bearing * scale_factor)/SCALE_DOWN < PREVIEW_WIDTH && y/SCALE_DOWN < PREVIEW_HEIGHT)
      {
        int val = preview_canvas [(y/SCALE_DOWN) * PREVIEW_WIDTH + (int)((x + xo + g->left_bearing * scale_factor)/SCALE_DOWN)];
        val += g->raster[y * g->r_width + x] * opacity / SCALE_DOWN / SCALE_DOWN;
        if (val > 255) val = 255;
        preview_canvas [(y/SCALE_DOWN) * PREVIEW_WIDTH + (int)((x + xo + g->left_bearing * scale_factor)/SCALE_DOWN)] = val;
      }

  return xo + kernagic_get_advance (g) * scale_factor;
}

static void redraw_test_text (void)
{
  float period = kerner_settings.alpha_target;
  memset (preview_canvas, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT);
  big = 0;
  {
    const char *utf8;
    gunichar *str2;
    int i;
    utf8 = gtk_entry_get_text (GTK_ENTRY (test_text));
    float x = 0;

    str2 = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);

    if (str2)
    {
      Glyph *prev_g = NULL;
    for (i = 0; str2[i]; i++)
      {
        Glyph *g = kernagic_find_glyph_unicode (str2[i]);
        if (g)
          {
            if (prev_g)
              x += kernagic_kern_get (prev_g, g) * scale_factor;

              g_entries[big] = g;
              x_entries[big++] = x;

            x = place_glyph (g, x, 1.0);
            prev_g = g;
          }
        else if (str2[i] == ' ') /* we're only faking it if we have to  */
          {
            Glyph *t = kernagic_find_glyph_unicode ('i');
            if (t)
              x += kernagic_get_advance (t) * scale_factor;
            prev_g = NULL;
          }
      }
    g_free (str2);
    }
  }

  if (toggle_measurement_lines)
  {
    int i;
    for (i = 0; i * period * scale_factor < PREVIEW_WIDTH - period * scale_factor; i++)
      {
        int y;
        int x = (i + 0.5) * period * scale_factor;
        for (y= PREVIEW_HEIGHT*0.8; y < PREVIEW_HEIGHT*0.85; y++)
          {
            preview_canvas[y* PREVIEW_WIDTH + x] =
              (preview_canvas[y* PREVIEW_WIDTH + x] + 96) / 2;
          }
      }
  }
 
  gtk_widget_queue_draw (preview);
}

static void configure_kernagic (void)
{
  kerner_settings.method =
      kernagic_method_no (gtk_combo_box_get_active (GTK_COMBO_BOX (spin_method)));

  kerner_settings.maximum_distance =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_max_dist));
  kerner_settings.minimum_distance =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_min_dist));
  kerner_settings.alpha_target =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_gray_target));
  kerner_settings.tracking =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_tracking));
  kerner_settings.offset =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_offset));
  kerner_settings.rythm =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_rythm));

  toggle_measurement_lines = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check));
}

static guint delayed_updater = 0;
static gboolean delayed_trigger (gpointer foo)
{
  configure_kernagic ();
  kernagic_set_glyph_string (gtk_entry_get_text (GTK_ENTRY (test_text)));
  kernagic_compute (NULL);
  redraw_test_text ();
  delayed_updater = 0;
  return FALSE;
}

static void trigger (void)
{
  if (!preview_canvas)
    preview_canvas = g_malloc0 (PREVIEW_WIDTH * PREVIEW_HEIGHT);

  if (delayed_updater)
    {
      g_source_remove (delayed_updater);
      delayed_updater = 0;
    }
  delayed_updater = g_timeout_add (100, delayed_trigger, NULL);
}


static guint delayed_reload_updater = 0;
static gboolean delayed_reload_trigger (gpointer foo)
{
  char *ufo_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (font_path));
  //kernagic_strip_left_bearing = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (strip_bearing_check));
  kernagic_strip_left_bearing = TRUE;


  kernagic_load_ufo (ufo_path, kernagic_strip_left_bearing);
  g_free (ufo_path);
  if (delayed_updater)
    {
      g_source_remove (delayed_updater);
      delayed_updater = 0;
    }
  delayed_trigger (foo);
  delayed_reload_updater = 0;
  return FALSE;
}

static void trigger_prop_show (void)
{
  KernagicMethod *method =
      kernagic_method_no (gtk_combo_box_get_active (GTK_COMBO_BOX (spin_method)));

  gtk_widget_hide (vbox_options_gray);
  gtk_widget_hide (vbox_options_rythm);

  if (!strcmp (method->name, "gray"))
    gtk_widget_show (vbox_options_gray);
  else if (!strcmp (method->name, "rythm")||
           !strcmp (method->name, "gap"))
    gtk_widget_show (vbox_options_rythm);
}

static void trigger_reload (void)
{
  if (delayed_reload_updater)
    {
      g_source_remove (delayed_reload_updater);
      delayed_reload_updater = 0;
    }
  delayed_reload_updater = g_timeout_add (500, delayed_reload_trigger, NULL);
}

static void set_defaults (void)
{
  gtk_entry_set_text (GTK_ENTRY (test_text), "Kern Me Tight");
  gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), KERNER_DEFAULT_MODE);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_min_dist),      KERNER_DEFAULT_MIN);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_max_dist),      KERNER_DEFAULT_MAX);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_gray_target),   KERNER_DEFAULT_TARGET_GRAY);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_offset),    KERNER_DEFAULT_OFFSET);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_rythm),         KERNER_DEFAULT_RYTHM);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_tracking),      KERNER_DEFAULT_TRACKING);
  //gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (strip_bearing_check), KERNAGIC_DEFAULT_STRIP_LEFT_BEARING);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check), TRUE);
}

static void set_defaults_from_args (void)
{
  if (kernagic_sample_text)
  gtk_entry_set_text (GTK_ENTRY (test_text), kernagic_sample_text);
  else
  gtk_entry_set_text (GTK_ENTRY (test_text), "Kern Me Tight");
  gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), kernagic_active_method_no());

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_min_dist), kerner_settings.minimum_distance);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_max_dist), kerner_settings.maximum_distance);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_gray_target), kerner_settings.alpha_target);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_offset), kerner_settings.offset);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_rythm), kerner_settings.rythm);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_tracking), kerner_settings.tracking);

  /*
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (strip_bearing_check), kernagic_strip_left_bearing);
  */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check), TRUE);
}

static void do_save (void)
{
  kernagic_save_kerning_info ();
}

static void do_process (void)
{
  kernagic_set_glyph_string (NULL);
  gtk_widget_show (progress);
  kernagic_compute (GTK_PROGRESS_BAR (progress));
  gtk_widget_hide (progress);
  fprintf (stderr, "done kerning!\n");
  kernagic_save_kerning_info ();
}


static gboolean
preview_press_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  int i = 0;
  float x, y;
  Glyph *g;
  float advance;

  x = event->button.x;
  y = event->button.y;
  for (i = 0; i+1 < big && x_entries[i+1] < event->button.x; i++);
  x -= x_entries[i];
  g = g_entries[i];

  x /= scale_factor;
  y /= scale_factor;

  advance = kernagic_get_advance (g);
  printf ("%i %f %f %f\n", i, x, y, advance);

  if (y < 100)
  {
    g->rstem = x - g->left_bearing;
    g->lstem = x - g->left_bearing;
  }
  else
  {

  if (x / advance < 0.5)
  {
    g->lstem = x - g->left_bearing;
  }
  else
  {
    g->rstem = x - g->left_bearing;
  }
  }

  trigger ();
  return TRUE;
}

static gboolean
preview_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  cairo_save (cr);
  cairo_set_source_rgb (cr, 0.93,0.93,0.93);
  cairo_paint (cr);

  if (!preview_canvas)
    {
    }
  else
    {
      cairo_surface_t *surface =
        cairo_image_surface_create_for_data (preview_canvas,
            CAIRO_FORMAT_A8, PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_WIDTH);
      cairo_set_source_rgb (cr, 0,0,0);
      cairo_mask_surface (cr, surface, 0, 0);
      cairo_surface_destroy (surface);
    }
  cairo_restore (cr);
  return FALSE;
}

extern const char *ufo_path;

int ui_gtk (int argc, char **argv)
{
  GtkWidget    *window;
  GtkWidget    *hbox;
  GtkWidget    *vbox1;

  GtkSizeGroup *labels;
  GtkSizeGroup *sliders;

  gtk_init (&argc, &argv);

  labels  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sliders = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_add (GTK_CONTAINER (window), hbox);
  vbox1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (hbox), vbox1);

  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);

  preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (preview, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_container_add (GTK_CONTAINER (hbox), preview);

  g_signal_connect (preview, "draw", G_CALLBACK (preview_draw_cb), NULL);

  g_signal_connect (preview, "button-press-event", G_CALLBACK (preview_press_cb), NULL);
  gtk_widget_add_events (preview, GDK_BUTTON_PRESS_MASK);

#if 1
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Font");
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    font_path = gtk_file_chooser_button_new ("font", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (font_path), ufo_path);
      gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

    gtk_size_group_add_widget (sliders, font_path);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), font_path);
  }
#endif
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Text sample");
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    test_text = gtk_entry_new ();
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    gtk_size_group_add_widget (labels, label);
    gtk_size_group_add_widget (sliders, test_text);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), test_text);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Fitting method");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_method = gtk_combo_box_text_new ();
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    0, "ink bounds");
    //gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
    //                                1, "x-height gray");
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    2,  "rennaisance period table");
    //gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    //3, "rythm");
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    3, "gap");
    gtk_size_group_add_widget (sliders, spin_method);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_method);

  }

  vbox_options_gray = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (vbox1), vbox_options_gray);
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Min distance");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_min_dist = gtk_spin_button_new_with_range (0.00, 100.0, 1);
    gtk_size_group_add_widget (sliders, spin_min_dist);
    gtk_container_add (GTK_CONTAINER (vbox_options_gray), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_min_dist);
  }
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Max distance");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_max_dist = gtk_spin_button_new_with_range (0.00, 100.0, 1);
    gtk_size_group_add_widget (sliders, spin_max_dist);
    gtk_container_add (GTK_CONTAINER (vbox_options_gray), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_max_dist);
  }
  vbox_options_rythm = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (vbox1), vbox_options_rythm);
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Cadence");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_gray_target = gtk_spin_button_new_with_range (0.0, 2000.0, 0.04);
    gtk_size_group_add_widget (sliders, spin_gray_target);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_gray_target);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Offset");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_offset = gtk_spin_button_new_with_range (-50.0, 600.0, 0.01);
    gtk_size_group_add_widget (sliders, spin_offset);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_offset);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Rythm");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_rythm = gtk_spin_button_new_with_range (1.0, 10.0, 1.0);
    gtk_size_group_add_widget (sliders, spin_rythm);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_rythm);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Tracking");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_tracking = gtk_spin_button_new_with_range (0.0, 300.0, 0.5);
    gtk_size_group_add_widget (sliders, spin_tracking);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_tracking);
  }
#if 0
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Gray strength");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_gray_strength = gtk_spin_button_new_with_range (0.0, 100.0, 1);
    gtk_size_group_add_widget (sliders, spin_gray_strength);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_gray_strength);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Area target");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_area_target = gtk_spin_button_new_with_range (0.0, 100, 1);
    gtk_size_group_add_widget (sliders, spin_area_target);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_area_target);
  }

  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Area strength");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_area_strength = gtk_spin_button_new_with_range (0.0, 100.0, 1);
    gtk_size_group_add_widget (sliders, spin_area_strength);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_area_strength);
  }
#endif

#if 0
  {
    strip_bearing_check = gtk_check_button_new_with_label ("Strip bearings");
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_size_group_add_widget (sliders, strip_bearing_check);
    gtk_box_pack_end (GTK_BOX (hbox), strip_bearing_check, FALSE, TRUE, 2);
  }
#endif
  {
    toggle_measurement_lines_check = gtk_check_button_new_with_label ("Measurement lines");
    //GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), toggle_measurement_lines_check);
    //gtk_size_group_add_widget (sliders, toggle_measurement_lines_check);
    //gtk_box_pack_end (GTK_BOX (hbox), toggle_measurement_lines_check, FALSE, TRUE, 2);
  }
#if 0
  {
    GtkWidget *label = gtk_check_button_new_with_label ("Generate left bearing");
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_size_group_add_widget (sliders, label);
    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, TRUE, 2);
  }
  {
    GtkWidget *label = gtk_check_button_new_with_label ("Simulate no kerning");
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_size_group_add_widget (sliders, label);
    gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, TRUE, 2);
  }
#endif

  {
    GtkWidget *hbox;
    GtkWidget *defaults_button = gtk_button_new_with_label ("Reset to defaults");
    GtkWidget *process_button  = gtk_button_new_with_label ("Process");
    GtkWidget *save_button     = gtk_button_new_with_label ("Save (modifies UFO in place)");

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_box_pack_start (GTK_BOX (hbox), defaults_button, TRUE, TRUE, 2);
    gtk_box_pack_start (GTK_BOX (hbox), process_button, TRUE, TRUE, 2);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_box_pack_start (GTK_BOX (hbox), save_button, TRUE, TRUE, 2);

    g_signal_connect (defaults_button,"clicked", G_CALLBACK (set_defaults), NULL);
    g_signal_connect (process_button, "clicked", G_CALLBACK (do_process), NULL);
    g_signal_connect (save_button,  "clicked",  G_CALLBACK (do_save), NULL);
  }

  {
    progress = gtk_progress_bar_new ();
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), TRUE);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.112);
    gtk_container_add (GTK_CONTAINER (vbox1), progress);
  }

  /* when these change, we need to reinitialize from scratch */
#if 0
  g_signal_connect (strip_bearing_check, "toggled",       G_CALLBACK (trigger_reload), NULL);
#endif
  g_signal_connect (font_path,           "file-set",      G_CALLBACK (trigger_reload), NULL);
  /* and when these change, we should be able to do an incremental update */
  g_signal_connect (toggle_measurement_lines_check, "toggled",   G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_method,        "notify::active", G_CALLBACK (trigger), NULL);

  g_signal_connect (spin_method,        "notify::active", G_CALLBACK (trigger_prop_show), NULL);
  g_signal_connect (spin_min_dist,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_max_dist,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_gray_target,   "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_tracking,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_offset,        "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_rythm,         "notify::value", G_CALLBACK (trigger), NULL);
#if 0
  g_signal_connect (spin_area_target,   "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_gray_strength, "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_area_strength, "notify::value", G_CALLBACK (trigger), NULL);
#endif
  g_signal_connect (test_text,          "notify::text",  G_CALLBACK (trigger), NULL);


  set_defaults_from_args ();

  gtk_widget_show_all (hbox);
  gtk_widget_hide (progress);
  gtk_widget_show (window);
  trigger_prop_show ();

  if (getenv ("KERNAGIC_DEBUG"))
    kerner_debug_ui ();

  trigger_reload ();
  gtk_main ();
  return 0;
}

#include <gtk/gtk.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "kernagic.h"

#define PREVIEW_WIDTH  1024
#define PREVIEW_HEIGHT 256

static GtkWidget *preview;
static GtkWidget *test_text;
static GtkWidget *spin_min_dist;
static GtkWidget *spin_max_dist;
static GtkWidget *spin_gray_target;
static GtkWidget *progress;
static GtkWidget *strip_left_bearing_check;
static GtkWidget *font_path;

static uint8_t *preview_canvas = NULL;

extern float  scale_factor;

float place_glyph (Glyph *g, float xo, float opacity)
{
  int x, y;

#if SHOW_LEFT_BOUNDS
  for (y = 0; y < g->r_height; y++)
    for (x = 0; x < 1; x++)
      if (x + xo >= 0 && x + xo < PREVIEW_WIDTH && y < PREVIEW_HEIGHT &&
      preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] == 0
          )
      preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] = 64;
#endif

  for (y = 0; y < g->r_height; y++)
    for (x = 0; x < g->r_width; x++)
      if (x + xo >= 0 && x + xo < PREVIEW_WIDTH && y < PREVIEW_HEIGHT &&
      preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] == 0
          )
      preview_canvas [y * PREVIEW_WIDTH + (int)(x + xo)] = g->raster[y * g->r_width + x] * opacity;

  return xo + g->advance * scale_factor;
}

static void redraw_test_text (void)
{
  memset (preview_canvas, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT);
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
            x = place_glyph (g, x, 1.0);
            prev_g = g;
          }
        else if (str2[i] == ' ')
          {
            x += kernagic_x_height () * 0.4 * scale_factor;
            prev_g = NULL;
          }
      }
    g_free (str2);
    }
  }
 
  gtk_widget_queue_draw (preview);
}

extern float kernagic_min_dist;
extern float kernagic_max_dist;
extern float kernagic_gray_target;


static void configure_kernagic (void)
{
  kernagic_max_dist    = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_max_dist));
  kernagic_min_dist    = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_min_dist));
  kernagic_gray_target = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_gray_target));
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
    preview_canvas = calloc (PREVIEW_WIDTH * PREVIEW_HEIGHT, 1);

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

  kernagic_load_ufo (ufo_path);
  free (ufo_path);
  if (delayed_updater)
    {
      g_source_remove (delayed_updater);
      delayed_updater = 0;
    }
  delayed_trigger (foo);
  delayed_reload_updater = 0;
  return FALSE;
}
static void trigger_reload (void)
{
  if (delayed_reload_updater)
    {
      g_source_remove (delayed_reload_updater);
      delayed_reload_updater = 0;
    }
  delayed_reload_updater = g_timeout_add (100, delayed_reload_trigger, NULL);
}

static void set_defaults (void)
{
  gtk_entry_set_text (GTK_ENTRY (test_text), "Kern Me Tight");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_gray_target), 40);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_min_dist),    0.1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_max_dist),    0.3);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (strip_left_bearing_check), TRUE);
}

static void do_save (void)
{
  kernagic_save_kerning_info ();
}

static void do_process (void)
{
  configure_kernagic ();
  kernagic_set_glyph_string (NULL);
  gtk_widget_show (progress);
  kernagic_compute (GTK_PROGRESS_BAR (progress));
  gtk_widget_hide (progress);
  fprintf (stderr, "done kerning!\n");
  kernagic_save_kerning_info ();
}

static gboolean
preview_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
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
      cairo_fill (cr);
      cairo_surface_destroy (surface);
    }
  return FALSE;
}

int main (int argc, char **argv)
{
  const char *ufo_path = argv[1]?argv[1]:"./LucidaSans2.ufo";

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

#if 1
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Font");
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    font_path = gtk_file_chooser_button_new ("font", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (font_path), ufo_path);
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
    GtkWidget *label = gtk_label_new ("Min distance");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_min_dist = gtk_spin_button_new_with_range (0.00, 1.0, 0.01);
    gtk_size_group_add_widget (sliders, spin_min_dist);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_min_dist);
  }
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Max distance");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_max_dist = gtk_spin_button_new_with_range (0.00, 1.0, 0.01);
    gtk_size_group_add_widget (sliders, spin_max_dist);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_max_dist);
  }
  {
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *label = gtk_label_new ("Gray target");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_gray_target = gtk_spin_button_new_with_range (0.0, 100, 1);
    gtk_size_group_add_widget (sliders, spin_gray_target);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_gray_target);
  }
  {
    strip_left_bearing_check = gtk_check_button_new_with_label ("Strip left bearing");
    GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_container_add (GTK_CONTAINER (vbox1), hbox);
    gtk_size_group_add_widget (sliders, strip_left_bearing_check);
    gtk_box_pack_end (GTK_BOX (hbox), strip_left_bearing_check, FALSE, TRUE, 2);
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
    GtkWidget *save_button     = gtk_button_new_with_label ("Save Kerning");

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
  g_signal_connect (strip_left_bearing_check, "toggled",       G_CALLBACK (trigger_reload), NULL);
  g_signal_connect (font_path,                "file-set",      G_CALLBACK (trigger_reload), NULL);
  /* and when these change, we should be able to do an incremental update */
  g_signal_connect (spin_min_dist,            "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_max_dist,            "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_gray_target,         "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (test_text,                "notify::text",  G_CALLBACK (trigger), NULL);

  set_defaults ();

  gtk_widget_show_all (hbox);
  gtk_widget_hide (progress);
  gtk_widget_show (window);

  //process_debug_ui ();

  gtk_main ();
  return 0;
}

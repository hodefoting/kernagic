#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "kernagic.h"

static char *ipsum = NULL;

void redraw_test_text (const char *intext, const char *ipsum, int ipsum_no, int debuglevel);

#define INDEX_WIDTH    256
#define INDEX_HEIGHT   256

#define GTK2 1
#define HILBERTCODE 1

#ifdef GTK2
#else
#define gtk_hbox_new(a,n)  gtk_box_new (GTK_ORIENTATION_HORIZONTAL, n)
#define gtk_vbox_new(a,n)  gtk_box_new (GTK_ORIENTATION_VERTICAL, n)
#endif

extern Glyph     *g_entries[];
extern int        x_entries[];
extern int        big;
extern gboolean   toggle_measurement_lines;
extern float      scale_factor;
extern char      *kernagic_sample_text;
extern uint8_t   *kernagic_preview;
static GtkWidget *preview;
static GtkWidget *index = NULL;
static GtkWidget *test_text;
static GtkWidget *ipsum_glyphs;
static GtkWidget *spin_method;
static GtkWidget *spin_min_dist;
static GtkWidget *spin_max_dist;
static GtkWidget *spin_gray_target;
static GtkWidget *spin_divisor;
static GtkWidget *spin_tracking;
static GtkWidget *spin_offset;
GtkWidget *spin_ipsum_no;


static GtkWidget *vbox_options_cadence;
static GtkWidget *cadence_path;

static GtkWidget *vbox_options_gray;
static GtkWidget *vbox_options_rythm;

static GtkWidget *progress;
static GtkWidget *toggle_measurement_lines_check;
static GtkWidget *font_path;
static GtkWidget *ipsum_path;
static uint8_t *index_canvas = NULL;


static void configure_kernagic (void)
{
  kerner_settings.method =
      kernagic_method_no (gtk_combo_box_get_active (GTK_COMBO_BOX (spin_method)));

  kerner_settings.maximum_distance =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_max_dist));
  kerner_settings.minimum_distance =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_min_dist));
  kerner_settings.alpha_target =
       gtk_range_get_value (GTK_RANGE (spin_gray_target));
  kerner_settings.divisor =
       gtk_range_get_value (GTK_RANGE (spin_divisor));
  kerner_settings.tracking =
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_tracking));
  kerner_settings.offset =
       gtk_range_get_value (GTK_RANGE (spin_offset));

  toggle_measurement_lines = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check));
}

static guint delayed_updater = 0;
static gboolean delayed_trigger (gpointer foo)
{
  GString *str = g_string_new ("");
  configure_kernagic ();

  g_string_append (str, gtk_entry_get_text (GTK_ENTRY (test_text)));
  if (ipsum)
    g_string_append (str, ipsum);

  kernagic_set_glyph_string (str->str);
  g_string_free (str, TRUE);
  kernagic_compute (NULL);

  redraw_test_text ( gtk_entry_get_text (GTK_ENTRY (test_text)), ipsum, 
      
       gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_ipsum_no))
      ,toggle_measurement_lines);

  gtk_widget_queue_draw (preview);

  if (index)
    gtk_widget_queue_draw (index);

  delayed_updater = 0;
  return FALSE;
}

static void trigger (void)
{

  if (delayed_updater)
    {
      g_source_remove (delayed_updater);
      delayed_updater = 0;
    }
  delayed_updater = g_timeout_add (100, delayed_trigger, NULL);
}

static int frozen = 0;

static void trigger_divisor (void)
{
  float divisor = gtk_range_get_value (GTK_RANGE (spin_divisor));
  if (frozen)
    return;

  frozen++;
  gtk_range_set_value (GTK_RANGE (spin_gray_target),
      n_distance () / divisor);
  frozen--;
}

static void trigger_cadence_path (void)
{
  kernagic_set_cadence (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (cadence_path)));
  trigger ();
}

static void trigger_cadence (void)
{
  float cadence = gtk_range_get_value (GTK_RANGE (spin_gray_target));
  //if (frozen)
  //  return;

  frozen++;
  gtk_range_set_value (GTK_RANGE (spin_divisor),
      n_distance () / cadence);
  frozen--;

  trigger ();
}

static void trigger_ipsum (void)
{
  GString *str = g_string_new ("");
  GList *l, *list = kernagic_glyphs ();
  for (l = list; l; l = l->next)
    {
      Glyph *glyph = l->data;
      if (glyph->unicode)
        g_string_append_unichar (str, glyph->unicode);
    }
  if (!list)
    g_string_append (str, "abcdefghijklmnopqrstuvxyz");
  if (ipsum)
    g_free (ipsum);
  ipsum = g_strdup (ipsumat_generate (NULL, str->str,
        gtk_entry_get_text (GTK_ENTRY (ipsum_glyphs)), 7, 13));
  trigger ();
  g_string_free (str, TRUE);
}


static guint delayed_reload_updater = 0;
static gboolean delayed_reload_trigger (gpointer foo)
{
  char *ufo_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (font_path));
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

  gtk_range_set_value (GTK_RANGE (spin_divisor),   KERNER_DEFAULT_DIVISOR+1);
  gtk_range_set_value (GTK_RANGE (spin_divisor),   KERNER_DEFAULT_DIVISOR);

  return FALSE;
}

static void trigger_prop_show (void)
{
  KernagicMethod *method =
      kernagic_method_no (gtk_combo_box_get_active (GTK_COMBO_BOX (spin_method)));

  gtk_widget_hide (vbox_options_cadence);
  gtk_widget_hide (vbox_options_gray);
  gtk_widget_hide (vbox_options_rythm);

  if (!strcmp (method->name, "cadence"))
    gtk_widget_show (vbox_options_cadence);
  else if (!strcmp (method->name, "gray"))
    gtk_widget_show (vbox_options_gray);
  else if (!strcmp (method->name, "rythm")||
           !strcmp (method->name, "gap"))
    gtk_widget_show (vbox_options_rythm);
}

static void ipsum_reload (void)
{
  if (ipsum)
    g_free (ipsum);
  g_file_get_contents (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (ipsum_path)),
      &ipsum, NULL, NULL);
  trigger ();

}

static void trigger_reload (void)
{
  if (delayed_reload_updater)
    {
      g_source_remove (delayed_reload_updater);
      delayed_reload_updater = 0;
    }
  delayed_reload_updater = g_timeout_add (250, delayed_reload_trigger, NULL);
}

static void set_defaults (void)
{
  gtk_entry_set_text (GTK_ENTRY (test_text), "Kern Me Tight");
  gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), KERNER_DEFAULT_MODE);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ipsum_no),      1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_min_dist),      KERNER_DEFAULT_MIN);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_max_dist),      KERNER_DEFAULT_MAX);
  gtk_range_set_value (GTK_RANGE (spin_gray_target),   KERNER_DEFAULT_TARGET_GRAY);
  gtk_range_set_value (GTK_RANGE (spin_divisor),   KERNER_DEFAULT_DIVISOR);
  gtk_range_set_value (GTK_RANGE (spin_offset),    KERNER_DEFAULT_OFFSET);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_tracking),      KERNER_DEFAULT_TRACKING);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check), TRUE);
}

static void set_defaults_from_args (void)
{
  if (kernagic_sample_text)
  gtk_entry_set_text (GTK_ENTRY (test_text), kernagic_sample_text);
  else
  gtk_entry_set_text (GTK_ENTRY (test_text), "Kern Me Tight");
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ipsum_no),      1);

  gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), kernagic_active_method_no()); 

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_min_dist), kerner_settings.minimum_distance);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_max_dist), kerner_settings.maximum_distance);

  gtk_range_set_value (GTK_RANGE (spin_gray_target), kerner_settings.alpha_target);

  gtk_range_set_value (GTK_RANGE (spin_divisor), kerner_settings.divisor);
  gtk_range_set_value (GTK_RANGE (spin_offset), kerner_settings.offset);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_tracking), kerner_settings.tracking);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle_measurement_lines_check), TRUE);
}

static void do_process (void)
{
  kernagic_set_glyph_string (NULL);
  gtk_widget_show (progress);
  kernagic_compute (GTK_PROGRESS_BAR (progress));
  gtk_widget_hide (progress);
  kernagic_save_kerning_info ();
}

static void do_save (void)
{
  do_process ();
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
  
  if (i + 1 >= big)
    return TRUE;

  x -= x_entries[i];
  g = g_entries[i];

  x /= scale_factor;
  y /= scale_factor;

  if (!g)
    return TRUE;
  advance = kernagic_get_advance (g);

  /* should be adjusted according to an y-offset */

  /* for lowest y coords- do word picking from background ipsum,
   * for detailed adjustments
   */
  {
    const char *word;
    /* it is ugly to have to do this */
    word = detect_word (event->button.x, event->button.y);
    if (word)
    {
      gtk_entry_set_text (GTK_ENTRY (test_text), word);
      trigger ();
      return TRUE;
    }
  }

  if (y < 0)
  {}
  else if (y < kernagic_x_height () * 1.5)
  {
    g->rstem = x - g->left_bearing;
    g->lstem = x - g->left_bearing;
  }
  else if (y > kernagic_x_height () * 2.5)
  {
    g->rstem = 0;
    g->lstem = 0;
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


#ifdef GTK2
static gboolean
preview_draw_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
static gboolean
preview_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
#endif
  cairo_save (cr);
  cairo_set_source_rgb (cr, 0.93,0.93,0.93);
  cairo_paint (cr);

  if (!kernagic_preview)
    {
    }
  else
    {
      cairo_surface_t *surface =
        cairo_image_surface_create_for_data (kernagic_preview,
            CAIRO_FORMAT_A8, canvas_width (), canvas_height (), canvas_width ());
      cairo_set_source_rgb (cr, 0,0,0);
      cairo_mask_surface (cr, surface, 0, 0);
      cairo_surface_destroy (surface);
    }
  cairo_restore (cr);
#ifdef GTK2
  cairo_destroy (cr);
#endif
  return FALSE;
}


/* hilbert curve functions from wikipedia */
static void rot(int n, int *x, int *y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n-1 - *x;
            *y = n-1 - *y;
        }

        //Swap x and y
        int t  = *x;
        *x = *y;
        *y = t;
    }
}


static void
d2xy (int n, int d, int *x, int *y)
{
  int rx, ry, s, t=d;
  *x = *y = 0;
  for (s=1; s<n; s*=2)
    {
      rx = 1 & (t/2);
      ry = 1 & (t ^ rx);
      rot(s, x, y, rx, ry);
      *x += s * rx;
      *y += s * ry;
      t /= 4;
    }
}

//convert (x,y) to d
static int
xy2d (int n, int x, int y)
{
  int rx, ry, s, d=0;
  for (s=n/2; s>0; s/=2)
    {
      rx = (x & s) > 0;
      ry = (y & s) > 0;
      d += s * s * ((3 * rx) ^ ry);
      rot(s, &x, &y, rx, ry);
    }
  return d;
}

gboolean
index_press_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GString *str = g_string_new ("");
  float x, y;
  int i;
  int unicode;
  x = event->button.x;
  y = event->button.y;
  unicode = xy2d (256, x, y);
  //sprintf (buf, "%f %f %i", x, y, unicode);

  GList *list, *l;
  list = kernagic_glyphs ();

  for (l = list; l->next; l = l->next)
    {
      Glyph *glyph = l->data;
      if (glyph->unicode >= unicode)
        {
          break;
        }
    }
  list = l;

  for (i = 0, l = list;l->prev && i < 1; i++, l = l->prev);
  for (i = 0; l && i < 3; i++, l = l->next)
    {
      Glyph *glyph = l->data;
      g_string_append_unichar (str, glyph->unicode);
    }

  gtk_entry_set_text (GTK_ENTRY (test_text), str->str);
  g_string_free (str, TRUE);
  return TRUE;
}

#ifdef GTK2
gboolean
index_draw_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr = gdk_cairo_create (widget->window);
#else
static gboolean
index_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
#endif
  memset (index_canvas, 0, INDEX_WIDTH * INDEX_HEIGHT);
  GList *list, *l;
  list = kernagic_glyphs ();

  for (l = list; l; l = l->next)
    {
      Glyph *glyph = l->data;
      int unicode = glyph->unicode;
      int x, y;
      d2xy (256, unicode, &x, &y);

      if (x <0) x = 0;
      if (y <0) y = 0;
      if (x >= INDEX_WIDTH) x= INDEX_WIDTH-1;
      if (y >= INDEX_HEIGHT) y= INDEX_WIDTH-1;
      index_canvas [y * INDEX_WIDTH + x] = 255;
    }

  cairo_save (cr);
  /* 0.93 is gtk default gray :d */
  cairo_set_source_rgb (cr, 0.93,0.93,0.93);
  cairo_paint (cr);
  {
    cairo_surface_t *surface =
      cairo_image_surface_create_for_data (index_canvas,
          CAIRO_FORMAT_A8, INDEX_WIDTH, INDEX_HEIGHT, INDEX_WIDTH);
    cairo_set_source_rgb (cr, 0,0,0);
    cairo_mask_surface (cr, surface, 0, 0);
    cairo_surface_destroy (surface);
  }
  cairo_restore (cr);
#ifdef GTK2
  cairo_destroy (cr);
#endif
  return FALSE;
}

extern const char *ufo_path;

static gboolean
kernagic_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  switch (event->keyval)
  {
    case GDK_Page_Up:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ipsum_no),  
        gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_ipsum_no)) - 1);
      return TRUE;
    case GDK_Page_Down:
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_ipsum_no),  
        gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_ipsum_no)) + 1);
      return TRUE;
    case GDK_F1:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 0); break;
    case GDK_F2:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 1); break;
    case GDK_F3:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 2); break;
    case GDK_F4:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 3); break;
    case GDK_F5:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 4); break;
    case GDK_F6:
      gtk_combo_box_set_active (GTK_COMBO_BOX (spin_method), 5); break;
    case GDK_s:
    case GDK_S:
      if (event->state & GDK_CONTROL_MASK)
      {
        do_save ();
        return TRUE;
      }
      break;
    case GDK_q:
    case GDK_Q:
      if (event->state & GDK_CONTROL_MASK)
      {
        gtk_main_quit ();
        return TRUE;
      }
      break;
    default:
      break;
  }
  return FALSE; 
}

int ui_gtk (int argc, char **argv)
{
  GtkWidget    *window;
  GtkWidget    *hbox;
  GtkWidget    *vbox2;
  GtkWidget    *vbox1;

  GtkSizeGroup *labels;
  GtkSizeGroup *sliders;

  if (!index_canvas)
    index_canvas = g_malloc0 (INDEX_WIDTH * INDEX_HEIGHT);


  gtk_init (&argc, &argv);

  labels  = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  sliders = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (kernagic_key_press), NULL);


  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (window), hbox);
  vbox1 = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), vbox1, FALSE, FALSE, 2);

  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);

  preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (preview, canvas_width(), canvas_height());

  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_end (GTK_BOX (vbox2), preview, TRUE, TRUE, 2);
  gtk_container_add (GTK_CONTAINER (hbox), vbox2);

#ifdef GTK2
  g_signal_connect (preview, "expose-event", G_CALLBACK (preview_draw_cb), NULL);
#else
  g_signal_connect (preview, "draw", G_CALLBACK (preview_draw_cb), NULL);
#endif

  g_signal_connect (preview, "button-press-event", G_CALLBACK (preview_press_cb), NULL);
  gtk_widget_add_events (preview, GDK_BUTTON_PRESS_MASK);

  {
    GtkWidget *hbox;
    /* XXX: add a button to remove all custom stems? */
    GtkWidget *defaults_button = gtk_button_new_with_label ("defaults");
    //GtkWidget *process_button  = gtk_button_new_with_label ("Respace all");
    GtkWidget *save_button     = gtk_button_new_with_label ("Save");

    gtk_widget_set_tooltip_text (save_button, "Ctrl + S");

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (hbox), defaults_button, TRUE, TRUE, 2);
    //gtk_box_pack_start (GTK_BOX (hbox), process_button, TRUE, TRUE, 2);

    gtk_box_pack_start (GTK_BOX (hbox), save_button, TRUE, TRUE, 2);

    g_signal_connect (defaults_button,"clicked", G_CALLBACK (set_defaults), NULL);
    //g_signal_connect (process_button, "clicked", G_CALLBACK (do_process), NULL);
    g_signal_connect (save_button,  "clicked",  G_CALLBACK (do_save), NULL);
  }
  {
    toggle_measurement_lines_check = gtk_check_button_new_with_label ("Measurement lines");
    gtk_box_pack_start (GTK_BOX (vbox1), toggle_measurement_lines_check, FALSE, FALSE, 2);
  }


  {
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
    GtkWidget *label = gtk_label_new ("Font");
    gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);

    font_path = gtk_file_chooser_button_new ("font", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    if (ufo_path)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (font_path), ufo_path);
    }
      gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

    gtk_size_group_add_widget (sliders, font_path);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), font_path);
  }

  {
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
    GtkWidget *label = gtk_label_new ("Ipsum");
    gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);
    GtkWidget *hbox2 = gtk_hbox_new (FALSE, 4);

    ipsum_path = gtk_file_chooser_button_new ("ipsum", GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

    spin_ipsum_no = gtk_spin_button_new_with_range (0, 100, 1);
    gtk_widget_set_tooltip_text (spin_ipsum_no, "PgUp / PgDn");
    gtk_container_add (GTK_CONTAINER (hbox2), ipsum_path);
    gtk_container_add (GTK_CONTAINER (hbox2), spin_ipsum_no);
    gtk_size_group_add_widget (sliders, hbox2);

    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), hbox2);
  }
  {
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
    GtkWidget *label = gtk_label_new ("Ipsum glyphs");
    gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 2);
    ipsum_glyphs = gtk_entry_new ();
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    gtk_size_group_add_widget (labels, label);
    gtk_size_group_add_widget (sliders, ipsum_glyphs);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), ipsum_glyphs);
  }

  {
    //GtkWidget *label = gtk_label_new ("Text sample");
    test_text = gtk_entry_new ();
#if 0
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    gtk_size_group_add_widget (labels, label);
    gtk_size_group_add_widget (sliders, test_text);
    gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox1), test_text, FALSE, FALSE, 2);
    gtk_container_add (GTK_CONTAINER (hbox), test_text);
#endif

    gtk_box_pack_start (GTK_BOX (vbox2), test_text, FALSE, FALSE, 2);
  }
  {
    spin_method = gtk_combo_box_text_new ();
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    0, "original");
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    1, "ink bounds");
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    2, "bearing table");
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (spin_method),
                                    3, "en-divisor snapped stem gap");
    gtk_widget_set_tooltip_text (spin_method, "F1, F2, F3…");
    gtk_box_pack_start (GTK_BOX (vbox1), spin_method, FALSE, FALSE, 2);
  }

  vbox_options_cadence = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox_options_cadence, FALSE, FALSE, 2);

  {
    GtkWidget *label = gtk_label_new ("cadence table file");
    cadence_path = gtk_file_chooser_button_new ("cadence file", GTK_FILE_CHOOSER_ACTION_OPEN);
    if (ufo_path)
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (font_path), ufo_path);
    }
      gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);

    gtk_size_group_add_widget (sliders, cadence_path);
    gtk_box_pack_start (GTK_BOX (vbox_options_cadence), label, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox_options_cadence), cadence_path, FALSE, FALSE, 2);
  }

  vbox_options_gray = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox_options_gray, FALSE, FALSE, 2);
  {
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
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
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
    GtkWidget *label = gtk_label_new ("Max distance");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_max_dist = gtk_spin_button_new_with_range (0.00, 100.0, 1);
    gtk_size_group_add_widget (sliders, spin_max_dist);
    gtk_container_add (GTK_CONTAINER (vbox_options_gray), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_max_dist);
  }
  vbox_options_rythm = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox_options_rythm, FALSE, FALSE, 2);

  {
    GtkWidget *label = gtk_label_new ("Divisor");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_divisor = gtk_hscale_new_with_range (0.0, 100.0, 1);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), label);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), spin_divisor);
  }

  {
    GtkWidget *label = gtk_label_new ("Cadence");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_gray_target = gtk_hscale_new_with_range (1, 100.0, 0.01);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), label);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), spin_gray_target);
  }

  {
    GtkWidget *label = gtk_label_new ("Offset");
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_offset = gtk_hscale_new_with_range (-5.0, 100.0, 0.1);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), label);
    gtk_container_add (GTK_CONTAINER (vbox_options_rythm), spin_offset);
  }
  {
    GtkWidget *hbox = gtk_hbox_new (FALSE, 4);
    GtkWidget *label = gtk_label_new ("Tracking");
    gtk_size_group_add_widget (labels, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
    spin_tracking = gtk_spin_button_new_with_range (0.0, 300.0, 0.5);
    gtk_size_group_add_widget (sliders, spin_tracking);
    //gtk_container_add (GTK_CONTAINER (vbox_options_rythm), hbox);
    gtk_container_add (GTK_CONTAINER (hbox), label);
    gtk_container_add (GTK_CONTAINER (hbox), spin_tracking);
  }

  {
    progress = gtk_progress_bar_new ();
#if 0
    gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (progress), TRUE);
#endif
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.112);
    gtk_box_pack_start (GTK_BOX (vbox1), progress, FALSE, FALSE, 2);
  }

#if 1
  index = gtk_drawing_area_new ();
  gtk_widget_set_size_request (index, INDEX_WIDTH, INDEX_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox1), index, FALSE, FALSE, 2);

  //g_signal_connect (index, "draw", G_CALLBACK (index_draw_cb), NULL);
  g_signal_connect (index, "expose-event", G_CALLBACK (index_draw_cb), NULL);

#ifdef GTK2
  g_signal_connect (index, "expose-event", G_CALLBACK (index_draw_cb), NULL);
#else
  g_signal_connect (index, "draw", G_CALLBACK (index_draw_cb), NULL);
#endif

  g_signal_connect (index, "button-press-event", G_CALLBACK (index_press_cb), NULL);
  gtk_widget_add_events (index, GDK_BUTTON_PRESS_MASK);
#endif


  {
    GtkWidget *help = gtk_label_new ("Click a small ipsum word to replace the word being worked on,\nclicking below the baseline removes custom stems, within the x-height left and right stems are set, above the x-height single stem overrides are set.\n\nThe short lines are stem-lines, weak ones auto-detected and dark ones manual overrides.");
    gtk_label_set_line_wrap (GTK_LABEL (help), TRUE);
    gtk_misc_set_alignment (GTK_MISC (help), 0.0, 0.0);
    gtk_box_pack_start (GTK_BOX (vbox1), help, FALSE, FALSE, 2);
  }

  /************/

  /* when these change, we need to reinitialize from scratch */
  g_signal_connect (cadence_path,       "file-set",      G_CALLBACK (trigger_cadence_path), NULL);
  g_signal_connect (font_path,          "file-set",      G_CALLBACK (trigger_reload), NULL);
  g_signal_connect (ipsum_path,         "file-set",      G_CALLBACK (ipsum_reload), NULL);
  /* and when these change, we should be able to do an incremental update */
  g_signal_connect (toggle_measurement_lines_check, "toggled",   G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_method,        "notify::active", G_CALLBACK (trigger), NULL);

  g_signal_connect (spin_method,        "notify::active", G_CALLBACK (trigger_prop_show), NULL);
  g_signal_connect (spin_ipsum_no,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_min_dist,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_max_dist,      "notify::value", G_CALLBACK (trigger), NULL);
  g_signal_connect (spin_gray_target,   "value-changed", G_CALLBACK (trigger_cadence), NULL);
  g_signal_connect (spin_divisor,       "value-changed", G_CALLBACK (trigger_divisor), NULL);
  g_signal_connect (spin_offset,        "value-changed", G_CALLBACK (trigger), NULL);
  g_signal_connect (test_text,          "notify::text",  G_CALLBACK (trigger), NULL);

  g_signal_connect (ipsum_glyphs,          "notify::text",  G_CALLBACK (trigger_ipsum), NULL);

  set_defaults_from_args ();

  gtk_widget_show_all (hbox);
  gtk_widget_hide (progress);
  gtk_widget_show (window);
  trigger_prop_show ();

  if (getenv ("KERNAGIC_DEBUG"))
    kerner_debug_ui ();

  trigger_reload ();

  ipsum = g_strdup ("the five boxing wizards jump quickly\n"
      "onoo nnin ilnum\n"
      "lodger jets keller viewed breast catty magi eskimo sudden allay sitars glues snub quail myself criers parole hunts sync bulky nissan recede freud sculpt numbs arias doyens vector liming polyp yamaha boodle legree zigzag lusty babies unwind ginkgo\n"
      "shrub wisdom cipher causal sigh grubby ate gaffs hutch mapped levitt duluth peeks hyping halos shoed bog enable copra distil storm zuni besets trump quaver modem reap murals kurtis fazing pursed vaguer aisle tilt began gentry effect convoy crowds\n"
      "illegally roach unlimited variable costello downscale cloak walton radius rojas lemony nuke theses tipped skids sullen stael hopi toward highly croak purina croup rector pantie yeahs irks scuds egoism queen sags deity fenced taegu adams somali flaws swords dwarfs webern fronde cayuga pilaf roving verb hotel needy\n"
      "abcdefghijklmnopqrstuvwxyz\n"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
      "0123456789 -=+_ ,.:;'\" ()[]{} *&^%$#?@! <>~`/|\\ \n"
      "kernagic is free software distributed under the AGPL\n"
      "Øyvind Kolås pippin@gimp.org\n");

  gtk_main ();
  return 0;
}

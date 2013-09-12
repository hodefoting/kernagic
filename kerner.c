                                                                            /*
Kernagic a libre auto spacer and kerner for Unified Font Objects.
Copyright (C) 2013 Øyvind Kolås

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.       */

#include <string.h>
#include <math.h>
#include <assert.h>
#include "kernagic.h"
#include "kerner.h"

extern float scale_factor;

KernerSettings kerner_settings = {
  KERNER_DEFAULT_MODE,
  KERNER_DEFAULT_MIN,
  KERNER_DEFAULT_MAX,
  KERNER_DEFAULT_TARGET_GRAY,
  KERNER_DEFAULT_MULTIPLIER,
  KERNER_DEFAULT_TRACKING
};

static float alpha = 0.23;
static float beta  = 0.23;
static gboolean visual_debug_enabled = FALSE;

#define DIST_MAX  120

static int      s_width  = 0;
static int      s_height = 0;
static uint8_t *scratch  = NULL;
static uint8_t *scratch2 = NULL;
static uint8_t *scratch3 = NULL;
static long int scratch3_unicode  = -1;

static void place_a (Glyph *left, Glyph *right, float opacity);

static void save_scratch (void)
{
  memcpy (scratch2, scratch, s_width * s_height);
}

static int  compute_dist (Glyph *left, Glyph *right, int space)
{
  int x, y;
  int min_dist = 2000;

  if (scratch3_unicode == left->unicode)
    {
      memcpy (scratch2, scratch3, s_width * s_height);
      memcpy (scratch, scratch3, s_width * s_height);
    }
  else
    {
      memset (scratch, 0, s_height * s_width);
      place_a (left, right, 1.0);

      for (y = 1; y < s_height-1; y++)
        for (x = 1; x < s_width-1; x++)
            if (scratch [y * s_width + x] < 254)
              scratch [y * s_width + x] = 0;

      for (int j = 0; j < DIST_MAX; j++)
        {
          save_scratch ();
          for (y = 1; y < s_height-1; y++)
            for (x = 1; x < s_width-1; x++)
              {
                if (scratch2 [y * s_width + x] < 254)
                  {
                    if (scratch2 [y * s_width + x -1] ||
                        scratch2 [y * s_width + x +1] ||
                        scratch2 [y * s_width + x - s_width] ||
                        scratch2 [y * s_width + x + s_width])
                     scratch [y * s_width + x] ++;
                  }
              }
        }
      save_scratch ();

      scratch3_unicode = left->unicode;
      memcpy (scratch3, scratch2, s_width * s_height);
    }

  for (y = 0; y < right->r_height; y++)
    for (x = 0; x < right->r_width; x++)
      if (
          x + space < s_width &&
          x + space > 0 &&
          y < s_height &&
          
          right->raster[y * right->r_width + x] > 170)
        {
          if (scratch2 [y * s_width + x + space] > 0 &&
              DIST_MAX-scratch2 [y * s_width + x + space] < min_dist)
            min_dist = DIST_MAX-scratch [y * s_width + x + space];
          scratch [y * s_width + x + space] = 200;
        }

  if (min_dist < 0)
    min_dist = -1;
  if (min_dist >= 1000)
    min_dist = -1;
  return min_dist;
}


static float compute_xheight_graylevel (Glyph *left, Glyph *right, int s)
{
  int x0, y0, x1, y1;
  int x, y;
  int count = 0;

  float graylevel = 0;
  count = 0;
  y0 = kernagic_x_height () * 1.0 * scale_factor;
  y1 = kernagic_x_height () * 2.0 * scale_factor;
  x0 = left->ink_width * scale_factor * 0.4;
  x1 = s + right->ink_width * scale_factor * 0.6;
  for (y = y0; y < y1; y++)
    for (x = x0; x < x1; x++)
      {
        graylevel += scratch [y * s_width + x];
        count ++;
      }
  graylevel = graylevel / count / 255.0;
  return graylevel;
}

/* build an array of valid starting points per scanline.. being the left most
 * non-filled pixels of the glyph..
 *
 */

static void floodfill (int x, int y, int x0, int y0, int x1, int y1, int *count)
{
  if (x < x0 || x > x1 || y < y0 || y > y1 ||
      scratch [y * s_width + x] != 0)
    return;
  scratch [y * s_width + x] = 127;
  (*count) ++;
  floodfill (x + 1, y, x0, y0, x1, y1, count);
  floodfill (x - 1, y, x0, y0, x1, y1, count);
  floodfill (x, y + 1, x0, y0, x1, y1, count);
  floodfill (x, y - 1, x0, y0, x1, y1, count);
}

static float compute_negative_area_ratio (Glyph *left, Glyph *right, int s)
{
  int x0, y0, x1, y1;
  int x = 0, y;
  int count = 0;

  count = 0;
  y0 = kernagic_x_height () * 1.04 * scale_factor;
  y1 = kernagic_x_height () * 1.96 * scale_factor;
  x0 = left->ink_width * scale_factor * 0.2;
  x1 = s + right->ink_width * scale_factor *0.8;

  for (y = y0 + 1; y < y1; y++)
    {
      x = left->scan_width[y];
      if (scratch [y * s_width + x] == 0)
        break;
    }
  if (x < x0)
    x = x0;

  floodfill (x + 1, y, x0, y0, x1, y1, &count);
  return count / (kernagic_x_height () * scale_factor * kernagic_x_height() * scale_factor * 1.0);
}

static void place_a (Glyph *left, Glyph *right, float opacity)
{
  int x, y;

  for (y = 0; y < left->r_height; y++)
    for (x = 0; x < left->r_width; x++)
      if (x < s_width && y < s_height)
      scratch [y * s_width + x] = left->raster[y * left->r_width + x] * opacity;
}

static void place_glyphs (Glyph *left,
                          Glyph *right,
                          float        spacing)
{
  int x, y;
  int space = spacing;
  assert (left);
  assert (right);

  memset (scratch, 0, s_height * s_width);
  place_a (left, right, 1.0);

  for (y = 0; y < right->r_height; y++)
    for (x = 0; x < right->r_width; x++)
      if (x + space < s_width &&
          x + space >= 0 && 
          y < s_height)
        scratch [y * s_width + x + space] += right->raster[y * right->r_width + x];
}

static GtkWidget *drawing_area;


float kerner_kern (KernerSettings *settings,
                   Glyph          *left,
                   Glyph          *right)
{
  int s;
  int min_dist;

  gint    best_advance = 0;
  gfloat  best_diff = 10000.0;

  int maxs = left->ink_width * scale_factor * 1.5;

  if (maxs < kernagic_x_height () * scale_factor)
    maxs = kernagic_x_height () * scale_factor;

  /* XXX: it would be better to go left until going under min_dist, then from
   * guess mid_point go right until going over max dist,. this would cut out
   * even distance testing of the extremes that are not neccesary to test on.
   */
  for (s = left->ink_width * scale_factor * 0.5; s < maxs; s++)
  {
    min_dist = compute_dist (left, right, s);

    if (min_dist < settings->maximum_distance * kernagic_x_height () * scale_factor / 100.0 &&
        
        min_dist > settings->minimum_distance * kernagic_x_height () * scale_factor / 100.0)
      {
        place_glyphs (left, right, s);
        alpha = compute_xheight_graylevel (left, right, s);
        //place_glyphs (left, right, s);
        beta = compute_negative_area_ratio (left, right, s);

        float alphadiff = fabs (alpha - settings->alpha_target / 100.0);
        //float betadiff  = fabs (beta  - settings->beta_target / 100.0);
        float sumdiff;

        //alphadiff *= alphadiff;
        //betadiff *= betadiff;
        //alphadiff *= settings->alpha_strength / 100.0;
        //betadiff  *= settings->beta_strength / 100.0;

        sumdiff = alphadiff /* + betadiff */;

        if (sumdiff < best_diff)
          {
            best_diff = sumdiff;
            best_advance = s;
          }

        if (visual_debug_enabled)
          {
            gtk_widget_queue_draw (drawing_area);
            //for (int i = 0; i < 5500; i++)
              {
                gtk_main_iteration_do (FALSE);
              }
          }
      }
  }
  return best_advance / scale_factor;
}

static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data)
{
  cairo_set_source_rgb (cr, 1,1,1);
  cairo_paint (cr);

  cairo_surface_t *g_surface;
  
  g_surface = cairo_image_surface_create_for_data (scratch, CAIRO_FORMAT_A8,
      s_width, s_height, s_width);

  cairo_set_source_rgb (cr, 1,0,0);
  cairo_translate (cr, 0, 0);
  cairo_mask_surface (cr, g_surface, 0, 0);
  cairo_fill (cr);

  cairo_surface_destroy (g_surface);
  cairo_set_source_rgb (cr, 0,0,0);
  
  cairo_select_font_face(cr, "Sans",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 20);

  char buf[4096];
  float y = 0.0;
  float x = 300;

  sprintf (buf, "alpha: %2.2f%%", 100 * alpha);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);


  sprintf (buf, "beta: %2.2f%%", 100 * beta);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);

  return FALSE;
}

void kerner_debug_ui (void)
{
  GtkWidget *window;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 512, 256);
  g_signal_connect (drawing_area, "draw", G_CALLBACK (draw_cb), NULL);
  gtk_container_add (GTK_CONTAINER (window), drawing_area);
  gtk_widget_show (drawing_area);
  gtk_widget_show (window);
  visual_debug_enabled = TRUE;
}

void init_kerner (void)
{
  if (scratch != NULL)
    return;

  s_width  = kernagic_x_height () * scale_factor * 4;
  s_height = kernagic_x_height () * scale_factor * 3;

  s_width /= 8;
  s_width *= 8;

  scratch  = g_malloc0 (s_width * s_height);
  scratch2 = g_malloc0 (s_width * s_height);
  scratch3 = g_malloc0 (s_width * s_height);
}

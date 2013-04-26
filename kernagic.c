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

#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernagic.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>

#define SPECIMEN_SIZE 60

static char *ufo_path = NULL;

GList *glyphs = NULL;
float  scale_factor = 0.18;
static gunichar *glyph_string = NULL;

#define DIST_MAX  120

int      s_width  = 0;
int      s_height = 0;
uint8_t *scratch  = NULL;
uint8_t *scratch2 = NULL;
uint8_t *scratch3 = NULL;
long int scratch3_unicode  = -1;

float    kernagic_max_dist = 0.1;
float    kernagic_min_dist = 0.1;
float    kernagic_gray_target = 0.5;

gboolean kernagic_strip_bearing = FALSE;

static pthread_mutex_t kernagic_mutex;

void kernagic_lock (void)
{
  pthread_mutex_lock (&kernagic_mutex);
}

void kernagic_unlock (void)
{
  pthread_mutex_unlock (&kernagic_mutex);
}

void init_kernagic (void)
{
  if (scratch != NULL)
    return;

  s_width  = kernagic_x_height () * scale_factor * 4;
  s_height = kernagic_x_height () * scale_factor * 3;

  s_width /= 8;
  s_width *= 8;

  scratch  = calloc (s_width * s_height, 1);
  pthread_mutex_init (&kernagic_mutex, NULL);
  kernagic_lock ();

  scratch2 = calloc (s_width * s_height, 1);
  scratch3 = calloc (s_width * s_height, 1);

  kernagic_unlock ();
}

void save_scratch (void)
{
  memcpy (scratch2, scratch, s_width * s_height);
}

void render_ufo_glyph (Glyph *glyph);

void glyph_free (Glyph *glyph)
{
  if (glyph->path)
    free (glyph->path);
  if (glyph->name)
    free (glyph->name);
  if (glyph->xml)
    free (glyph->xml);
  if (glyph->raster)
    free (glyph->raster);
  if (glyph->kerning)
    g_hash_table_destroy (glyph->kerning);
  free (glyph);
}

Glyph *nsd_glyph_new (const char *path)
{
  Glyph *glyph = calloc (sizeof (Glyph), 1);
  g_file_get_contents (path, &glyph->xml, NULL, NULL);

  if (glyph->xml)
    {
      load_ufo_glyph (glyph);
    }
  else
    {
      free (glyph);
      glyph = NULL;
    }

  if (glyph)
    {
      /* skipping some glyphs */
      if (!glyph->name ||
          glyph->unicode == ' ')
        {
          free (glyph);
          glyph = NULL;
        }
    }

  if (glyph)
    {
      glyph->kerning = g_hash_table_new (g_direct_hash, g_direct_equal);
      glyph->path = strdup (path);
    }
  return glyph;
}

static void free_ptr (void *p)
{
  void **ptr = p;
  if (!ptr)return;
  if (*ptr)
    {
      free (*ptr);
      *ptr = NULL;
    }
}

void nsd_glyph_free (Glyph *glyph)
{
  free_ptr (&glyph->xml);
  free_ptr (&glyph->raster);
  free_ptr (&glyph->name);
  free (glyph);
}

static int
add_glyph(const char *fpath, const struct stat *sb,
             int tflag, struct FTW *ftwbuf)
{
  if (strstr (fpath, "contents.plist"))
    return 0;
  Glyph *glyph = nsd_glyph_new (fpath);

  if (glyph)
    glyphs = g_list_prepend (glyphs, glyph);
  return 0;
}

char *psystem (const char *command)
{
  FILE *fp;
  char buf[1024]="";
  fp = popen(command, "r");
  if (fp == NULL) {
    fprintf(stderr, "Failed to run command: %s\n", command);
    return strdup ("");
  }
  fgets(buf, sizeof(buf)-1, fp);
  pclose(fp);
  return strdup (buf);
}

int atoi_system (const char *command)
{
  int ret;
  char *res = psystem (command);
  ret = atoi (res);
  free (res);
  return ret;
}

/* simplistic recomputation of right bearings; it uses the average advance
 * taking all kerning pairs into account, and modifies all the involved
 * kerning pairs accordingly.
 */
void
recompute_right_bearings ()
{
  GList *l;
  for (l = glyphs; l; l= l->next)
    {
      Glyph *lg = l->data;
      GList *r;
      float advance_sum = 0;
      long  glyph_count = 0;
      int   new_advance;
      int   advance_diff;
      for (r = glyphs; r; r= r->next)
        {
          Glyph *rg = r->data;
          advance_sum += kernagic_kern_get (lg, rg) + lg->advance;
          glyph_count ++;
        }
      new_advance = advance_sum / glyph_count;
      advance_diff = new_advance - lg->advance;
      lg->advance = new_advance;
      for (r = glyphs; r; r= r->next)
        {
          Glyph *rg = r->data;
          float oldkern = kernagic_kern_get (lg, rg);
          kernagic_kern_set (lg, rg, oldkern - advance_diff);
        }
    }
}

void kernagic_save_kerning_info (void)
{
  GString *str = g_string_new (
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\"\n"
"\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"  <dict>\n");

  GList *left;
  GList *right;

  recompute_right_bearings ();

  for (left = glyphs; left; left = left->next)
  {
    Glyph *lg = left->data;
    int found = 0;

    for (right = glyphs; right; right = right->next)
      {
        Glyph *rg = right->data;
        float kerning;

        if ((kerning=kernagic_kern_get (lg, rg)) != 0.0)
          {
            if (!found)
              {
    g_string_append_printf (str, "    <key>%s</key>\n    <dict>\n", lg->name);
    found = 1;
              }
      
            g_string_append_printf (str, "      <key>%s</key><integer>%d</integer>\n", rg->name, (int)kerning);

          }
      }
    if (found)
      g_string_append (str, "    </dict>\n");
  }

  g_string_append (str, "  </dict>\n</plist>");

  char path[4095];
  GError *error = NULL;

  sprintf (path, "%s/kerning.plist", ufo_path);

  g_file_set_contents (path, str->str, -1, &error);
  if (error)
    fprintf (stderr, "EEeek %s\n", error->message);
  g_string_free (str, TRUE);

  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      rewrite_ufo_glyph (glyph);
    }
}

void kernagic_load_ufo (const char *font_path, gboolean strip_left_bearing)
{
  char path[4095];

  kernagic_strip_bearing = strip_left_bearing;

  if (ufo_path)
    free (ufo_path);
  ufo_path = strdup (font_path);
  if (ufo_path [strlen(ufo_path)] == '/')
    ufo_path [strlen(ufo_path)] = '\0';

  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      glyph_free (glyph);
    }
  g_list_free (glyphs);
  glyphs = NULL;

  sprintf (path, "%s/glyphs", ufo_path);

  if (nftw(path, add_glyph, 20, 0) == -1)
    {
      fprintf (stderr, "EEEeeek! probably not a ufo dir\n");
    }

  scale_factor = SPECIMEN_SIZE / kernagic_x_height ();

  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      render_ufo_glyph (glyph);
    }
  init_kernagic ();
}

void kernagic_kern_clear_all (void)
{
  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      g_hash_table_remove_all (glyph->kerning);
    }
}

void kernagic_kern_set (Glyph *a, Glyph *b, float kerning)
{
  int intkern = kerning * 10;
  if (!a || !b)
    return;

  g_hash_table_insert (a->kerning, b, GINT_TO_POINTER (intkern));
}

float kernagic_kern_get (Glyph *a, Glyph *b)
{
  int intkern;
  float kern;
  if (!a || !b)
    return 0;

  gpointer pvalue = g_hash_table_lookup (a->kerning, b);

  intkern = GPOINTER_TO_INT (pvalue);
  kern = intkern / 10.0;
  return kern;
}

Glyph *kernagic_find_glyph_unicode (unsigned int unicode)
{
  GList *l;
  for (l = glyphs; l; l = l->next)
    {
      Glyph *glyph = l->data;
      if (glyph->unicode == unicode)
        return glyph;
    }
  return NULL;
}

static Glyph   *ga = NULL;
static Glyph   *gb = NULL;
static int      space = 0;

float graylevel  = 0.23;
float graylevel2 = 0.23;
float overpaint    = 0.0;
int   g2_width = 0;
int   g1_width = 0;
int   min_dist = 0;

void place_a (float opacity);
void update_stats (int s)
{
  int x0, y0, x1, y1;
  int x, y;
  int count = 0;
  graylevel = 0.0;

  count = 0;
  y0 = kernagic_x_height () * 1.0 * scale_factor;
  y1 = kernagic_x_height () * 2.0 * scale_factor;
  x0 = 0;
  x1 = x0 + s + g2_width * scale_factor;

  overpaint = 0;
  for (y = 0; y < s_height; y++)
    for (x = x0; x < x1; x++)
      {
        if (scratch [y * s_width + x] > 127)
          overpaint++;
        count ++;
      }
  overpaint /= count;

  count = 0;
  for (y = y0; y < y1; y++)
    for (x = x0; x < x1; x++)
      {
        graylevel += scratch [y * s_width + x];
        count ++;
      }
  graylevel = graylevel / count / 127.0;

  graylevel2 = 0;
  count = 0;
  y0 = kernagic_x_height () * 1.0 * scale_factor;
  y1 = kernagic_x_height () * 2.0 * scale_factor;
  x0 = g1_width * scale_factor / 2;
  x1 = s + g2_width * scale_factor / 2;
  for (y = y0; y < y1; y++)
    for (x = x0; x < x1; x++)
      {
        graylevel2 += scratch [y * s_width + x];
        count ++;
        scratch [y * s_width + x] += 120;
      }
  graylevel2 = graylevel2 / count / 127.0;

  if (scratch3_unicode == ga->unicode)
    {
      memcpy (scratch2, scratch3, s_width * s_height);
      memcpy (scratch, scratch3, s_width * s_height);
    }
  else
    {
      memset (scratch, 0, s_height * s_width);
      place_a (1.0);


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

      scratch3_unicode = ga->unicode;
      memcpy (scratch3, scratch2, s_width * s_height);
    }

  min_dist = 1000;

  for (y = 0; y < gb->r_height; y++)
    for (x = 0; x < gb->r_width; x++)
      if (
          x + space < s_width &&
          x + space > 0 &&
          y < s_height &&
          
          gb->raster[y * gb->r_width + x] > 170)
        {
          if (scratch2 [y * s_width + x + space] > 0 &&
              DIST_MAX-scratch2 [y * s_width + x + space] < min_dist)
            min_dist = DIST_MAX-scratch [y * s_width + x + space];
          scratch [y * s_width + x + space] = 200;
        }

  if (min_dist < 0)
    min_dist = 1000;
  if (min_dist > 1000)
    min_dist = 1000;
}


void place_a (float opacity)
{
  int x, y;

  for (y = 0; y < ga->r_height; y++)
    for (x = 0; x < ga->r_width; x++)
      if (x < s_width && y < s_height)
      scratch [y * s_width + x] = ga->raster[y * ga->r_width + x] * opacity;
}

void place_glyphs (unsigned int glyph_a_unicode,
                   unsigned int glyph_b_unicode,
                   float         spacing)
{
  int x, y;
  space = spacing;
  ga = kernagic_find_glyph_unicode (glyph_a_unicode);
  gb = kernagic_find_glyph_unicode (glyph_b_unicode);
  assert (ga);
  assert (gb);

  memset (scratch, 0, s_height * s_width);
  place_a (0.5);

  for (y = 0; y < gb->r_height; y++)
    for (x = 0; x < gb->r_width; x++)
      if (x + space < s_width &&
          x + space >= 0 && 
          y < s_height)
        scratch [y * s_width + x + space] += gb->raster[y * gb->r_width + x] / 2;

  g1_width = ga->width;
  g2_width = gb->width;
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

  sprintf (buf, "graylevel:   %2.2f%%", 100 * graylevel);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);

  sprintf (buf, "graylevel2: %2.2f%%", 100 * graylevel2);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);

  sprintf (buf, "overpaint:      %2.2f%%", 100 * overpaint);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);

  sprintf (buf, "min-dist:    %2.2f", min_dist / scale_factor);
  cairo_move_to (cr, x, y+=30);
  cairo_show_text (cr, buf);

  return FALSE;
}

GtkWidget *drawing_area;

/* programmatic way of finding x-height, is guaranteed to work better than
 * font metadata...
 */
float kernagic_x_height (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('x');
  if (!g)
    return 0.0;
  return (g->max_y - g->min_y);
}


static gboolean deal_with_glyph (gunichar unicode)
{
  int i;
  if (!glyph_string)
    return TRUE;
  for (i = 0; glyph_string[i]; i++)
    if (glyph_string[i] == unicode)
      return TRUE;
  return FALSE;
}

static gboolean deal_with_glyphs (gunichar unicode, gunichar unicode2)
{
  int i;
  if (!glyph_string)
    return TRUE;
  if (!unicode || !unicode2)
    return FALSE;
  for (i = 0; glyph_string[i]; i++)
    if (glyph_string[i] == unicode &&
        glyph_string[i+1] == unicode2)
      return TRUE;
  return FALSE;
}

void kernagic_compute (GtkProgressBar *progress)
{
  long int total = g_list_length (glyphs);
  long int count = 0;
  GList *left;
  GList *right;

  total *= total;

  for (left = glyphs; left; left = left->next)
  {
    Glyph *lg = left->data;
  
    if (progress || deal_with_glyph (lg->unicode))
    for (right = glyphs; right; right = right->next)
      {
        Glyph *rg = right->data;
        count ++;
        if (progress || deal_with_glyphs (lg->unicode, rg->unicode))
        {
        int s;

        gint    best_advance = 0;
        gfloat  best_gray_diff = 1.0;

        int maxs = lg->width * scale_factor * 1.5;

        if (maxs < kernagic_x_height () * scale_factor)
          maxs = kernagic_x_height () * scale_factor;

        for (s = lg->width * scale_factor * 0.5; s < maxs; s++)
        {
          place_glyphs (lg->unicode, rg->unicode, s);
          update_stats (s);

          if (min_dist < kernagic_max_dist * kernagic_x_height () * scale_factor &&
              
              min_dist > kernagic_min_dist * kernagic_x_height () * scale_factor
              
              &&
              overpaint <= 0.0 &&
              graylevel2 >= 0.01 && graylevel2 <= 0.98)
            {
              float graydiff = fabs (graylevel2 - kernagic_gray_target / 100.0);
              if (graydiff < best_gray_diff)
                {
                  best_gray_diff = graydiff;
                  best_advance = s;

                }
            }
        }
        
        kernagic_kern_set (lg, rg,
            best_advance / scale_factor - lg->advance);
        }
        if (progress)
          {
            float fraction = count / (float)total;
            gtk_progress_bar_set_fraction (progress, fraction);
            gtk_main_iteration_do (FALSE);
          }
      }
  }

  return;
}

void process_debug_ui (void)
{
  GtkWidget *window;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 512, 256);
  g_signal_connect (drawing_area, "draw", G_CALLBACK (draw_cb), NULL);
  gtk_container_add (GTK_CONTAINER (window), drawing_area);
  gtk_widget_show (drawing_area);
  gtk_widget_show (window);
}

void   kernagic_set_glyph_string (const char *utf8)
{
  kernagic_lock ();
  if (glyph_string)
    free (glyph_string);
  glyph_string = NULL;
  if (utf8)
    glyph_string = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);

  kernagic_unlock ();
}

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
#include <glib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include "kernagic.h"
#include "kerner.h"

#define SPECIMEN_SIZE 60

static char *ufo_path = NULL;

static GList *glyphs = NULL;
float  scale_factor = 0.18;
static gunichar *glyph_string = NULL;

gboolean kernagic_strip_bearing = FALSE;

void init_kernagic (void)
{
  init_kerner ();
}

void render_ufo_glyph (Glyph *glyph);

void glyph_free (Glyph *glyph)
{
  if (glyph->path)
    g_free (glyph->path);
  if (glyph->name)
    g_free (glyph->name);
  if (glyph->xml)
    g_free (glyph->xml);
  if (glyph->raster)
    g_free (glyph->raster);
  if (glyph->kerning)
    g_hash_table_destroy (glyph->kerning);
  g_free (glyph);
}

Glyph *nsd_glyph_new (const char *path)
{
  Glyph *glyph = g_malloc0 (sizeof (Glyph));
  g_file_get_contents (path, &glyph->xml, NULL, NULL);

  if (glyph->xml)
    {
      load_ufo_glyph (glyph);
    }
  else
    {
      g_free (glyph);
      glyph = NULL;
    }

  if (glyph)
    {
      /* skipping some glyphs */
      if (!glyph->name ||
          glyph->unicode == ' ')
        {
          g_free (glyph);
          glyph = NULL;
        }
    }

  if (glyph)
    {
      glyph->kerning = g_hash_table_new (g_direct_hash, g_direct_equal);
      glyph->path = g_strdup (path);
    }
  return glyph;
}

void nsd_glyph_free (Glyph *glyph)
{
  if (glyph->xml)
    g_free (glyph->xml);
  if (glyph->raster)
    free (glyph->raster);
  if (glyph->name)
    g_free (glyph->name);
  g_free (glyph);
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

  GList *left, *right;

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
    g_free (ufo_path);
  ufo_path = g_strdup (font_path);
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
      fprintf (stderr, "EEEeeek! '%s' probably not a ufo dir\n", ufo_path);
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
            float kerned_advance = kerner_kern (&kerner_settings, lg, rg);
            kernagic_kern_set (lg, rg, kerned_advance - lg->advance);
          }
        if (progress)
          {
            float fraction = count / (float)total;
            gtk_progress_bar_set_fraction (progress, fraction);
            gtk_main_iteration_do (FALSE);
          }
      }
  }
}

void   kernagic_set_glyph_string (const char *utf8)
{
  if (glyph_string)
    g_free (glyph_string);
  glyph_string = NULL;
  if (utf8)
    glyph_string = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);
}

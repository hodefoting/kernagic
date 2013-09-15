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
#include "kernagic-glyph.h"


void   kernagic_set_advance        (Glyph *a, float advance)
{
  //a->advance = advance;
  fprintf (stderr, "ooops\n");
}

float  kernagic_get_advance        (Glyph *a)
{
  return a->ink_width + a->left_bearing + a->right_bearing;
}

void kernagic_set_kerning (Glyph *a, Glyph *b, float kerning)
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

void load_ufo_glyph (Glyph *glyph);

Glyph *kernagic_glyph_new (const char *path)
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
      if (!glyph->name || glyph->unicode == ' ')
        {
          g_free (glyph);
          glyph = NULL;
        }
    }

  if (glyph)
    {
      glyph->kerning = g_hash_table_new (g_direct_hash, g_direct_equal);
      glyph->path = g_strdup (path);
      glyph->lstem = 0.0;
      glyph->rstem = 0.0;
    }
  return glyph;
}

void kernagic_glyph_reset (Glyph *glyph)
{
  if (glyph->kerning)
    g_hash_table_destroy (glyph->kerning);
  glyph->kerning = g_hash_table_new (g_direct_hash, g_direct_equal);
  glyph->left_bearing = 0;
  glyph->right_bearing = 0;
}

void kernagic_glyph_free (Glyph *glyph)
{
  if (glyph->xml)
    g_free (glyph->xml);
  if (glyph->raster)
    free (glyph->raster);
  if (glyph->name)
    g_free (glyph->name);
  g_free (glyph);
}

void   kernagic_set_left_bearing  (Glyph *g, float left_bearing)
{
  g->left_bearing = left_bearing;
}
void   kernagic_set_right_bearing (Glyph *g, float right_bearing)
{
  g->right_bearing = right_bearing;
}

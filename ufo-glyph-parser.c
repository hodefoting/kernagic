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

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "kernagic.h"

static int   cc = 0;
static float cx[2];
static float cy[2];
static float scx = 0;
static float scy = 0;
static int   sc = 0;
static int   first = 0;

extern gboolean kernagic_strip_bearing; /* XXX: global and passed out of bounds.. */

static void
parse_start_element (GMarkupParseContext *context,
                     const gchar         *element_name,
                     const gchar        **attribute_names,
                     const gchar        **attribute_values,
                     gpointer             user_data,
                     GError             **error)
{
  Glyph *glyph = user_data;
  if (!strcmp (element_name, "glyph"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "name"))
            glyph->name = g_strdup (*a_v);
        }
      glyph->ink_min_x =  8192;
      glyph->ink_min_y =  8192;
      glyph->ink_max_x = -8192;
      glyph->ink_max_y = -8192;
    }
  else if (!strcmp (element_name, "advance"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "width"))
            {};//glyph->advance = atoi (*a_v);
        }
    }
  else if (!strcmp (element_name, "unicode"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "hex"))
          {
            unsigned int value;
            if(sscanf(*a_v, "%X;", &value) != 1)
               printf("Parse error\n");
            glyph->unicode = value;
          }
        }
    }
  else if (!strcmp (element_name, "point"))
    {
      float x = 0;
      float y = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "x"))
            x = atof (*a_v);
          if (!strcmp (*a_n, "y"))
            y = atof (*a_v);
        }
      if (x < glyph->ink_min_x) glyph->ink_min_x = x;
      if (y < glyph->ink_min_y) glyph->ink_min_y = y;
      if (x > glyph->ink_max_x) glyph->ink_max_x = x;
      if (y > glyph->ink_max_y) glyph->ink_max_y = y;
    }
}

static void
parse_end_element (GMarkupParseContext *context,
                   const gchar         *element_name,
                   gpointer             user_data,
                   GError             **error)
{
  //Glyph *glyph = user_data;
}


static void
glif_start_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    const gchar        **attribute_names,
                    const gchar        **attribute_values,
                    gpointer             user_data,
                    GError             **error)
{
  Glyph *glyph = user_data;
  cairo_t *cr = glyph->cr;

  if (!strcmp (element_name, "point"))
    {
      int   offcurve = 1;
      int   curveto = 0;
      float x = 0, y = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "x"))
            x = atof (*a_v) + glyph->offset_x;
          else if (!strcmp (*a_n, "y"))
            y = atof (*a_v);
          else if (!strcmp (*a_n, "type"))
            {
              if (!strcmp (*a_v, "line") ||
                  !strcmp (*a_v, "curve"))
                offcurve = 0;
              if (!strcmp (*a_v, "curve"))
                curveto = 1;
            }
        }
      if (offcurve)
        {
          cx[cc] = x;
          cy[cc] = y;
          cc++;
          assert (cc <= 2);
        }
      else
        {
          if (curveto && cc == 2)
              cairo_curve_to (cr, cx[0], cy[0],
                                  cx[1], cy[1],
                                  x, y);
          else if (curveto && cc == 1)
              cairo_curve_to (cr, cx[0], cy[0],
                                  cx[0], cy[0],
                                  x, y);
          else if (curveto && cc == 0)
            {
              if (first)
                {
                  scx = x;
                  scy = y;
                  sc = 1;
                  cairo_move_to (cr, x, y);
                }
              else
                {
                  cairo_curve_to (cr, cx[0], cy[0],
                                      cx[0], cy[0],
                                      x, y);
                }
            }
          else
              cairo_line_to (cr, x, y);
          cc = 0;
        }
      first = 0;
    }
  else if (!strcmp (element_name, "contour"))
    {
      cairo_new_sub_path (cr);
      first = 1;
      sc = 0;
      cc = 0;
    }
}

static void
glif_end_element (GMarkupParseContext *context,
                  const gchar         *element_name,
                  gpointer             user_data,
                  GError             **error)
{
  Glyph *glyph = user_data;
  cairo_t *cr = glyph->cr;

  if (!strcmp (element_name, "contour"))
  {
    if (sc)
      {
        assert (cc == 2);
        cairo_curve_to (cr, cx[0], cy[0],
                            cx[1], cy[1],
                            scx, scy);
      }
  }
  else if (!strcmp (element_name, "outline"))
    {
      cairo_set_source_rgb (cr, 0.0, 0.0,0.0);
      cairo_fill_preserve (cr);
    }
}

static GMarkupParser glif_parse =
{ parse_start_element, parse_end_element, NULL, NULL, NULL };

static GMarkupParser glif_render =
{ glif_start_element, glif_end_element, NULL, NULL, NULL };

extern float  scale_factor;

void render_glyph (Glyph *glyph)
{
  cairo_t *cr = glyph->cr;
  GMarkupParseContext *ctx;
  cairo_save (cr);
  ctx = g_markup_parse_context_new (&glif_render, 0, glyph, NULL);
  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);
  cairo_restore (cr);

  {
    int y;
    for (y = 0; y < glyph->r_height; y ++)
      {
        int x;
        for (x = glyph->r_width -1; x>=0; x--)
          {
            if (glyph->raster[y * glyph->r_width + x] != 0)
              break;
          }
        glyph->scan_width[y] = x + 1;
      }
  }
}

void
load_ufo_glyph (Glyph *glyph)
{
  GMarkupParseContext *ctx = g_markup_parse_context_new (&glif_parse, 0, glyph, NULL);
  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);

  glyph->ink_width = glyph->ink_max_x - glyph->ink_min_x;
  glyph->ink_height = glyph->ink_max_y - glyph->ink_min_y;

  if (kernagic_strip_bearing)
    {
      glyph->offset_x = -glyph->ink_min_x;

      glyph->ink_min_x   += glyph->offset_x;
      glyph->ink_max_x   += glyph->offset_x;
      //glyph->advance += glyph->offset_x;
      glyph->left_bearing = 0;
      glyph->right_bearing = 0;
    }
  else
    {
      fprintf (stderr, "ow\n");
    }
}

void
render_ufo_glyph (Glyph *glyph)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  assert (glyph->xml);

  load_ufo_glyph (glyph);
  

  {
    glyph->r_width = kernagic_x_height () * scale_factor * 2.5;
    glyph->r_height = kernagic_x_height () * scale_factor * 2.8;
    glyph->r_width /= 16;
    glyph->r_width *= 16;

    glyph->raster = g_malloc0 (glyph->r_width * glyph->r_height);
  }
  int width = glyph->r_width;
  int height = glyph->r_height;
  uint8_t *raster = glyph->raster;
  assert (raster);

  surface = cairo_image_surface_create_for_data (raster, CAIRO_FORMAT_A8,
      width, height, width);
  cr = cairo_create (surface);
  glyph->cr = cr;

  cairo_set_source_rgba (cr, 1, 1, 1, 0);
  cairo_paint(cr);

  cairo_translate (cr, 0, kernagic_x_height () * 2.0 * scale_factor);
  cairo_scale (cr, scale_factor, -scale_factor);
  render_glyph (glyph);

  cairo_destroy (cr);
  glyph->cr = NULL;
  cairo_surface_destroy (surface);

  {
    int x;
    int t;
    for (t = 1; t < 8; t ++)
    for (x = 0; x < glyph->r_width; x++)
    {
      long sum = 0;
      int y;
      int c = 0;
      int y0 = kernagic_x_height () * 1.0 * scale_factor;
      int y1 = kernagic_x_height () * 2.0 * scale_factor;

      for (y = y0; y < y1; y++)
        {
          sum += raster[glyph->r_width * y + x];
          c++;
        }
      raster [glyph->r_width * (glyph->r_height-t) + x] = sum / c;
    }
  }
}

/**********************************************************************************/

static GString *ts = NULL;

static void
rewrite_start_element (GMarkupParseContext *context,
                       const gchar         *element_name,
                       const gchar        **attribute_names,
                       const gchar        **attribute_values,
                       gpointer             user_data,
                       GError             **error)
{
  Glyph *glyph = user_data;
  g_string_append_printf (ts, "<%s ", element_name);
  const char **a_n, **a_v;
  for (a_n = attribute_names,
       a_v = attribute_values; *a_n; a_n++, a_v++)
     {
       if (!strcmp (element_name, "point") && !strcmp (*a_n, "x"))
         {
           char str[512];
           int value = atoi (*a_v);
           value = value + glyph->offset_x;
           sprintf (str, "%d", value);
           g_string_append_printf (ts, "%s=\"%s\" ", *a_n, str);
         }
       else if (!strcmp (element_name, "advance") && !strcmp (*a_n, "width"))
         {
           char str[512];
           sprintf (str, "%d", (int)(kernagic_get_advance (glyph)));
           g_string_append_printf (ts, "%s=\"%s\" ", *a_n, str);
         }
       else
         {
           g_string_append_printf (ts, "%s=\"%s\" ", *a_n, *a_v);
         }
     }
  g_string_append_printf (ts, ">");
}

static void
rewrite_end_element (GMarkupParseContext *context,
                     const gchar         *element_name,
                     gpointer             user_data,
                     GError             **error)
{
  g_string_append_printf (ts, "</%s>", element_name);
}

static void
rewrite_text (GMarkupParseContext *context,
              const gchar         *text,
              gsize                text_len,
              gpointer             user_data,
              GError             **error)
{
  g_string_append_len (ts, text, text_len);
}

static void
rewrite_passthrough (GMarkupParseContext *context,
                     const gchar         *passthrough_text,
                     gsize                text_len,
                     gpointer             user_data,
                     GError             **error)
{
  g_string_append_len (ts, passthrough_text, text_len);
}

static GMarkupParser glif_rewrite =
{ rewrite_start_element, rewrite_end_element, rewrite_text, rewrite_passthrough, NULL };

void
rewrite_ufo_glyph (Glyph *glyph)
{
  ts = g_string_new ("");
  GMarkupParseContext *ctx = g_markup_parse_context_new (&glif_rewrite, 0, glyph, NULL);

  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);
  g_file_set_contents (glyph->path, ts->str, ts->len, NULL);
  g_string_free (ts, TRUE);
  ts = NULL;
}

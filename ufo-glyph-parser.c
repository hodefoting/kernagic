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

static int pinlib = 0;
static int pinself = 0;

static void 
parse_component (Glyph *glyph, const char *base, float xoffset, float yoffset)
{
  Glyph *component_glyph = kernagic_find_glyph (base);
  if (component_glyph)
  {
    int x, y;
    y = component_glyph->ink_min_y;
    x = component_glyph->ink_min_x + component_glyph->left_original;

    x -= xoffset;
    y -= yoffset;
    if (x > glyph->ink_max_x)
      glyph->ink_max_x = x;
    if (y > glyph->ink_max_y)
      glyph->ink_max_y = y;
    if (x < glyph->ink_min_x)
      glyph->ink_min_x = x;
    if (y < glyph->ink_min_y)
      glyph->ink_min_y = y;

    y = component_glyph->ink_max_y;
    x = component_glyph->ink_max_x + component_glyph->left_original;
    x -= xoffset;
    y -= yoffset;
    if (x > glyph->ink_max_x)
      glyph->ink_max_x = x;
    if (y > glyph->ink_max_y)
      glyph->ink_max_y = y;
    if (x < glyph->ink_min_x)
      glyph->ink_min_x = x;
    if (y < glyph->ink_min_y)
      glyph->ink_min_y = y;
  }
  else
    fprintf (stderr, "Problems importing '%s' maybe the component %s is using components\n", glyph->name, base);
}

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
            glyph->right_original = atoi (*a_v);
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
  else if (!strcmp (element_name, "component"))
    {
      const char *base = "";
      float xoffset = 0;
      float yoffset = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "base")) base = *a_v;
          else if (!strcmp (*a_n, "xOffset")) xoffset = atof (*a_v);
          else if (!strcmp (*a_n, "yOffset")) yoffset = atof (*a_v);
        }
      parse_component (glyph, base, xoffset, yoffset);
    }
  else if (!strcmp (element_name, "lib"))
    {
      pinlib++;
    }
}

static void
parse_text (GMarkupParseContext *context,
            const gchar         *text,
            gsize                text_len,
            gpointer             user_data,
            GError             **error)
{
  Glyph *glyph = user_data;
  if (pinself)
    {
      if (!strcmp (text, "rstem"))
        pinself = 2;

      int number = atoi (text);
      if (number)
      {
        if (pinself == 1)
          glyph->lstem = number;
        else
          glyph->rstem = number;
      }
    }
  if (!strcmp (text, "org.pippin.gimp.org.kernagic"))
    {
      pinself = 1;
    }
}

static void
parse_end_element (GMarkupParseContext *context,
                   const gchar         *element_name,
                   gpointer             user_data,
                   GError             **error)
{
  //Glyph *glyph = user_data;
  if (!strcmp (element_name, "lib"))
    pinlib --;
  if (!strcmp (element_name, "dict"))
    pinself = 0;
}

void render_glyph (Glyph *glyph);

static void 
render_component (Glyph *glyph, const char *base, float xoffset, float yoffset)
{
  Glyph *cglyph = NULL;
  cairo_t *cr = glyph->cr;
  cglyph = kernagic_find_glyph (base);
  //fprintf (stderr, "Component %s  %f,%f %p\n", base, xoffset, yoffset, cglyph);
  cairo_save (cr);

  xoffset -= cglyph->offset_x;

  cairo_translate (cr, xoffset, yoffset);

  cglyph->cr = cr;
  render_glyph (cglyph);
  cairo_restore (cr);
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
          /* too many points; wrong type of curve */
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
  else if (!strcmp (element_name, "component"))
    {
      const char *base = "";
      float xoffset = 0;
      float yoffset = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "base")) base = *a_v;
          else if (!strcmp (*a_n, "xOffset")) xoffset = atof (*a_v);
          else if (!strcmp (*a_n, "yOffset")) yoffset = atof (*a_v);
        }
      render_component (glyph, base, xoffset, yoffset);
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
        if (cc != 2)
        {
          fprintf (stderr, "it seems like font contained non cubic outlines, kernagic only deals with cubic bezier\n");
          assert (cc == 2);
        }
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
{ parse_start_element, parse_end_element, parse_text, NULL, NULL };

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
  GMarkupParseContext *ctx;

  if (glyph->loaded)
    return;
  glyph->loaded = 1;
  
  ctx = g_markup_parse_context_new (&glif_parse, 0, glyph, NULL);
  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);

  glyph->ink_width = glyph->ink_max_x - glyph->ink_min_x;
  glyph->ink_height = glyph->ink_max_y - glyph->ink_min_y;

  if (kernagic_strip_bearing)
    {
      glyph->offset_x = -glyph->ink_min_x;
      glyph->left_original = glyph->ink_min_x;
      glyph->right_original -= glyph->ink_max_x;

      glyph->ink_min_x   += glyph->offset_x;
      glyph->ink_max_x   += glyph->offset_x;

      glyph->left_bearing = 0;
      glyph->right_bearing = 0;
    }
  else
    {
      fprintf (stderr, "ow\n");
    }
}

void gen_debug (Glyph *glyph)
{
  uint8_t *raster = glyph->raster;
    int x;
    int t;
    float x_height = kernagic_x_height ();

    for (t = 1; t < 3; t ++)
    for (x = 0; x < glyph->r_width; x++)
    {
      long sum = 0;
      int y;
      long c = 0;
      int y0 = x_height  * 1.0 * scale_factor;
      int y1 = x_height  * 2.0 * scale_factor;

      for (y = y0; y < y1; y++)
        {
          sum += raster[glyph->r_width * y + x];
          c++;
        }
      {
        int foo = sum / c;
       // foo /= 32;
       // foo *= 32;
        raster [glyph->r_width * (glyph->r_height-t) + x] = foo;
      }
    }

    /* detect candidate stems/rythm points*/
    long sum = 0;
    long sum2 = 0;

    for (x = glyph->ink_width/2 * scale_factor * 0.0;
         x < glyph->ink_width * scale_factor * 0.3; x++)
      {
        int val;
        t = 1;
        val = raster [glyph->r_width * (glyph->r_height-t) + x];
        sum += val;
      }
    sum2 = 0;
    for (x = 0; sum2 < sum/2; x++)
      {
        int val;
        t = 1;
        val = raster [glyph->r_width * (glyph->r_height-t) + x];
        sum2 += val;
      }

    glyph->stems[glyph->stem_count] = x / scale_factor;
    glyph->stem_weight[glyph->stem_count++] = x;

    sum = 0;
    sum2 = 0;
    for (x = glyph->ink_width * scale_factor * 0.7;
         x < glyph->ink_width * scale_factor * 1.0; x++)
      {
        int val;
        t = 2;
        val = raster [glyph->r_width * (glyph->r_height-t) + x];
        sum += val;
      }
    sum2 = 0;
    for (x = glyph->ink_width * scale_factor * 0.7;
        sum2 < sum /2 && x < glyph->r_width; x++)
      {
        int val;
        t = 1;
        val = raster [glyph->r_width * (glyph->r_height-t) + x];
        sum2 += val;
      }

    glyph->stems[glyph->stem_count] = x / scale_factor;
    glyph->stem_weight[glyph->stem_count++] = 1;

    for (t = 1; t < 3; t ++)
    for (x = 0; x < glyph->r_width; x++)
    {
      raster [glyph->r_width * (glyph->r_height-t) + x] = 0;
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

  gen_debug (glyph);
}

/**********************************************************************************/

static GString *ts = NULL;

static int inlib = 0;
static int inself = 0;


static void
rewrite_start_element (GMarkupParseContext *context,
                       const gchar         *element_name,
                       const gchar        **attribute_names,
                       const gchar        **attribute_values,
                       gpointer             user_data,
                       GError             **error)
{
  Glyph *glyph = user_data;

  if (inself)
    return;

  g_string_append_printf (ts, "<%s", element_name);
  const char **a_n, **a_v;

  if (!strcmp (element_name, "lib"))
    {
      inlib ++;
    }

  if (!strcmp (element_name, "component"))
  {
    const char *base = "";
    float xoffset = 0;
    float yoffset = 0;
    const char **a_n, **a_v;
    for (a_n = attribute_names,
         a_v = attribute_values; *a_n; a_n++, a_v++)
      {
        if (!strcmp (*a_n, "base")) base = *a_v;
        else if (!strcmp (*a_n, "xOffset")) xoffset = atof (*a_v);
        else if (!strcmp (*a_n, "yOffset")) yoffset = atof (*a_v);
      }

      xoffset = xoffset + glyph->offset_x + glyph->left_bearing;

      Glyph *component_glyph = kernagic_find_glyph (base);
      if (component_glyph)
      {
        xoffset -= (component_glyph->offset_x + component_glyph->left_bearing);
      }

      g_string_append_printf (ts, " base=\"%s\" xOffset=\"%f\" yOffset=\"%f\" ", base, xoffset, yoffset);
  }
  else
  {
    for (a_n = attribute_names,
         a_v = attribute_values; *a_n; a_n++, a_v++)
       {
         if (!strcmp (element_name, "point") && !strcmp (*a_n, "x"))
           {
             char str[512];
             int value = atoi (*a_v);
             value = value + glyph->offset_x + glyph->left_bearing;
             sprintf (str, "%d", value);
             g_string_append_printf (ts, " %s=\"%s\"", *a_n, str);
           }
         else if (!strcmp (element_name, "advance") && !strcmp (*a_n, "width"))
           {
             char str[512];
             sprintf (str, "%d", (int)(kernagic_get_advance (glyph)));
             g_string_append_printf (ts, " %s=\"%s\"", *a_n, str);
           }
         else
           {
             g_string_append_printf (ts, " %s=\"%s\"", *a_n, *a_v);
           }
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
  Glyph *glyph = user_data;

  if (!strcmp (element_name, "lib"))
    {
      inlib --;
    }

  if (!inself)
    g_string_append_printf (ts, "</%s>", element_name);

  if (inself)
  {
    if (!strcmp (element_name, "dict"))
      {
        inself = 0;
        g_string_append_printf (ts, "</key><dict><key>lstem</key><integer>%0.0f</integer><key>rstem</key><integer>%0.0f</integer></dict>",
            glyph->lstem, glyph->rstem);
      }
  }
}

static void
rewrite_text (GMarkupParseContext *context,
              const gchar         *text,
              gsize                text_len,
              gpointer             user_data,
              GError             **error)
{
  if (inself)
    return;
  g_string_append_len (ts, text, text_len);

  if (inlib)
  {
    if (!strcmp (text, "org.pippin.gimp.org.kernagic"))
      inself = 1;
  }
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
  GString *tmp = g_string_new ("");
  ts = g_string_new ("");
  GMarkupParseContext *ctx = g_markup_parse_context_new (&glif_rewrite, 0, glyph, NULL);

  /* ensure that a lib section exist and that it contains a skeleton kernagic
   * section.
   */
  
  if (strstr (glyph->xml, "<lib"))
  {
    if (strstr (glyph->xml, "org.pippin.gimp.kernagic"))
    {
      g_string_append (tmp, glyph->xml);
    }
    else
    {
      char *cut = strdup (glyph->xml);
      char *p = strstr (cut, "<lib");
      char *rest;
      p = strstr (p, "<dict");
      p = strstr (p, ">");
      rest = p+1;
      if (p)
        *p = 0;
      g_string_append (tmp, cut);
      g_string_append (tmp, "><key>org.pippin.gimp.org.kernagic</key><dict><key>lstem</key><integer>0</integer><key>rstem</key><integer>0</integer></dict>\n");
      g_string_append (tmp, "");
      g_string_append (tmp, rest);
      free (cut);
    }
  }
  else
  {
    char *cut = strdup (glyph->xml);
    char *p = strstr (cut, "</glyph>");
    if (p)
      *p = 0;
    g_string_append (tmp, cut);
    g_string_append (tmp, "<lib><dict><key>org.pippin.gimp.org.kernagic</key><dict><key>lstem</key><integer>0</integer><key>rstem</key><integer>0</integer></dict>\n</dict></lib>\n");
    g_string_append (tmp, "</glyph>\n");
    free (cut);
  }

  /* detect whether lstem and rstem are already existing,. if they do rewrite
   * them.
   *
   * otherwise insert stems.. after initial rewrite by simple search and
   * replace.
   */

  g_markup_parse_context_parse (ctx, tmp->str, strlen (tmp->str), NULL);
  g_markup_parse_context_free (ctx);
  g_file_set_contents (glyph->path, ts->str, ts->len, NULL);
  g_string_free (ts, TRUE);
  g_string_free (tmp, TRUE);
  ts = NULL;
}

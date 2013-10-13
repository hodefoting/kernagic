#include <string.h>
#include "kernagic.h"

#include <cairo.h>

/* This is the main rendering code of kernagic.. 
 */
int debug_start_y = 0;
float debug_scale = 1.0;

uint8_t *kernagic_preview = NULL;
#define MAX_BIG       1024

Glyph  *g_entries[MAX_BIG];
int     x_entries[MAX_BIG];
int     big = 0;

typedef struct {
  unsigned long unicode;
  double x;
  double y;
} TextGlyph;

extern float  scale_factor;

gboolean toggle_measurement_lines = FALSE;


float advance_glyph (Glyph *g, float xo, int yo, float scale)
{
  return xo + kernagic_get_advance (g) * scale_factor * scale;
}

void render_glyph (Glyph *glyph);

float place_glyph (Glyph *g, float xo, int yo, float opacity, float scale)
{
  int canvas_w = canvas_width ();
  int canvas_h = canvas_height ();
  cairo_t *cr;
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data (kernagic_preview,
            CAIRO_FORMAT_A8, canvas_w, canvas_h, canvas_w);
  cr = cairo_create (surface);

  g->cr = cr;

  xo += (g->left_bearing) * scale * scale_factor;

  /* do transforms so that the original coordinates in the unmodified parsed
   * file is correct cairo-side
   */
  cairo_translate (cr, xo, yo + 2*kernagic_x_height () * scale * scale_factor);
  cairo_scale (cr, scale * scale_factor, scale * scale_factor * -1.0);

  /* render glyph, renders glyphs with ink_bounds cut off */
  render_glyph (g);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return advance_glyph (g, xo, yo, scale);
}

void place_glyphs (TextGlyph *glyphs, int num_glyphs, float opacity, float scale)
{
  int i;
  for (i = 0; i < num_glyphs; i ++)
    {
      Glyph *glyph = kernagic_find_glyph_unicode (glyphs[i].unicode);
      if (glyph)
        place_glyph (glyph, glyphs[i].x, glyphs[i].y, opacity, scale);
    }
}

static void draw_glyph_debug
(Glyph *g, float xo, int yo, float opacity, float scale, int debuglevel)
{
  int x, y;
  int canvas_w = canvas_width ();
  int canvas_h = canvas_height ();

  float y0 = yo;
  float y1 = y0 + 512 * scale * 0.8; /* XXX: should be based on xheight */

  scale *= scale_factor;

  if (toggle_measurement_lines)
    {
      /* glyph-block edges */
      for (y = y0; y < y1; y++)
      {
        int xs[] = {0, kernagic_get_advance (g) * scale};
        int i;
        for (i = 0; i < sizeof(xs)/sizeof(xs[0]); i++)
        {
          x = xs[i];
          if (x + xo >= 0 &&
              x + xo < canvas_w &&
              y >= 0 &&
              y < canvas_h &&
          kernagic_preview [y * canvas_w + (int)(x + xo)] == 0
              )
          kernagic_preview [y * canvas_w + (int)(x + xo)] = 255;
        }
      }

      for (y = y0 * 0.9 + y1 * 0.1; y < y1 * 0.9 + y0 * 0.1; y++)
        {
          /* automatic stems */
          if (g->stem_count >=1)
            {
            x = g->stems[0] * scale + g->left_bearing * scale;
            if (x + xo >= 0 &&
                x + xo < canvas_w &&
                y >= 0 &&
                y < canvas_h)
              kernagic_preview [y * canvas_w + (int)(x + xo)] = 64;
            }
          if (g->stem_count > 1)
          {
            x = g->stems[g->stem_count-1] * scale + g->left_bearing * scale;
            if (x + xo >= 0 &&
                x + xo < canvas_w &&
                y >= 0 &&
                y < canvas_h)
              kernagic_preview [y * canvas_w + (int)(x + xo)] = 64;
          }

          /* manual stems */
          if (g->lstem > 0)
          {
            x = g->lstem * scale + g->left_bearing * scale;
            if (x + xo >= 0 &&
                x + xo < canvas_w &&
                y >= 0 &&
                y < canvas_h)
              kernagic_preview [y * canvas_w + (int)(x + xo)] = 255;
          }
          if (g->rstem > 0)
          {
            x = g->rstem * scale + g->left_bearing * scale;
            if (x + xo >= 0 &&
                x + xo < canvas_w &&
                y >= 0 &&
                y < canvas_h)
              kernagic_preview [y * canvas_w + (int)(x + xo)] = 255;
          }
        }
    }
}

void draw_text (const char *string, float x, float y, float scale)
{
}

float measure_word_width (const gunichar *uword,int ulen, float scale)
{
  int j;
  Glyph *g;
  Glyph *prev_g = NULL;
  float x = 0;
  for (j = 0; j < ulen; j++)
  {
    g = kernagic_find_glyph_unicode (uword[j]);
    if (!g)
      continue;

    if (prev_g)
      x += kernagic_kern_get (prev_g, g) * scale_factor * scale;

    x = advance_glyph (g, x, 0, scale);
    prev_g = g;
  }
  return x;
}

float waterfall_offset = 0.0;
extern int desired_pos;

void redraw_test_text (const char *intext, int debuglevel)
{
  float period = kerner_settings.alpha_target;
  const char *utf8;
  gunichar *str2;
  int i;
  float x0;
  float y0;
  float linestep = 512;
  float x, y;

  int canvas_w = canvas_width ();
  int canvas_h = canvas_height ();
again:
  x0 = PREVIEW_PADDING;
  y0 = PREVIEW_PADDING;
  x = x0;
  y = y0;

  memset (kernagic_preview, 0, canvas_w * canvas_h);
  debug_start_y = canvas_h/2;
  debug_scale = 1.0;

  big = 0;
  utf8 = intext;
  if (utf8)
  {
    TextGlyph text[2048];
    int       text_count = 0;

    str2 = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);
    if (str2)
    {

      float scale = WATERFALL_START;
      int waterfall = WATERFALL_LEVELS;

      int w;

      for (w = 0; w < waterfall; w++)
      {
        linestep = 512 * scale;

        text_count = 0;

        y = y0;
        x = x0;

        x = x - waterfall_offset * scale * scale_factor + canvas_w/2;

        i = 0;
        GString *word = g_string_new ("");
        gunichar uword[1024];
        int ulen = 0;
        int wrap = 0;

        int j;

        if (w == waterfall - 1)
          {
            debug_start_y = y;
            debug_scale = scale;
          }

        for (; str2[i]; i++)
          {
            Glyph *g = kernagic_find_glyph_unicode (str2[i]);

            if (str2[i] == '\n')
              {
                if (y > 150)
                  break;

                if (wrap && x + measure_word_width (uword, ulen, scale) > canvas_w -PREVIEW_PADDING)
                {
                  y += linestep;
                  x = x0;
                }
                Glyph *prev_g = NULL;

                for (j = 0; j < ulen; j++)
                {
                  Glyph *g;

                  g = kernagic_find_glyph_unicode (uword[j]);
                  if (g)
                  {
                    if (prev_g)
                      x += kernagic_kern_get (prev_g, g) * scale_factor * scale;

                    text[text_count].unicode = g->unicode;
                    text[text_count].x = x;
                    text[text_count++].y = y;

                    x = advance_glyph (g, x, y, scale);
                    prev_g = g;
                  }
                }

                if (wrap)
                {
                  y += linestep;
                  x = x0;
                }
                g_string_assign (word, "");
                ulen = 0;
              }
            else if (g)
              {
                g_string_append_unichar (word, g->unicode);
                uword[ulen++] = g->unicode;

              }
            else if (str2[i] == ' ') /* we're only faking it if we have to  */
              {
                Glyph *t = kernagic_find_glyph_unicode ('i');
                int j;
                Glyph *prev_g = NULL;
                
                if (wrap && x + measure_word_width (uword, ulen, scale) > canvas_w-PREVIEW_PADDING)
                {
                  y += linestep;
                  x = x0;

                }

                for (j = 0; j < ulen; j++)
                {
                  Glyph *g;

                  g = kernagic_find_glyph_unicode (uword[j]);
                  if (g)
                  {
                    if (prev_g)
                      x += kernagic_kern_get (prev_g, g) * scale_factor * scale;

                    text[text_count].unicode = g->unicode;
                    text[text_count].x = x;
                    text[text_count++].y = y;

                    if (w == waterfall-1)
                    {

                      if (desired_pos != -1 && desired_pos <= i)
                      {
                        waterfall_offset = (x + waterfall_offset * scale * scale_factor - canvas_w/2)/scale/scale_factor;
                        desired_pos = -1;
                        goto again;
                      }

                      g_entries[big] = g;
                      x_entries[big++] = x;
                      draw_glyph_debug (g, x, y, 1.0, scale, debuglevel);
                    }

                    x = advance_glyph (g, x, y, scale);
                    prev_g = g;
                  }
                  else
                  {
                    g_entries[big] = kernagic_find_glyph_unicode ('i');
                    x_entries[big++] = x + waterfall_offset;
                  }
                }

                if (t)
                  x += kernagic_get_advance (t) * scale_factor * scale;

                g_string_assign (word, "");
                ulen = 0;
              }
          }

        if (word->len)
          {
            if (wrap && x + measure_word_width (uword, ulen, scale) > canvas_w-PREVIEW_PADDING)
            {
              y += linestep;
              x = x0;
            }
            Glyph *prev_g = NULL;

            for (j = 0; j < ulen; j++)
            {
              Glyph *g;

              g = kernagic_find_glyph_unicode (uword[j]);
              if (g)
              {
                if (prev_g)
                  x += kernagic_kern_get (prev_g, g) * scale_factor * scale;

                if (w == waterfall-1)
                {
                  g_entries[big] = g;
                  x_entries[big++] = x;

                  if (desired_pos != -1 && desired_pos <= i)
                  {
                    waterfall_offset = (x + waterfall_offset * scale * scale_factor - canvas_w/2)/scale/scale_factor;
                    desired_pos = -1;
                    goto again;
                  }

                  draw_glyph_debug (g, x, y, 1.0, scale, debuglevel);
                }

                text[text_count].unicode = g->unicode;
                text[text_count].x = x;
                text[text_count++].y = y;

                x = advance_glyph (g, x, y, scale);
                prev_g = g;
              }
              else
              {
                g_entries[big] = kernagic_find_glyph_unicode ('i');
                x_entries[big++] = x + waterfall_offset;
              }
            }
          }
      
        /* we wait with the blast until here */
        place_glyphs (text, text_count, 1.0, scale);

        if (waterfall > 1)
          {
            y0 += 512 * scale * WATERFALL_SPACING;
            scale = scale * WATERFALL_SCALING;

            if (w == waterfall -2) /* snap last level to full size */
              scale = 1.0;
          }
        g_string_free (word, TRUE);
        if (w == waterfall-1 && desired_pos != -1)
        {
          waterfall_offset = (x + waterfall_offset * scale * scale_factor - canvas_w/2)/scale/scale_factor;
          desired_pos = -1;
          goto again;
        }
      }
     g_free (str2);
    }
  }

  if (toggle_measurement_lines && period * scale_factor * debug_scale > 2)
  {
    int i;
    for (i = 0; i * period * scale_factor  * debug_scale < canvas_w - period * scale_factor * debug_scale; i++)
      {
        int y;
        int x = (i + 0.5) * period * scale_factor * debug_scale;
        float y0, y1;

        y0 = debug_start_y + kernagic_x_height () * scale_factor * debug_scale * 0.8;
        y1 = debug_start_y + kernagic_x_height () * scale_factor * debug_scale * 2.2;

        for (y = y0; y < y1; y++)
          {
            if (x >= 0 &&
                x < canvas_w &&
                y >= 0 &&
                y < canvas_h)
            kernagic_preview[y* canvas_w + x] =
              kernagic_preview[y* canvas_w + x] * 0.9 +
              255 * 0.1;
          }
      }
  }
}

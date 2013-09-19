#include <string.h>
#include "kernagic.h"
#include "kerner.h"

/* This is the main rendering code of kernagic.. 
 * 
 */
uint8_t *kernagic_preview = NULL;
#define MAX_BIG       1024

Glyph  *g_entries[MAX_BIG];
int     x_entries[MAX_BIG];
int     big = 0;

extern float  scale_factor;

gboolean toggle_measurement_lines = FALSE;

#define SCALE_DOWN 10

float place_glyph (Glyph *g, float xo, int yo, float opacity, float scale)
{
  int x, y;
  int xp = 5;
  int yp = 5;

  for (y = 0; y < g->r_height; y++)
    for (x = 0; x < g->r_width; x++)
      if (xo + (xp + x + g->left_bearing * scale_factor)* scale >= 0 &&
          xo + (xp + x + g->left_bearing * scale_factor)* scale < PREVIEW_WIDTH &&
          yp + yo + y* scale < PREVIEW_HEIGHT &&
          yp + yo + y* scale >= 0
          )
      {
        int val = kernagic_preview [(int)(yp + yo + ((y)* scale)) * PREVIEW_WIDTH + (int)(xo + (x + g->left_bearing * scale_factor)* scale) + xp]; 
        val += g->raster[y * g->r_width + x] * opacity * scale * scale;
        if (val > 255)
          val = 255;
        kernagic_preview [(int)(yp + yo + ((y) * scale)) * PREVIEW_WIDTH + (int)(xo + (x + g->left_bearing * scale_factor) * scale) + xp] = val;
      }

  return xo + kernagic_get_advance (g) * scale_factor * scale;
}

void place_glyph_debug (Glyph *g, float xo, int yo, float opacity, float scale, int debuglevel)
{
  int x, y;

  if (toggle_measurement_lines)
    {
      for (y = 0; y < g->r_height; y++)
        for (x = 0; x < 1; x++)
          if (x + xo >= 0 && x + xo < PREVIEW_WIDTH && y < PREVIEW_HEIGHT &&
          kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] == 0
              )
          kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 64;

      for (y = 0; y < PREVIEW_HEIGHT; y++)
        {
#if 0
          if (g->stem_count >=1)
            {
            x = g->stems[0] * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
            }

          if (g->stem_count > 1)
          {
            x = g->stems[g->stem_count-1] * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
#endif
          if (g->lstem > 0)
          {
            x = g->lstem * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH &&
                x + xo >= 0)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }

          if (g->rstem > 0)
          {
            x = g->rstem * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH &&
                x + xo >= 0)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
        }

      for (y = 0; y < PREVIEW_HEIGHT; y++)
        {
          if (g->lstem > 0)
          {
            x = g->lstem * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH &&
                x + xo >= 0)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
          if (g->rstem > 0)
          {
            x = g->rstem * scale_factor + g->left_bearing * scale_factor;
            if (x + xo < PREVIEW_WIDTH &&
                x + xo >= 0)
              kernagic_preview [y * PREVIEW_WIDTH + (int)(x + xo)] = 255;
          }
        }
    }
}

void redraw_test_text (const char *intext, const char *ipsum, int debuglevel)
{
  float period = kerner_settings.alpha_target;
  const char *utf8;
  gunichar *str2;
  int i;
  float x = 0;
  float y = 0;

  memset (kernagic_preview, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT);

  big = 0;
  utf8 = ipsum;
  str2 = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, NULL);
  if (str2)
  {
    Glyph *prev_g = NULL;
    float scale = 0.1;
    for (i = 0; str2[i]; i++)
      {
        Glyph *g = kernagic_find_glyph_unicode (str2[i]);
        if (g)
          {
            if (prev_g)
              x += kernagic_kern_get (prev_g, g) * scale_factor;

            x = place_glyph (g, x, y, 1.0, scale);
            prev_g = g;
          }
        else if (str2[i] == ' ') /* we're only faking it if we have to  */
          {
            Glyph *t = kernagic_find_glyph_unicode ('i');
            if (t)
              x += kernagic_get_advance (t) * scale_factor * scale;
            prev_g = NULL;
          }
        if (x > 800)
          {
            y += 512;
            x = 0;
          }
      }
      g_free (str2);
  }

  x = 0;
  y = 0;
  big = 0;
  utf8 = intext;
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

          place_glyph_debug (g, x, y, 1.0, 1.0, debuglevel);
          x = place_glyph (g, x, y, 1.0, 1.0);
          prev_g = g;
        }
      else if (str2[i] == ' ') /* we're only faking it if we have to  */
        {
          Glyph *t = kernagic_find_glyph_unicode ('i');
          if (t)
            x += kernagic_get_advance (t) * scale_factor;
          prev_g = NULL;
        }
      if (x > 8192)
        {
          y += 512;
          x = 0;
        }
    }
    g_free (str2);
  }

  /* should be based on n_width for table method */
  if (toggle_measurement_lines)
  {
    int i;
    for (i = 0; i * period * scale_factor < PREVIEW_WIDTH - period * scale_factor; i++)
      {
        int y;
        int x = (i + 0.5) * period * scale_factor;
        for (y= PREVIEW_HEIGHT*0.8; y < PREVIEW_HEIGHT*0.85; y++)
          {
            kernagic_preview[y* PREVIEW_WIDTH + x] =
              (kernagic_preview[y* PREVIEW_WIDTH + x] + 96) / 2;
          }
      }
  }
}

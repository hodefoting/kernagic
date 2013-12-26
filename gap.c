                                                                   /*
Kernagic a libre spacing tool for Unified Font Objects.
Copyright (C) 2013 Øyvind Kolås

Kernagicis free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Kernagic is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Kernagic.  If not, see <http://www.gnu.org/licenses/>.  */

#include <math.h>
#include <ctype.h>
#include "kernagic.h"

extern float scale_factor;

float left_most_center (Glyph *g);
float right_most_center (Glyph *g);
static float n_width = 0;

extern int kernagic_n_overrides;
extern int kernagic_override_unicode[256];
extern float kernagic_override_left[256];
extern float kernagic_override_right[256];

static void kernagic_gap_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  n_width = (right_most_center (g) - left_most_center(g)) * scale_factor;

  if (kernagic_n_overrides > 0)
  {
    for (int i = 0; i < kernagic_n_overrides; i++)
    {
      Glyph *g = kernagic_find_glyph_unicode (kernagic_override_unicode[i]);
      if (g)
      {
        g->lstem = g->ink_width * kernagic_override_left[i];
        g->rstem = g->ink_width * kernagic_override_right[i];
      }
    }
  }

}

float n_distance (void)
{
  float lstem;
  float rstem;
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return 100.0;

  lstem = g->stems[0];
  rstem = g->stems[g->stem_count-1];

  /* if manual overrides are set, use them */
  if (g->lstem > 0.0)
    lstem = g->lstem;
  if (g->rstem > 0.0)
    rstem = g->rstem;

  return rstem - lstem;
}

static void kernagic_gap_each (Glyph *g, GtkProgressBar *progress)
{
  float period = kerner_settings.snap;
  float gap = kerner_settings.gap * kernagic_x_height ();
  float left, right;
  float lstem;
  float rstem;

  lstem = g->stems[0];
  rstem = g->stems[g->stem_count-1];

  if (!islower (g->unicode))
    {
      if (g->ink_height >= kernagic_x_height () * 1.1)
        gap *= kerner_settings.big_glyph_scaling;
    }

  /* if manual overrides are set, use them */
  if (g->lstem > 0.0)
    lstem = g->lstem;
  if (g->rstem > 0.0)
    rstem = g->rstem;

  left = gap + period * (0.5) - lstem;

  /* can we come up with something better than ink_width here?.. */

  right = left + rstem + (gap) + period;
  right = ((int)(right/period))*period;
  right = right - (left + g->ink_width);
    
  left = left * kerner_settings.tracking / 100.0;
  right = right * kerner_settings.tracking / 100.0;

  kernagic_set_left_bearing (g, left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {"gap", kernagic_gap_init, kernagic_gap_each, NULL};
KernagicMethod *kernagic_gap = &method;


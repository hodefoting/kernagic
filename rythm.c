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
#include "kernagic.h"

extern float scale_factor;

float left_most_center (Glyph *g);
float right_most_center (Glyph *g);
static float n_width = 0;

static void kernagic_rythm_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  n_width = (right_most_center (g) - left_most_center(g)) * scale_factor;
}

static void kernagic_rythm_each (Glyph *g, GtkProgressBar *progress)
{
  float period = kerner_settings.snap;
  float gap = kerner_settings.gap;
  float rythm = 1;
  float left, right;
  float lstem;
  float rstem;

  if (rythm == 0)
    rythm = 1;

  lstem = g->stems[0];
  rstem = g->stems[g->stem_count-1];

  /* if manual overrides are set, use them */
  if (g->lstem > 0.0)
    lstem = g->lstem;
  if (g->rstem > 0.0)
    rstem = g->rstem;

  left = period * (gap + 0.5) - lstem;

  /* can we come up with something better than ink_width here?.. */

  right = left + rstem + (period * gap) + period;
  right = ((int)(right/period))*period;
  right = right - (left + g->ink_width);
    
  right = (period *rythm) - fmod (left + g->ink_width , period * rythm);

  left = left * kerner_settings.tracking / 100.0;
  right = right * kerner_settings.tracking / 100.0;

  kernagic_set_left_bearing (g, left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {"rythm", kernagic_rythm_init, kernagic_rythm_each, NULL};
KernagicMethod *kernagic_rythm = &method;


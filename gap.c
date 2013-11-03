#include <math.h>
#include <ctype.h>
#include "kernagic.h"

extern float scale_factor;

float left_most_center (Glyph *g);
float right_most_center (Glyph *g);
static float n_width = 0;

static void kernagic_gap_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  n_width = (right_most_center (g) - left_most_center(g)) * scale_factor;
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


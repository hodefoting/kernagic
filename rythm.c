#include <math.h>
#include "kernagic.h"
#include "kerner.h"

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
  float period = kerner_settings.alpha_target;
  float offset = kerner_settings.offset;
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

  left = period * (offset + 0.5) - lstem;

  /* can we come up with something better than ink_width here?.. */

  right = left + rstem + (period * offset) + period;
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


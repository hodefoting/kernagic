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
  float cadence = kerner_settings.alpha_target;
  float multiplier = kerner_settings.multiplier;
  float rythm = kerner_settings.fnord;
  float left, right;

  left = cadence * (multiplier+0.5) - g->stems[0];

  /* can we come up with something better than ink_width here?.. */
  right = cadence - fmod (left + g->ink_width , cadence);

  { /* sync in on rythm */
    float tw = left + g->ink_width + right;
    if (rythm != 0)
    while ( ((int)(tw/cadence) %( (int)rythm)) != 0)
        tw += cadence;
    right = tw - left - g->ink_width;
  }

  fprintf (stderr, "%f  %f  .... \n", 
      left + g->ink_width + right,
      fmod (left + g->ink_width + right, cadence));

  left = left * kerner_settings.tracking / 100.0;
  right = right * kerner_settings.tracking / 100.0;

  kernagic_set_left_bearing (g, left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {"rythm", kernagic_rythm_init, kernagic_rythm_each, NULL};
KernagicMethod *kernagic_rythm = &method;


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
  float left, right;
  float width;

  left = cadence * (multiplier+0.5) - g->stems[0];

  /* can we come up with something better than ink_width here?.. */
  width = g->stems[g->stem_count-1];//g->ink_width;
  //width = g->stems[g->stem_count-1];
  fprintf (stderr, "%i %f\n", (int)g->stem_count, g->stems[g->stem_count-1]);

  right = cadence - fmod (left + g->ink_width + right, cadence);

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


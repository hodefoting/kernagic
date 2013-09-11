#include <math.h>
#include "kernagic.h"
#include "kerner.h"

#if 0

dXXvXXX
X  X  X
X  X  X
X  X  X

X  X  X  X  X  X  X  X  X  X  X  X

X  X  X
X     X
X  X  X
X  X  X
X  XX X

.  .


X  X  X  X  X  X  X  X  X  X  X  X

X  X  X
X     X
X XX  X
X  X  X
X  X  X

X  X  X  X  X  X  X  X  X  X  X  X

X  X  X
X  X  X
X  X  X

X  X  X  X  X  X  X  X  X  X  X  X

X  dXXb  X
X  X  X  X
X  X  X  X

X  X  X  X  X  X  X  X  X  X  X  X

X  dXXb  X
X  X  X  X
X  YXXP  X

X  X  X  X  X  X  X  X  X  X  X  X

X  dXXvXXX  X
X  X  X  X  X
X  X  X  X  X

X  X  X  X  X  X  X  X  X  X  X  X


#endif



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
  float left, right;
  float width;

  left = cadence * 0.5 - g->stems[0];

  /* can we come up with something better than ink_width here?.. */
  width = g->ink_width;
  //width = g->stems[g->stem_count-1];
  right = cadence - fmod (left + width, cadence);

  kernagic_set_left_bearing (g, left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {"rythm", kernagic_rythm_init, kernagic_rythm_each, NULL};
KernagicMethod *kernagic_rythm = &method;


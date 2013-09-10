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


static void kernagic_rythm_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  if (kerner_settings.maximum_distance < 1)
   kerner_settings.maximum_distance = 1;
}

static void kernagic_rythm_each (Glyph *g, GtkProgressBar *progress)
{
  float left = 0;
  float right = 1;
  printf ("%s\n", g->name);

  kernagic_set_left_bearing (g,  left);
  kernagic_set_right_bearing (g, right);
}

static KernagicMethod method = {"rythm", kernagic_rythm_init, kernagic_rythm_each, NULL};
KernagicMethod *kernagic_rythm = &method;


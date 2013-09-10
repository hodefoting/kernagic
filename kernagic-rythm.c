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
  if (kerner_settings.maximum_distance < 1)
   kerner_settings.maximum_distance = 1;
}

static void kernagic_rythm_each (Glyph *g, GtkProgressBar *progress)
{
  float cadence = kerner_settings.alpha_target;
  printf ("%s\n", g->name);

  int   doubling;
  int   best_doubling;
  float best_left = 0;
  float best_right = 0;
  float best_total_dist = 10000000000000000.0;


  for (doubling = 1; doubling < 7; doubling ++)
    {
      float maxleft;
      float left;
      int postno;
#if 0
      if (cadence * (doubling - 2) < g->ink_width)
        continue;
#endif

      maxleft = doubling * cadence - g->ink_width;
      if (maxleft < cadence * 0.6)
        continue;

      for (left = 0; left < maxleft; left++)
      {
        float total_dist = 0;
        float right = maxleft - left;

        /* for each doubling, note how far it is from each rythm post to
         * a stem in the glyph considered
         */
        for (postno = 0; postno < doubling; postno++)
          {
            int stemno;
            //int beststem = -1;
            float bestdist = 1000000;
            float postpos = (postno+0.5) * cadence;

            if (g->stem_count)
            {
            for (stemno = 0; stemno < g->stem_count; stemno++)
              {
                float dist = fabs ((g->stems[stemno]+left)-postpos);
                if (dist < bestdist)
                {
                  bestdist = dist;
                }
              }
            total_dist += bestdist;
            }
          }
        if (total_dist < best_total_dist)
          {
            best_total_dist = total_dist;
            best_left = left;
            best_right = right;
            best_doubling = doubling;
          }
      }
    }

  fprintf (stderr, "%f %i %f %f %f %f\n", cadence, best_doubling, best_left, best_right, best_total_dist, n_width);

  kernagic_set_left_bearing (g, best_left);
  kernagic_set_right_bearing (g, best_right);
}

static KernagicMethod method = {"rythm", kernagic_rythm_init, kernagic_rythm_each, NULL};
KernagicMethod *kernagic_rythm = &method;


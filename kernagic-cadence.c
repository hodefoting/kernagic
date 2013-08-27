#include "kernagic.h"
#include "kerner.h"

void kernagic_cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  printf ("n-width: %f\n", g->ink_max_x - g->ink_min_x);
  printf ("n-width: %f %f\n", g->ink_max_x, g->ink_min_x);
  //return (g->max_y - g->min_y);
}

void kernagic_cadence_each (Glyph *lg, GtkProgressBar *progress)
{
  printf ("%s\n", lg->name);
}

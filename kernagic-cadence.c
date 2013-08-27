#include "kernagic.h"
#include "kerner.h"

void kernagic_cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  printf ("n-width: %f\n", g->max_x - g->min_x);
  printf ("n-width: %f %f\n", g->max_x, g->min_x);
  //return (g->max_y - g->min_y);
}

void kernagic_cadence_each (Glyph *lg, GtkProgressBar *progress)
{
  printf ("%s\n", lg->name);
}

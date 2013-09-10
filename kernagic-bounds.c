#include "kernagic.h"

void bounds_each (Glyph *lg, GtkProgressBar *progress)
{
  lg->right_bearing = 0;
  lg->left_bearing = 0;
  /* clear kerning? */
}


static KernagicMethod method = {"bounds", 
  NULL,  bounds_each, NULL};

KernagicMethod *kernagic_bounds = &method;

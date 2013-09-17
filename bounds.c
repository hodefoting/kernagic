#include "kernagic.h"

void bounds_each (Glyph *lg, GtkProgressBar *progress)
{
  /*
    we don't need to do anything, since this is the default

    lg->right_bearing = 0;
    lg->left_bearing = 0;
  */
}

static KernagicMethod method = {"bounds", 
  NULL,  bounds_each, NULL};

KernagicMethod *kernagic_bounds = &method;

#include "kernagic.h"

static void original_each (Glyph *g, GtkProgressBar *progress)
{
  g->right_bearing = g->right_original;
  g->left_bearing = g->left_original;
}

static KernagicMethod method = {"original", 
  NULL,  original_each, NULL};

KernagicMethod *kernagic_original = &method;

#include "kernagic.h"

static void original_each (Glyph *g, GtkProgressBar *progress)
{
  g->right_bearing = g->right_original * kerner_settings.tracking/100.0;
  g->left_bearing = g->left_original * kerner_settings.tracking/100.0;
}

static KernagicMethod method = {"original", 
  NULL,  original_each, NULL};

KernagicMethod *kernagic_original = &method;

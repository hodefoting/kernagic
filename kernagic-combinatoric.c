#include "kernagic.h"

#include "kerner.h"


void kernagic_combinatoric_each_left (Glyph *lg, GtkProgressBar *progress)
{
  GList *right;
  for (right = kernagic_glyphs (); right; right = right->next)
    {
      Glyph *rg = right->data;
      if (progress || kernagic_deal_with_glyphs (lg->unicode, rg->unicode))
        {
          /*XXX: kerner kern is wrong, this is method specific */
          float kerned_advance = kerner_kern (&kerner_settings, lg, rg);
          kernagic_kern_set (lg, rg, kerned_advance - lg->advance);
        }
    }
}

void kernagic_cadence_init (void)
{
  Glyph *g = kernagic_find_glyph_unicode ('n');
  if (!g)
    return;
  //return (g->max_y - g->min_y);
}



void kernagic_cadence_each_left (Glyph *lg, GtkProgressBar *progress)
{
  printf ("%s\n", lg->name);
}



















void kernagic_compute (GtkProgressBar *progress)
{
  long int total = g_list_length (kernagic_glyphs ());
  long int count = 0;
  GList *left;

  /* initialization `*/
  switch (kerner_settings.mode)
  {
    case KERNAGIC_CADENCE:
      kernagic_cadence_init ();
      break;
    case KERNAGIC_RYTHM:
      break;
    case KERNAGIC_GRAY:
    default:
      break;
  }


  for (left = kernagic_glyphs (); left; left = left->next)
  {
    Glyph *lg = left->data;
    if (progress)
    {
      float fraction = count / (float)total;
      gtk_progress_bar_set_fraction (progress, fraction);
      gtk_main_iteration_do (FALSE);
    }
  
    if (progress || kernagic_deal_with_glyph (lg->unicode))
    {
      switch (kerner_settings.mode)
      {
        case KERNAGIC_CADENCE:
          kernagic_cadence_each_left (lg, progress);
          break;
        case KERNAGIC_RYTHM:
          fprintf (stderr, "missing linear iterator\n");
          break;
        case KERNAGIC_GRAY:
        default:
          kernagic_combinatoric_each_left (lg, progress);
          break;
      }
    }
    count ++;
  }
}

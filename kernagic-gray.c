#include "kernagic.h"
#include "kerner.h"


void kernagic_gray_each (Glyph *lg, GtkProgressBar *progress)
{
  GList *right;
  for (right = kernagic_glyphs (); right; right = right->next)
    {
      Glyph *rg = right->data;
      if (progress || kernagic_deal_with_glyphs (lg->unicode, rg->unicode))
        {
          /*XXX: kerner kern is wrong, this is method specific */
          float kerned_advance = kerner_kern (&kerner_settings, lg, rg);

          kernagic_set_advance (lg, lg->ink_width);
          kernagic_set_kerning (lg, rg, kerned_advance - lg->advance);
        }
    }
}

void kernagic_gray_init (void)
{
}


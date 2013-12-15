                                                                   /*
Kernagic a libre spacing tool for Unified Font Objects.
Copyright (C) 2013 Øyvind Kolås

Kernagicis free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Kernagic is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Kernagic.  If not, see <http://www.gnu.org/licenses/>.  */


#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "kernagic.h"

/* A little comment about the ufo data-directory format (it isn't really a
 * file format; but using the file system directly for the structure of a
 * composite media object is a lot better than what most applications do).
 *
 * The painful part is the use of XML plists. The desire to do something clean
 * for adding user data has wasted too many hours; the amount of code it would
 * take to do something this trivial has prevented me from properly starting
 * adding the feature. Even with a SAX like parser at my disposal the format
 * is ugly and unapproachable; not willing to change this until UFO changes
 * the .plists for something with less technical cost/debt.
 *
 * I suspect other .plist dealing code in kernagic has a bug^Wmalfeature -
 * that likely is masked by painlist parsers; as long as it works those .plist
 * files are no less broken than the format already is.
 */

static GString *ts = NULL;

#define KEY "com.hodefoting.kernagic.1.0"


int kernagic_libplist_read (const char *path)
{
  gchar *input = NULL;
  int ret = 0;

  g_file_get_contents (path, &input, NULL, NULL);
  if (!input)
    {
      return 0;
    }

  if (strstr (input, KEY))
    {
      gchar *p = strstr (input, KEY);
      while (*p != '>') p++;
      p++;
      while (*p != '>') p++;
      p++;
      float a, b, c;
      sscanf (p, "gap=%f snap=%f bigscale=%f", &a, &b, &c);

      kerner_settings.gap = a;
      kerner_settings.snap = b;
      if (c != 0)
        kerner_settings.big_glyph_scaling = c;

      ret = 1;
    }
  g_free (input);
  return ret;
}

void kernagic_libplist_rewrite (const char *path)
{
  gchar *input = NULL;
  g_file_get_contents (path, &input, NULL, NULL);
  if (!input)
    {
      input = g_strdup ("<?xml version='1.0' encoding='UTF-8'?> <!DOCTYPE plist PUBLIC '-//Apple Computer//DTD PLIST 1.0//EN' 'http://www.apple.com/DTDs/PropertyList-1.0.dtd'> <plist version='1.0'><dict></dict></plist>");
    }

  if (strstr (input, KEY))
    {
      /* erase prior data */
      gchar *p = strstr (input, KEY);
      while (*p != '<') p--;
      if (!strchr (p, '^'))
        return;
      while (*p != '^')
        {
          *p = ' ';
          p++;
        }
      while (*p != '>')
        {
          *p = ' ';
          p++;
        }
      *p = ' ';
    }

  {
    /* remove </dict></plist> */
    gchar *p = &input[strlen(input)];
    while (*p != '<')
    {
      *p = 0;
      p--;
    }
    *p = 0;
    while (*p != '<')
    {
      *p = 0;
      p--;
    }
    *p = 0;
    p--;
    while (*p == '\n' || *p == ' ')
    {
      *p = 0;
      p--;
    }
  }

  ts = g_string_new ("");

  g_string_append (ts, input);

  g_string_append_printf (ts, "\n<key>%s</key>\n", KEY);
  g_string_append_printf (ts, "<string>gap=%f snap=%f bigscale=%f^</string>\n",
      kerner_settings.gap,
      kerner_settings.snap,
      kerner_settings.big_glyph_scaling);
  g_string_append (ts, "</dict>\n</plist>\n");

  g_file_set_contents (path, ts->str, ts->len, NULL);
  g_string_free (ts, TRUE);
  ts = NULL;

  g_free (input);
  return;
}

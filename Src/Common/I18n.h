// I18n.h
//



#ifndef __i18n_h__
#define __i18n_h__

#if defined(_WIN32) || defined(__APPLE__)

#define _(szText) szText
#define N_(szText) (szText)

#else

#include <libintl.h>
// Attempt to get rid of '"_" redefined' compiler warnings.  I'm not sure the
// proper way to verify that gnome i18n support is present, so if you have a
// better idea...
#ifndef _
#define _(szText) gettext(szText)
#endif

#ifndef N_
#define N_(szText) (szText)
#endif

#endif

#endif

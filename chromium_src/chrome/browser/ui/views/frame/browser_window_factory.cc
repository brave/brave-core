#include "chrome/browser/ui/views/frame/browser_view.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"

// For now, BraveBrowserFrame only needs to run on !linux
// and build complains of unused variables if we include it.
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
#include "brave/browser/ui/views/frame/brave_browser_frame.h"
#define BrowserFrame BraveBrowserFrame
#endif

#define BrowserView BraveBrowserView
#include "../../../../../../../chrome/browser/ui/views/frame/browser_window_factory.cc"
#undef BrowserView

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
#undef BrowserFrame
#endif

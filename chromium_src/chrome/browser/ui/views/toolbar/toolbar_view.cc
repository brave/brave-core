#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"

#define LocationBarView BraveLocationBarView
#include "../../../../../../../chrome/browser/ui/views/toolbar/toolbar_view.cc"
#undef LocationBarView

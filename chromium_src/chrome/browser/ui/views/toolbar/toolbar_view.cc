#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/views/profiles/avatar_toolbar_button.h"
#include "brave/browser/ui/views/profiles/brave_avatar_toolbar_button.h"

#define LocationBarView BraveLocationBarView
#define AvatarToolbarButton BraveAvatarToolbarButton
#include "../../../../../../../chrome/browser/ui/views/toolbar/toolbar_view.cc"
#undef LocationBarView
#undef AvatarToolbarButton

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
// include brave_tab header so that patch inside tab_strip works
#include "brave/browser/ui/views/tabs/brave_tab.h"

#define NewTabButton BraveNewTabButton
#include "../../../../../../chrome/browser/ui/views/tabs/tab_strip.cc"
#undef NewTabButton

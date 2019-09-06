#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

#define GoogleApiKeysInfoBarDelegate BraveGoogleKeysInfoBarDelegate

class BraveGoogleKeysInfoBarDelegate {
 public:
   static void Create(InfoBarService* infobar_service) {
     // lulz
   }
};

#define BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH \
  onboarding_enabled = true;

#include "../../../../../../chrome/browser/ui/startup/startup_browser_creator_impl.cc"

#undef BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH
#undef GoogleApiKeysInfoBarDelegate

#include "brave/vendor/brave-ios/components/Passphrase.h"

#include <string>
#include <vector>

#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

@interface PassPhrase()
{

}
@end

@implementation PassPhrase
- (NSString *)getSyncCode {
	brave_sync::Prefs brave_sync_prefs{BrowserStateManager::GetInstance().GetBrowserState()->GetPrefs()};
	std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    std::string sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
    brave_sync_prefs.SetSeed(sync_code);
    return base::SysUTF8ToNSString(sync_code.c_str());
}
@end
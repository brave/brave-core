
#include "brave/vendor/brave-ios/components/browser_state/browser_state_otr_helper.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

namespace brave {
web::BrowserState* GetBrowserStateRedirectedInIncognito(
    web::BrowserState* browser_state) {
  return static_cast<ChromeBrowserState*>(browser_state)
      ->GetOriginalChromeBrowserState();
}

web::BrowserState* GetBrowserStateOwnInstanceInIncognito(
    web::BrowserState* browser_state) {
  return browser_state;
}
}

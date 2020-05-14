#include "brave/vendor/brave-ios/components/browser_state/browser_state_otr_helper.h"

#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"
#include "ios/web/public/browser_state.h"

web::BrowserState* GetBrowserStateRedirectedInIncognito(
    web::BrowserState* browser_state) {
  // TODO(bridiver)
  // return static_cast<ChromeBrowserState*>(browser_state)
  //     ->GetOriginalChromeBrowserState();
  return browser_state;
}

web::BrowserState* GetBrowserStateOwnInstanceInIncognito(
    web::BrowserState* browser_state) {
  return browser_state;
}

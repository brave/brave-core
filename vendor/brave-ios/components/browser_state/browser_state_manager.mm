#include "brave/vendor/brave-ios/components/browser_state/browser_state_manager.h"
#include "brave/vendor/brave-ios/components/browser_state/chrome_browser_state.h"

#include "base/path_service.h"

BrowserStateManager& BrowserStateManager::instance() {
  static BrowserStateManager instance;
  return instance;
}

BrowserStateManager::BrowserStateManager() : browser_state_(nullptr) {}

BrowserStateManager::~BrowserStateManager() {}

ChromeBrowserState* BrowserStateManager::getBrowserState() {
  if (!browser_state_) {
    browser_state_ = std::make_unique<ChromeBrowserState>(base::FilePath(kIOSChromeInitialBrowserState));
  }
  CHECK(browser_state_.get());
  return browser_state_.get();
}

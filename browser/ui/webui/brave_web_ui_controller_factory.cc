/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_web_ui_controller_factory.h"

#include "brave/common/webui_url_constants.h"
#include "brave/browser/ui/webui/brave_adblock_ui.h"
#include "brave/browser/ui/webui/brave_md_settings_ui.h"
#include "brave/browser/ui/webui/brave_new_tab_ui.h"
#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include "brave/browser/ui/webui/sync/sync_ui_impl.h"

#include "brave/browser/ui/webui/brave_welcome_ui.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "chrome/common/url_constants.h"
#include "components/grit/brave_components_resources.h"
#include "url/gurl.h"

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/browser/ui/webui/brave_rewards_ui.h"
#endif

using content::WebUI;
using content::WebUIController;

namespace {

// A function for creating a new WebUI. The caller owns the return value, which
// may be NULL (for example, if the URL refers to an non-existent extension).
typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template<class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

template<>
WebUIController* NewWebUI<BasicUI>(WebUI* web_ui, const GURL& url) {
  auto host = url.host_piece();
  if (host == kBraveUISyncHost) {
    return new SyncUIImpl(web_ui, url.host(), kBraveSyncJS,
        IDR_BRAVE_SYNC_JS, IDR_BRAVE_SYNC_HTML);
  } else
  if (host == kAdblockHost) {
    return new BraveAdblockUI(web_ui, url.host());
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  } else if (host == kRewardsHost) {
    return new BraveRewardsUI(web_ui, url.host());
#endif
  } else if (host == kWelcomeHost) {
    return new BraveWelcomeUI(web_ui, url.host());
  } else if (host == chrome::kChromeUINewTabHost) {
    return new BraveNewTabUI(web_ui, url.host());
  } else if (host == chrome::kChromeUISettingsHost) {
    return new BraveMdSettingsUI(web_ui, url.host());
  }
  return nullptr;
}

// Returns a function that can be used to create the right type of WebUI for a
// tab, based on its URL. Returns NULL if the URL doesn't have WebUI associated
// with it.
WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui,
                                             const GURL& url) {
  if (url.host_piece() == kAdblockHost ||
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
      url.host_piece() == kRewardsHost ||
#endif
      url.host_piece() == kWelcomeHost ||
      url.host_piece() == kBraveUIWelcomeURL ||
      url.host_piece() == kBraveUISyncHost ||
      url.host_piece() == chrome::kChromeUINewTabHost ||
      url.host_piece() == chrome::kChromeUISettingsHost) {
    return &NewWebUI<BasicUI>;
  }

  return nullptr;
}

}  // namespace

WebUI::TypeID BraveWebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const {

  WebUIFactoryFunction function = GetWebUIFactoryFunction(NULL, url);
  if (function) {
    return reinterpret_cast<WebUI::TypeID>(function);
  }
  return ChromeWebUIControllerFactory::GetWebUIType(browser_context, url);
}

std::unique_ptr<WebUIController>
BraveWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {

  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, url);
  if (!function) {
    return ChromeWebUIControllerFactory::CreateWebUIControllerForURL(
        web_ui, url);
  }

  return base::WrapUnique((*function)(web_ui, url));
}


// static
BraveWebUIControllerFactory* BraveWebUIControllerFactory::GetInstance() {
  return base::Singleton<BraveWebUIControllerFactory>::get();
}

BraveWebUIControllerFactory::BraveWebUIControllerFactory() {
}

BraveWebUIControllerFactory::~BraveWebUIControllerFactory() {
}

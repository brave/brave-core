/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/browser_url_handler.h"


namespace {

bool HandleNewTabURLRewrite(GURL* url,
                            content::BrowserContext* browser_context) {
  if (!url->SchemeIs(content::kChromeUIScheme) ||
      url->host() != chrome::kChromeUINewTabHost)
    return false;

  *url = GURL(kBraveNewTabUrl);
  return true;
}

bool HandleNewTabURLReverseRewrite(GURL* url,
                                   content::BrowserContext* browser_context) {
  // Do nothing in incognito.
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile && profile->IsOffTheRecord())
    return false;

  if (url->spec() == kBraveNewTabUrl) {
    *url = GURL(kBraveNewTabUrl);
    return true;
  }

  return false;
}

}

BraveContentBrowserClient::BraveContentBrowserClient() {}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

content::BrowserMainParts* BraveContentBrowserClient::CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) {
  ChromeBrowserMainParts* main_parts = (ChromeBrowserMainParts*)
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  main_parts->AddParts(new BraveBrowserMainExtraParts());
  return main_parts;
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(content::BrowserURLHandler* handler) {
  // Handler to rewrite chrome://newtab
  handler->AddHandlerPair(&HandleNewTabURLRewrite,
                          &HandleNewTabURLReverseRewrite);
  ChromeContentBrowserClient::BrowserURLHandlerCreated(handler);
}

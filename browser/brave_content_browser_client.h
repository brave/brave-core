/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

#include "chrome/browser/chrome_content_browser_client.h"

class BraveContentBrowserClient : public ChromeContentBrowserClient {
 public:
  BraveContentBrowserClient();
  ~BraveContentBrowserClient() override;

   // Overridden from ChromeContentBrowserClient:
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;
  std::unique_ptr<content::NavigationUIData> GetNavigationUIData(
      content::NavigationHandle* navigation_handle) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentBrowserClient);
};

#endif  // BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

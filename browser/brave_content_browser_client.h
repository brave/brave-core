/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

#include "chrome/browser/chrome_content_browser_client.h"
#include "content/public/browser/content_browser_client.h"

#include <memory>

class BraveContentBrowserClient : public ChromeContentBrowserClient {
 public:
  BraveContentBrowserClient(std::unique_ptr<ui::DataPack> data_pack = nullptr,
      ChromeFeatureListCreator* chrome_feature_list_creator = nullptr);
  ~BraveContentBrowserClient() override;

   // Overridden from ChromeContentBrowserClient:
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;
  bool AllowGetCookie(const GURL& url,
                      const GURL& first_party,
                      const net::CookieList& cookie_list,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id) override;
  bool AllowSetCookie(const GURL& url,
                      const GURL& first_party,
                      const net::CanonicalCookie& cookie,
                      content::ResourceContext* context,
                      int render_process_id,
                      int render_frame_id) override;

  bool HandleExternalProtocol(
      const GURL& url,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture) override;

  content::ContentBrowserClient::AllowWebBluetoothResult AllowWebBluetooth(
      content::BrowserContext* browser_context,
      const url::Origin& requesting_origin,
      const url::Origin& embedding_origin) override;

  void RegisterOutOfProcessServices(
      OutOfProcessServiceMap* services) override;

  std::unique_ptr<content::NavigationUIData> GetNavigationUIData(
      content::NavigationHandle* navigation_handle) override;
  void AdjustUtilityServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveContentBrowserClient);
};

#endif  // BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "brave/browser/brave_browser_main_extra_parts.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_webtorrent/browser/content_browser_client_helper.h"
#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/common/url_constants.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"

using content::BrowserThread;
using content::RenderFrameHost;
using content::WebContents;
using content_settings::BraveCookieSettings;
using brave_shields::BraveShieldsWebContentsObserver;

namespace {

bool HandleURLRewrite(GURL* url,
                      content::BrowserContext* browser_context) {
  if (url->SchemeIs(content::kChromeUIScheme) &&
      (url->host() == chrome::kChromeUIWelcomeHost ||
       url->host() == chrome::kChromeUIWelcomeWin10Host)) {
    *url = GURL(kBraveUIWelcomeURL);
    return true;
  }

  if (url->SchemeIs(content::kChromeUIScheme) &&
      url->host() == chrome::kChromeUINewTabHost) {
    // Disable new tab overrides, but keep it the same
    return true;
  }

  return false;
}

bool HandleURLReverseRewrite(GURL* url,
                             content::BrowserContext* browser_context) {
  // Handle mapping new tab URL to ourselves
  if (url->SchemeIs(content::kChromeUIScheme) &&
      url->host() == chrome::kChromeUINewTabHost) {
    return true;
  }
  if (url->spec() == kBraveUIWelcomeURL) {
    return true;
  }
  return false;
}

WebContents* GetWebContents(int render_process_id, int render_frame_id) {
  RenderFrameHost* rfh =
      RenderFrameHost::FromID(render_process_id, render_frame_id);
  return WebContents::FromRenderFrameHost(rfh);
}

}

BraveContentBrowserClient::BraveContentBrowserClient(std::unique_ptr<ui::DataPack> data_pack,
    ChromeFeatureListCreator* chrome_feature_list_creator) :
    ChromeContentBrowserClient(std::move(data_pack), chrome_feature_list_creator)
{}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

content::BrowserMainParts* BraveContentBrowserClient::CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) {
  ChromeBrowserMainParts* main_parts = (ChromeBrowserMainParts*)
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  main_parts->AddParts(new BraveBrowserMainExtraParts());
  return main_parts;
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
  // Insert handler for chrome://newtab so that we handle it
  // before anything else can.
  handler->AddHandlerPair(&webtorrent::HandleMagnetURLRewrite,
                          content::BrowserURLHandler::null_handler());
  handler->AddHandlerPair(&HandleURLRewrite,
                          &HandleURLReverseRewrite);
  ChromeContentBrowserClient::BrowserURLHandlerCreated(handler);
}

bool BraveContentBrowserClient::AllowGetCookie(
    const GURL& url,
    const GURL& first_party,
    const net::CookieList& cookie_list,
    content::ResourceContext* context,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(context);

  GURL tab_url = BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
      render_process_id, render_frame_id);
  BraveCookieSettings* cookie_settings =
      (BraveCookieSettings*)io_data->GetCookieSettings();

  bool allow =
      cookie_settings->IsCookieAccessAllowed(url, first_party, tab_url);

  base::Callback<WebContents*(void)> wc_getter =
      base::Bind(&GetWebContents, render_process_id, render_frame_id);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&TabSpecificContentSettings::CookiesRead, wc_getter, url,
                     first_party, cookie_list, !allow));
  return allow;
}

content::ContentBrowserClient::AllowWebBluetoothResult
BraveContentBrowserClient::AllowWebBluetooth(
    content::BrowserContext* browser_context,
    const url::Origin& requesting_origin,
    const url::Origin& embedding_origin) {
  return content::ContentBrowserClient::AllowWebBluetoothResult::BLOCK_GLOBALLY_DISABLED;
}

bool BraveContentBrowserClient::HandleExternalProtocol(
    const GURL& url,
    content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
    int child_id,
    content::NavigationUIData* navigation_data,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture) {

  if (webtorrent::HandleMagnetProtocol(url, web_contents_getter,
        page_transition, has_user_gesture)) {
    return true;
  }

  return ChromeContentBrowserClient::HandleExternalProtocol(
      url, web_contents_getter, child_id, navigation_data, is_main_frame,
      page_transition, has_user_gesture);
}

bool BraveContentBrowserClient::AllowSetCookie(
    const GURL& url,
    const GURL& first_party,
    const net::CanonicalCookie& cookie,
    content::ResourceContext* context,
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(context);

  BraveCookieSettings* cookie_settings =
      (BraveCookieSettings*)io_data->GetCookieSettings();

  GURL tab_url = BraveShieldsWebContentsObserver::GetTabURLFromRenderFrameInfo(
      render_process_id, render_frame_id);

  bool allow =
    cookie_settings->IsCookieAccessAllowed(url, first_party, tab_url);

  base::Callback<WebContents*(void)> wc_getter =
      base::Bind(&GetWebContents, render_process_id, render_frame_id);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&TabSpecificContentSettings::CookieChanged, wc_getter, url,
                     first_party, cookie, !allow));
  return allow;
}

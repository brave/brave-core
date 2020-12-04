/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "third_party/blink/public/mojom/loader/referrer.mojom.h"

class PrefChangeRegistrar;

namespace content {
class BrowserContext;
}

class BraveContentBrowserClient : public ChromeContentBrowserClient {
 public:
  BraveContentBrowserClient();
  ~BraveContentBrowserClient() override;

  // Overridden from ChromeContentBrowserClient:
  std::unique_ptr<content::BrowserMainParts> CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;

  bool HandleExternalProtocol(
      const GURL& url,
      content::WebContents::OnceGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      const base::Optional<url::Origin>& initiating_origin,
      mojo::PendingRemote<network::mojom::URLLoaderFactory>* out_factory)
      override;

  content::ContentBrowserClient::AllowWebBluetoothResult AllowWebBluetooth(
      content::BrowserContext* browser_context,
      const url::Origin& requesting_origin,
      const url::Origin& embedding_origin) override;

  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;

  std::vector<std::unique_ptr<blink::URLLoaderThrottle>>
  CreateURLLoaderThrottles(
      const network::ResourceRequest& request,
      content::BrowserContext* browser_context,
      const base::RepeatingCallback<content::WebContents*()>& wc_getter,
      content::NavigationUIData* navigation_ui_data,
      int frame_tree_node_id) override;

  bool WillCreateURLLoaderFactory(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* frame,
      int render_process_id,
      URLLoaderFactoryType type,
      const url::Origin& request_initiator,
      base::Optional<int64_t> navigation_id,
      ukm::SourceIdObj ukm_source_id,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory>* factory_receiver,
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
          header_client,
      bool* bypass_redirect_checks,
      bool* disable_secure_dns,
      network::mojom::URLLoaderFactoryOverridePtr* factory_override) override;

  bool WillInterceptWebSocket(content::RenderFrameHost* frame) override;
  void CreateWebSocket(
      content::RenderFrameHost* frame,
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const base::Optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client) override;

  void MaybeHideReferrer(content::BrowserContext* browser_context,
                         const GURL& request_url,
                         const GURL& document_url,
                         bool is_main_frame,
                         const std::string& method,
                         blink::mojom::ReferrerPtr* referrer) override;

  GURL GetEffectiveURL(content::BrowserContext* browser_context,
                       const GURL& url) override;
  static bool HandleURLOverrideRewrite(GURL* url,
      content::BrowserContext* browser_context);
  std::vector<std::unique_ptr<content::NavigationThrottle>>
      CreateThrottlesForNavigation(content::NavigationHandle* handle) override;

 private:
  uint64_t session_token_;
  uint64_t incognito_session_token_;

  void OnAllowGoogleAuthChanged();

  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BraveContentBrowserClient);
};

#endif  // BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

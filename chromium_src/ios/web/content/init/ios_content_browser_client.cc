// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/content/init/ios_content_browser_client.cc>

#include <string_view>

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/ios/web/content/shields_settings_sender.h"
#include "content/public/browser/browser_url_handler.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace web {

namespace {

// Content blocks about: URLs other than about:blank, but the iOS UI keys its
// native NTP off the about:newtab virtual URL.
bool RewriteAboutNewTab(GURL* url, content::BrowserContext*) {
  if (!url->SchemeIs(url::kAboutScheme)) {
    return false;
  }
  std::string_view content = url->GetContentPiece();
  if (base::TrimString(content, "/", base::TRIM_LEADING) != "newtab") {
    return false;
  }
  *url = GURL(url::kAboutBlankURL);
  return true;
}

SubresourceBlocker& GetSubresourceBlocker() {
  static base::NoDestructor<SubresourceBlocker> blocker;
  return *blocker;
}

// Sits in front of a subresource factory and fails requests the blocker
// rejects. Owns itself and goes away once either side disconnects.
class BlockingURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  static void Create(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target) {
    new BlockingURLLoaderFactory(std::move(receiver), std::move(target));
  }

  BlockingURLLoaderFactory(const BlockingURLLoaderFactory&) = delete;
  BlockingURLLoaderFactory& operator=(const BlockingURLLoaderFactory&) =
      delete;

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    const auto& blocker = GetSubresourceBlocker();
    if (blocker && blocker.Run(request)) {
      mojo::Remote<network::mojom::URLLoaderClient>(std::move(client))
          ->OnComplete(
              network::URLLoaderCompletionStatus(net::ERR_BLOCKED_BY_CLIENT));
      return;
    }
    target_->CreateLoaderAndStart(std::move(loader), request_id, options,
                                  request, std::move(client),
                                  traffic_annotation);
  }

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  BlockingURLLoaderFactory(
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      mojo::PendingRemote<network::mojom::URLLoaderFactory> target)
      : target_(std::move(target)) {
    receivers_.Add(this, std::move(receiver));
    receivers_.set_disconnect_handler(base::BindRepeating(
        &BlockingURLLoaderFactory::OnReceiverDisconnect,
        base::Unretained(this)));
    target_.set_disconnect_handler(base::BindOnce(
        &BlockingURLLoaderFactory::Destroy, base::Unretained(this)));
  }
  ~BlockingURLLoaderFactory() override = default;

  void OnReceiverDisconnect() {
    if (receivers_.empty()) {
      Destroy();
    }
  }

  void Destroy() { delete this; }

  mojo::ReceiverSet<network::mojom::URLLoaderFactory> receivers_;
  mojo::Remote<network::mojom::URLLoaderFactory> target_;
};

}  // namespace

void SetSubresourceBlocker(SubresourceBlocker blocker) {
  GetSubresourceBlocker() = std::move(blocker);
}

void IOSContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
  handler->AddHandlerPair(&RewriteAboutNewTab,
                          content::BrowserURLHandler::null_handler());
}

void IOSContentBrowserClient::WillCreateURLLoaderFactory(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* frame,
    int render_process_id,
    URLLoaderFactoryType type,
    const url::Origin& request_initiator,
    const net::IsolationInfo& isolation_info,
    std::optional<int64_t> navigation_id,
    ukm::SourceIdObj ukm_source_id,
    network::URLLoaderFactoryBuilder& factory_builder,
    mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*
        header_client,
    bool* bypass_redirect_checks,
    bool* disable_secure_dns,
    network::mojom::URLLoaderFactoryOverridePtr* factory_override,
    scoped_refptr<base::SequencedTaskRunner> navigation_response_task_runner,
    bool is_for_network_service) {
  if (type != URLLoaderFactoryType::kDocumentSubResource &&
      type != URLLoaderFactoryType::kWorkerSubResource &&
      type != URLLoaderFactoryType::kServiceWorkerSubResource) {
    return;
  }
  auto [receiver, target] = factory_builder.Append();
  BlockingURLLoaderFactory::Create(std::move(receiver), std::move(target));
}

void IOSContentBrowserClient::OnWebContentsCreated(
    content::WebContents* web_contents) {
  brave_shields::ShieldsSettingsSender::CreateForWebContents(web_contents);
}

}  // namespace web

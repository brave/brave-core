/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/base64url.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/components/brave_shields/browser/ad_block_custom_filters_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/common/url_pattern.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/network_context.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave {

void ShouldBlockAdOnTaskRunner(std::shared_ptr<BraveRequestInfo> ctx) {
  bool did_match_exception = false;
  std::string tab_host = ctx->tab_origin.host();
  if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
          ctx->request_url, ctx->resource_type, tab_host, &did_match_exception,
          &ctx->cancel_request_explicitly, &ctx->mock_data_url)) {
    ctx->blocked_by = kAdBlocked;
  } else if (!did_match_exception &&
             !g_brave_browser_process->ad_block_regional_service_manager()
                  ->ShouldStartRequest(ctx->request_url, ctx->resource_type,
                                       tab_host, &did_match_exception,
                                       &ctx->cancel_request_explicitly,
                                       &ctx->mock_data_url)) {
    ctx->blocked_by = kAdBlocked;
  } else if (!did_match_exception &&
             !g_brave_browser_process->ad_block_custom_filters_service()
                  ->ShouldStartRequest(ctx->request_url, ctx->resource_type,
                                       tab_host, &did_match_exception,
                                       &ctx->cancel_request_explicitly,
                                       &ctx->mock_data_url)) {
    ctx->blocked_by = kAdBlocked;
  }
}

void OnShouldBlockAdResult(const ResponseCallback& next_callback,
                           std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (ctx->blocked_by == kAdBlocked) {
    brave_shields::DispatchBlockedEvent(
        ctx->request_url, ctx->render_frame_id, ctx->render_process_id,
        ctx->frame_tree_node_id, brave_shields::kAds);
  }
  next_callback.Run();
}

void OnGetCnameResult(const ResponseCallback& next_callback,
                      std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      g_brave_browser_process->ad_block_service()->GetTaskRunner();
  // TODO - causes DCHECK failures in ref_counted.h
  task_runner->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&ShouldBlockAdOnTaskRunner, ctx),
      base::BindOnce(&OnShouldBlockAdResult, next_callback, ctx));
}

class AdblockCnameResolveHostClient : public network::mojom::ResolveHostClient {
 private:
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};
  base::OnceCallback<void()> cb_;

 public:
  AdblockCnameResolveHostClient(
      const ResponseCallback& next_callback,
      std::shared_ptr<BraveRequestInfo> ctx) {
    cb_ = base::Bind(&OnGetCnameResult, next_callback, ctx);

    content::BrowserContext* context =
        content::RenderProcessHost::FromID(ctx->render_process_id)
            ->GetBrowserContext();

    auto* rfh = content::RenderFrameHost::FromID(ctx->render_process_id,
                                                 ctx->render_frame_id);
    const net::NetworkIsolationKey network_isolation_key =
        rfh->GetNetworkIsolationKey();

    network::mojom::ResolveHostParametersPtr optional_parameters =
        network::mojom::ResolveHostParameters::New();
    optional_parameters->include_canonical_name = true;

    network::mojom::NetworkContext* network_context =
        content::BrowserContext::GetDefaultStoragePartition(context)
            ->GetNetworkContext();
    network_context->ResolveHost(
        net::HostPortPair::FromURL(ctx->request_url), network_isolation_key,
        std::move(optional_parameters), receiver_.BindNewPipeAndPassRemote());

    receiver_.set_disconnect_handler(
        base::BindOnce(&AdblockCnameResolveHostClient::OnComplete,
                       base::Unretained(this), net::ERR_NAME_NOT_RESOLVED,
                       net::ResolveErrorInfo(net::ERR_FAILED), base::nullopt));
  }

  void OnComplete(
      int32_t result,
      const net::ResolveErrorInfo& resolve_error_info,
      const base::Optional<net::AddressList>& resolved_addresses) override {
    if (result == net::OK && resolved_addresses) {
      DCHECK(resolved_addresses.has_value() && !resolved_addresses->empty());
      // TODO - add the canonical name to ctx
      LOG(ERROR) << resolved_addresses->canonical_name();
      std::move(cb_).Run();
    } else {
      // TODO - this brings up a lot of `-2` (generic failure)
      LOG(ERROR) << "Could not resolve: " << resolve_error_info.error;
    }

    delete this;
  }

  // Should not be called
  void OnTextResults(const std::vector<std::string>& text_results) override {
    DCHECK(false);
  }

  // Should not be called
  void OnHostnameResults(const std::vector<net::HostPortPair>& hosts) override {
    DCHECK(false);
  }
};

void OnBeforeURLRequestAdBlockTP(const ResponseCallback& next_callback,
                                 std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->tab_origin.has_host() ||
      ctx->request_url.is_empty()) {
    return;
  }
  DCHECK_NE(ctx->request_identifier, 0UL);

  auto* rph = content::RenderProcessHost::FromID(ctx->render_process_id);
  if (!rph) {
    // This case occurs for the top-level request. It's okay to ignore the
    // canonical name here since the top-level request can't be blocked
    // anyways.
    g_brave_browser_process->ad_block_service()
        ->GetTaskRunner()
        ->PostTaskAndReply(
            FROM_HERE, base::BindOnce(&ShouldBlockAdOnTaskRunner, ctx),
            base::BindOnce(&OnShouldBlockAdResult, next_callback, ctx));
  } else {
     new AdblockCnameResolveHostClient(next_callback, ctx);
  }
}

int OnBeforeURLRequest_AdBlockTPPreWork(const ResponseCallback& next_callback,
                                        std::shared_ptr<BraveRequestInfo> ctx) {
  if (ctx->request_url.is_empty()) {
    return net::OK;
  }

  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->tab_origin.is_empty() || !ctx->allow_brave_shields ||
      ctx->allow_ads ||
      ctx->resource_type == BraveRequestInfo::kInvalidResourceType) {
    return net::OK;
  }

  OnBeforeURLRequestAdBlockTP(next_callback, ctx);

  return net::ERR_IO_PENDING;
}

}  // namespace brave

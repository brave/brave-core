/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/url_pattern.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/network_context.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/url_canon.h"

namespace brave {

namespace {

content::WebContents* GetWebContents(int render_process_id,
                                     int render_frame_id,
                                     int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents) {
    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (!rfh) {
      return nullptr;
    }
    web_contents = content::WebContents::FromRenderFrameHost(rfh);
  }
  return web_contents;
}

}  // namespace

void ShouldBlockAdOnTaskRunner(std::shared_ptr<BraveRequestInfo> ctx,
                               base::Optional<std::string> canonical_name) {
  bool did_match_exception = false;
  if (!ctx->initiator_url.is_valid()) {
    return;
  }
  std::string source_host = ctx->initiator_url.host();
  if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
          ctx->request_url, ctx->resource_type, source_host,
          &did_match_exception, &ctx->mock_data_url)) {
    ctx->blocked_by = kAdBlocked;
  } else if (!did_match_exception && canonical_name.has_value() &&
             ctx->request_url.host() != *canonical_name &&
             *canonical_name != "") {
    GURL::Replacements replacements = GURL::Replacements();
    replacements.SetHost(
        canonical_name->c_str(),
        url::Component(0, static_cast<int>(canonical_name->length())));
    const GURL canonical_url = ctx->request_url.ReplaceComponents(replacements);

    if (!g_brave_browser_process->ad_block_service()->ShouldStartRequest(
            canonical_url, ctx->resource_type, source_host,
            &did_match_exception, &ctx->mock_data_url)) {
      ctx->blocked_by = kAdBlocked;
    }
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

void ShouldBlockAdWithOptionalCname(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx,
    const base::Optional<std::string> cname) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  task_runner->PostTaskAndReply(
      FROM_HERE, base::BindOnce(&ShouldBlockAdOnTaskRunner, ctx, cname),
      base::BindOnce(&OnShouldBlockAdResult, next_callback, ctx));
}

class AdblockCnameResolveHostClient : public network::mojom::ResolveHostClient {
 private:
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};
  base::OnceCallback<void(base::Optional<std::string>)> cb_;
  base::TimeTicks start_time_;

 public:
  AdblockCnameResolveHostClient(
      const ResponseCallback& next_callback,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      std::shared_ptr<BraveRequestInfo> ctx) {
    cb_ = base::BindOnce(&ShouldBlockAdWithOptionalCname, task_runner,
                         std::move(next_callback), ctx);

    auto* web_contents = GetWebContents(
        ctx->render_process_id, ctx->render_frame_id, ctx->frame_tree_node_id);
    if (!web_contents) {
      start_time_ = base::TimeTicks::Now();
      this->OnComplete(net::ERR_FAILED, net::ResolveErrorInfo(), base::nullopt);
      return;
    }

    content::BrowserContext* context = web_contents->GetBrowserContext();

    const auto network_isolation_key = ctx->network_isolation_key;

    network::mojom::ResolveHostParametersPtr optional_parameters =
        network::mojom::ResolveHostParameters::New();
    optional_parameters->include_canonical_name = true;

    network::mojom::NetworkContext* network_context =
        content::BrowserContext::GetDefaultStoragePartition(context)
            ->GetNetworkContext();

    start_time_ = base::TimeTicks::Now();

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
    UMA_HISTOGRAM_TIMES("Brave.ShieldsCNAMEBlocking.TotalResolutionTime",
                        base::TimeTicks::Now() - start_time_);
    if (result == net::OK && resolved_addresses) {
      DCHECK(resolved_addresses.has_value() && !resolved_addresses->empty());
      std::move(cb_).Run(
          base::Optional<std::string>(resolved_addresses->GetCanonicalName()));
    } else {
      std::move(cb_).Run(base::nullopt);
    }

    delete this;
  }

  // Should not be called
  void OnTextResults(const std::vector<std::string>& text_results) override {
    NOTREACHED();
  }

  // Should not be called
  void OnHostnameResults(const std::vector<net::HostPortPair>& hosts) override {
    NOTREACHED();
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

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      g_brave_browser_process->ad_block_service()->GetTaskRunner();

  new AdblockCnameResolveHostClient(std::move(next_callback), task_runner, ctx);
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

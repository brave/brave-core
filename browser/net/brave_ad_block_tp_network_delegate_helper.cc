/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_ad_block_tp_network_delegate_helper.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/timer/elapsed_timer.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/ad_block_pref_service_factory.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_pref_service.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/url_constants.h"
#include "brave/content/public/browser/devtools/adblock_devtools_instumentation.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/dns/public/dns_query_type.h"
#include "services/network/host_resolver.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

namespace brave {

namespace {

const std::string& GetCanonicalName(
    const std::vector<std::string>& dns_aliases) {
  return dns_aliases.size() >= 1 ? dns_aliases.front() : base::EmptyString();
}

// Used to keep track of state between a primary adblock engine query and one
// after CNAME uncloaking the request.
struct EngineFlags {
  bool did_match_rule = false;
  bool did_match_exception = false;
  bool did_match_important = false;
};

struct ShouldBlockRequestParams {
  GURL initiator_url;
  GURL request_url;
  blink::mojom::ResourceType resource_type =
      BraveRequestInfo::kInvalidResourceType;
  bool aggressive_blocking = false;
  std::string method;
  content::GlobalRenderFrameHostToken render_frame_token;
  std::optional<std::string> devtools_request_id;
};

struct ShouldBlockRequestResult {
  EngineFlags engine_flags;
  std::string new_url_spec;
  std::string mock_data_url;
  BlockedBy blocked_by = kNotBlocked;
};

}  // namespace

network::HostResolver* g_testing_host_resolver = nullptr;

void SetAdblockCnameHostResolverForTesting(
    network::HostResolver* host_resolver) {
  g_testing_host_resolver = host_resolver;
}

template <template <typename> class T>
void UseCnameResult(scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const ResponseCallback& next_callback,
                    T<BraveRequestInfo> ctx,
                    EngineFlags previous_result,
                    std::optional<std::string> cname);

template <template <typename> class T>
class AdblockCnameResolveHostClient : public network::mojom::ResolveHostClient {
 private:
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};
  base::OnceCallback<void(std::optional<std::string>)> cb_;
  base::ElapsedTimer elapsed_timer_;

 public:
  AdblockCnameResolveHostClient(
      const ResponseCallback& next_callback,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      T<BraveRequestInfo> ctx,
      EngineFlags previous_result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    cb_ = base::BindOnce(&UseCnameResult<T>, task_runner,
                         std::move(next_callback), ctx, previous_result);

    const auto network_anonymization_key = ctx->network_anonymization_key();

    network::mojom::ResolveHostParametersPtr optional_parameters =
        network::mojom::ResolveHostParameters::New();
    optional_parameters->include_canonical_name = true;
    optional_parameters->dns_query_type = net::DnsQueryType::A;

    SecureDnsConfig secure_dns_config =
        SystemNetworkContextManager::GetStubResolverConfigReader()
            ->GetSecureDnsConfiguration(false);
    // Explicitly specify source when DNS over HTTPS is enabled to avoid
    // using `HostResolverProc` which will be handled by system resolver
    // See https://crbug.com/872665
    if (secure_dns_config.mode() == net::SecureDnsMode::kSecure) {
      optional_parameters->source = net::HostResolverSource::DNS;
    }

    elapsed_timer_ = {};

    if (g_testing_host_resolver) {
      g_testing_host_resolver->ResolveHost(
          network::mojom::HostResolverHost::NewHostPortPair(
              net::HostPortPair::FromURL(ctx->request_url())),
          network_anonymization_key, std::move(optional_parameters),
          receiver_.BindNewPipeAndPassRemote());
    } else {
      auto* web_contents = content::WebContents::FromRenderFrameHost(
          content::RenderFrameHost::FromFrameToken(ctx->render_frame_token()));
      if (!web_contents) {
        elapsed_timer_ = {};
        this->OnComplete(net::ERR_FAILED, net::ResolveErrorInfo(),
                         net::AddressList(),
                         net::HostResolverEndpointResults());
        return;
      }

      network::mojom::NetworkContext* network_context =
          web_contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetNetworkContext();

      network_context->ResolveHost(
          network::mojom::HostResolverHost::NewHostPortPair(
              net::HostPortPair::FromURL(ctx->request_url())),
          network_anonymization_key, std::move(optional_parameters),
          receiver_.BindNewPipeAndPassRemote());
    }

    receiver_.set_disconnect_handler(base::BindOnce(
        &AdblockCnameResolveHostClient::OnComplete, base::Unretained(this),
        net::ERR_NAME_NOT_RESOLVED, net::ResolveErrorInfo(net::ERR_FAILED),
        net::AddressList(), net::HostResolverEndpointResults()));
  }

  void OnComplete(
      int result,
      const net::ResolveErrorInfo& resolve_error_info,
      const net::AddressList& resolved_addresses,
      const net::HostResolverEndpointResults& alternative_endpoints) override {
    UMA_HISTOGRAM_TIMES("Brave.ShieldsCNAMEBlocking.TotalResolutionTime",
                        elapsed_timer_.Elapsed());
    if (result == net::OK) {
      DCHECK(!resolved_addresses.empty());
      std::move(cb_).Run(std::optional<std::string>(
          GetCanonicalName(resolved_addresses.dns_aliases())));
    } else {
      std::move(cb_).Run(std::nullopt);
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

// If `canonical_url` is specified, this will only check if the CNAME-uncloaked
// response should be blocked. Otherwise, it will run the check for the
// original request URL.
ShouldBlockRequestResult ShouldBlockRequestOnTaskRunner(
    brave_shields::AdBlockEngineWrapper* engine_wrapper,
    ShouldBlockRequestParams input,
    EngineFlags previous_result,
    std::optional<GURL> canonical_url) {
  ShouldBlockRequestResult result;
  result.engine_flags = previous_result;

  if (!input.initiator_url.is_valid()) {
    return result;
  }
  const std::string source_host = std::string(input.initiator_url.host());

  GURL url_to_check;
  if (canonical_url.has_value()) {
    url_to_check = *canonical_url;
  } else {
    url_to_check = input.request_url;
  }

  bool force_aggressive = SameDomainOrHost(
      input.initiator_url,
      url::Origin::CreateFromNormalizedTuple("https", "youtube.com", 443),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  SCOPED_UMA_HISTOGRAM_TIMER("Brave.Adblock.ShouldBlockRequest");
  auto adblock_result = engine_wrapper->ShouldStartRequest(
      url_to_check, input.resource_type, source_host,
      input.aggressive_blocking || force_aggressive,
      previous_result.did_match_rule, previous_result.did_match_exception,
      previous_result.did_match_important);

  bool has_valid_rewritten_url = false;
  // Note that `rewritten_url` results should only be used for the
  // "user-facing" URL; never for the canonical one.
  if (!canonical_url && adblock_result.rewritten_url.has_value &&
      GURL(std::string(adblock_result.rewritten_url.value)).is_valid() &&
      (input.method == "GET" || input.method == "HEAD" ||
       input.method == "OPTIONS")) {
    result.new_url_spec = std::string(adblock_result.rewritten_url.value);
    has_valid_rewritten_url = true;
  }

  result.mock_data_url = std::string(adblock_result.redirect.value);

  previous_result.did_match_rule |= adblock_result.matched;
  previous_result.did_match_important |= adblock_result.important;
  previous_result.did_match_exception |= adblock_result.has_exception;
  result.engine_flags = previous_result;

  if (previous_result.did_match_important ||
      (previous_result.did_match_rule &&
       !previous_result.did_match_exception)) {
    result.blocked_by = kAdBlocked;
  }

  const bool should_report_to_devtools =
      input.devtools_request_id &&
      (result.blocked_by == kAdBlocked || previous_result.did_match_exception);

  if (should_report_to_devtools) {
    content::devtools_instrumentation::AdblockInfo info;
    info.request_url = input.request_url;
    info.checked_url = url_to_check;
    info.source_host = source_host;
    info.resource_type = input.resource_type;
    info.aggressive = input.aggressive_blocking || force_aggressive;
    info.blocked = result.blocked_by == kAdBlocked;
    info.did_match_important_rule = previous_result.did_match_important;
    info.did_match_rule = previous_result.did_match_rule;
    info.did_match_exception = previous_result.did_match_exception;
    info.has_mock_data = !result.mock_data_url.empty();
    if (has_valid_rewritten_url) {
      info.rewritten_url = result.new_url_spec;
    }

    content::devtools_instrumentation::SendAdblockInfo(
        input.render_frame_token, input.devtools_request_id.value(), info);
  }

  return result;
}

template <template <typename> class T>
void OnShouldBlockRequestResult(
    bool then_check_uncloaked,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> ctx,
    ShouldBlockRequestResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!ctx) {
    next_callback.Run();
    return;
  }
  ctx->set_blocked_by(result.blocked_by);
  ctx->set_mock_data_url(std::move(result.mock_data_url));
  if (!result.new_url_spec.empty()) {
    ctx->set_new_url_spec(std::move(result.new_url_spec));
  }

  if (ctx->blocked_by() == kAdBlocked) {
    brave_shields::BraveShieldsWebContentsObserver::DispatchBlockedEvent(
        ctx->request_url(), ctx->render_frame_token(), brave_shields::kAds);
  } else if (then_check_uncloaked) {
    // This will be deleted by `AdblockCnameResolveHostClient::OnComplete`.
    new AdblockCnameResolveHostClient<T>(std::move(next_callback), task_runner,
                                         ctx, result.engine_flags);
    return;
  }
  next_callback.Run();
}

template <template <typename> class T>
void UseCnameResult(scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const ResponseCallback& next_callback,
                    T<BraveRequestInfo> ctx,
                    EngineFlags previous_result,
                    std::optional<std::string> cname) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!ctx) {
    next_callback.Run();
    return;
  }

  if (cname.has_value() && ctx->request_url().host() != *cname &&
      !cname->empty()) {
    GURL::Replacements replacements;
    replacements.SetHostStr(cname->c_str());
    const GURL canonical_url =
        ctx->request_url().ReplaceComponents(replacements);

    ShouldBlockRequestParams input{ctx->initiator_url(),
                                   ctx->request_url(),
                                   ctx->resource_type(),
                                   ctx->aggressive_blocking(),
                                   ctx->method(),
                                   ctx->render_frame_token(),
                                   ctx->devtools_request_id()};
    task_runner->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(
            &ShouldBlockRequestOnTaskRunner,
            base::Unretained(
                g_brave_browser_process->ad_block_service()->engine_wrapper()),
            std::move(input), previous_result,
            std::make_optional<GURL>(canonical_url)),
        base::BindOnce(&OnShouldBlockRequestResult<T>, false, task_runner,
                       next_callback, ctx));
  } else {
    next_callback.Run();
  }
}

// If only particular types of network traffic are being proxied, or if no
// proxy is configured, it should be safe to continue making unproxied DNS
// queries. However, in SingleProxy mode all types of network traffic should go
// through the proxy, so additional DNS queries should be avoided. Also, in the
// case of per-scheme proxy configurations, a fallback for any non-matching
// request can be configured, in which case additional DNS queries should be
// avoided as well.
//
// Ideally, this should use |ConfiguredProxyResolutionService| to get an
// accurate answer on a per-URL basis. This may require extending the
// |NetworkContext| mojom interface.
bool ProxySettingsAllowUncloaking(content::BrowserContext* browser_context) {
  DCHECK(browser_context);

  bool can_uncloak = true;

  auto* ad_block_pref_service =
      brave_shields::AdBlockPrefServiceFactory::GetForBrowserContext(
          browser_context);

  net::ProxyConfigWithAnnotation config;
  net::ProxyConfigService::ConfigAvailability availability =
      ad_block_pref_service->GetLatestProxyConfig(&config);

  if (availability ==
      net::ProxyConfigService::ConfigAvailability::CONFIG_VALID) {
    // PROXY_LIST corresponds to SingleProxy mode.
    if (config.value().proxy_rules().type ==
            net::ProxyConfig::ProxyRules::Type::PROXY_LIST ||
        (config.value().proxy_rules().type ==
             net::ProxyConfig::ProxyRules::Type::PROXY_LIST_PER_SCHEME &&
         !config.value().proxy_rules().fallback_proxies.IsEmpty())) {
      can_uncloak = false;
    }

    if (config.value().proxy_rules().type ==
            net::ProxyConfig::ProxyRules::Type::PROXY_LIST_PER_SCHEME &&
        !config.value().proxy_rules().proxies_for_https.IsEmpty()) {
      can_uncloak = false;
    }

    if (config.value().has_pac_url()) {
      can_uncloak = false;
    }
  }

  return can_uncloak;
}

template <template <typename> class T>
void OnBeforeURLRequestAdBlockTP(const ResponseCallback& next_callback,
                                 T<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(ctx->request_identifier(), 0UL);
  DCHECK(!ctx->request_url().is_empty());
  DCHECK(!ctx->initiator_url().is_empty());

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      g_brave_browser_process->ad_block_service()->GetTaskRunner();

  // DNS queries won't be routed through Tor or proxies, so we need to
  // skip requests in those contexts.
  // This could be reconsidered if there is a reliable way to route DoH traffic
  // through the same channel.
  bool should_check_uncloaked =
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCnameUncloaking) &&
      ctx->browser_context() && !ctx->browser_context()->IsTor() &&
      ProxySettingsAllowUncloaking(ctx->browser_context());

  // When default 1p blocking is disabled, first-party requests should not be
  // CNAME uncloaked unless using aggressive blocking mode.
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) &&
      should_check_uncloaked && !ctx->aggressive_blocking() &&
      SameDomainOrHost(
          ctx->request_url(),
          url::Origin::CreateFromNormalizedTuple(
              "https", std::string(ctx->initiator_url().host()), 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    should_check_uncloaked = false;
  }

  ShouldBlockRequestParams input{
      ctx->initiator_url(),       ctx->request_url(), ctx->resource_type(),
      ctx->aggressive_blocking(), ctx->method(),      ctx->render_frame_token(),
      ctx->devtools_request_id()};
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          &ShouldBlockRequestOnTaskRunner,
          base::Unretained(
              g_brave_browser_process->ad_block_service()->engine_wrapper()),
          std::move(input), EngineFlags(), std::nullopt),
      base::BindOnce(&OnShouldBlockRequestResult<T>, should_check_uncloaked,
                     task_runner, next_callback, ctx));
}

template <template <typename> class T>
int OnBeforeURLRequest_AdBlockTPPreWork(const ResponseCallback& next_callback,
                                        T<BraveRequestInfo> ctx) {
  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->request_url().is_empty() || ctx->initiator_url().is_empty() ||
      !ctx->initiator_url().has_host() || !ctx->allow_brave_shields() ||
      ctx->allow_ads() ||
      ctx->resource_type() == BraveRequestInfo::kInvalidResourceType) {
    return net::OK;
  }

  // Filter out unnecessary request schemes, to avoid passing large `data:`
  // URLs to the blocking engine.
  if (!ctx->request_url().SchemeIsHTTPOrHTTPS() &&
      !ctx->request_url().SchemeIsWSOrWSS()) {
    return net::OK;
  }

  // Also, until a better solution is available, we explicitly allow any
  // request from an extension.
  if (ctx->initiator_url().SchemeIs(kChromeExtensionScheme) &&
      !base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveExtensionNetworkBlocking)) {
    return net::OK;
  }

  // Requests for main frames are handled by DomainBlockNavigationThrottle,
  // which can display a custom interstitial with an option to proceed if a
  // block is made. We don't need to check these twice.
  // However, we still need to check for websocket schemes (see
  // https://github.com/brave/brave-browser/issues/26302).
  if (ctx->resource_type() == blink::mojom::ResourceType::kMainFrame &&
      !ctx->request_url().SchemeIsWSOrWSS()) {
    return net::OK;
  }

  OnBeforeURLRequestAdBlockTP(next_callback, ctx);

  return net::ERR_IO_PENDING;
}

template int OnBeforeURLRequest_AdBlockTPPreWork<std::shared_ptr>(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

template int OnBeforeURLRequest_AdBlockTPPreWork<base::WeakPtr>(
    const ResponseCallback& next_callback,
    base::WeakPtr<BraveRequestInfo> ctx);

}  // namespace brave

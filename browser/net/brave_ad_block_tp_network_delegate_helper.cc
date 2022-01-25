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
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/net/url_context.h"
#include "brave/common/network_constants.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/browser/net/secure_dns_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/url_pattern.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "services/network/host_resolver.h"
#include "services/network/network_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/url_canon.h"

namespace brave {

network::HostResolver* g_testing_host_resolver;

void SetAdblockCnameHostResolverForTesting(
    network::HostResolver* host_resolver) {
  g_testing_host_resolver = host_resolver;
}

// These strings are duplicated in subresource_redirect_util.cc and
// private_cdn_util.cc.
std::array<std::string, 4> pcdn_domains{
    "pcdn.brave.com", "pcdn.bravesoftware.com", "pcdn.brave.software"};

void SetRedirectUrlAllowedDomainForTesting(std::string domain) {
  pcdn_domains[0] = domain;
}

// Used to keep track of state between a primary adblock engine query and one
// after CNAME uncloaking the request.
struct EngineFlags {
  bool did_match_rule = false;
  bool did_match_exception = false;
  bool did_match_important = false;
};

void UseCnameResult(scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const ResponseCallback& next_callback,
                    std::shared_ptr<BraveRequestInfo> ctx,
                    EngineFlags previous_result,
                    absl::optional<std::string> cname);

class AdblockCnameResolveHostClient : public network::mojom::ResolveHostClient {
 private:
  mojo::Receiver<network::mojom::ResolveHostClient> receiver_{this};
  base::OnceCallback<void(absl::optional<std::string>)> cb_;
  base::TimeTicks start_time_;

 public:
  AdblockCnameResolveHostClient(
      const ResponseCallback& next_callback,
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      std::shared_ptr<BraveRequestInfo> ctx,
      EngineFlags previous_result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    cb_ = base::BindOnce(&UseCnameResult, task_runner, std::move(next_callback),
                         ctx, previous_result);

    const auto network_isolation_key = ctx->network_isolation_key;

    network::mojom::ResolveHostParametersPtr optional_parameters =
        network::mojom::ResolveHostParameters::New();
    optional_parameters->include_canonical_name = true;

    SecureDnsConfig secure_dns_config =
        SystemNetworkContextManager::GetStubResolverConfigReader()
            ->GetSecureDnsConfiguration(false);
    // Explicitly specify source when DNS over HTTPS is enabled to avoid
    // using `HostResolverProc` which will be handled by system resolver
    // See https://crbug.com/872665
    if (secure_dns_config.mode() == net::SecureDnsMode::kSecure)
      optional_parameters->source = net::HostResolverSource::DNS;

    start_time_ = base::TimeTicks::Now();

    if (g_testing_host_resolver) {
      g_testing_host_resolver->ResolveHost(
          net::HostPortPair::FromURL(ctx->request_url), network_isolation_key,
          std::move(optional_parameters), receiver_.BindNewPipeAndPassRemote());
    } else {
      auto* web_contents =
          content::WebContents::FromFrameTreeNodeId(ctx->frame_tree_node_id);
      if (!web_contents) {
        start_time_ = base::TimeTicks::Now();
        this->OnComplete(net::ERR_FAILED, net::ResolveErrorInfo(),
                         absl::nullopt);
        return;
      }

      network::mojom::NetworkContext* network_context =
          web_contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetNetworkContext();

      network_context->ResolveHost(
          net::HostPortPair::FromURL(ctx->request_url), network_isolation_key,
          std::move(optional_parameters), receiver_.BindNewPipeAndPassRemote());
    }

    receiver_.set_disconnect_handler(
        base::BindOnce(&AdblockCnameResolveHostClient::OnComplete,
                       base::Unretained(this), net::ERR_NAME_NOT_RESOLVED,
                       net::ResolveErrorInfo(net::ERR_FAILED), absl::nullopt));
  }

  void OnComplete(
      int32_t result,
      const net::ResolveErrorInfo& resolve_error_info,
      const absl::optional<net::AddressList>& resolved_addresses) override {
    UMA_HISTOGRAM_TIMES("Brave.ShieldsCNAMEBlocking.TotalResolutionTime",
                        base::TimeTicks::Now() - start_time_);
    if (result == net::OK && resolved_addresses) {
      DCHECK(resolved_addresses.has_value() && !resolved_addresses->empty());
      std::move(cb_).Run(
          absl::optional<std::string>(resolved_addresses->GetCanonicalName()));
    } else {
      std::move(cb_).Run(absl::nullopt);
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

bool IsValidPrivateCDNDomain(GURL url) {
  for (const base::StringPiece domain : pcdn_domains) {
    if (url.DomainIs(domain))
      return true;
  }
  return false;
}

// If `canonical_url` is specified, this will only check if the CNAME-uncloaked
// response should be blocked. Otherwise, it will run the check for the
// original request URL.
EngineFlags ShouldBlockRequestOnTaskRunner(
    std::shared_ptr<BraveRequestInfo> ctx,
    EngineFlags previous_result,
    absl::optional<GURL> canonical_url) {
  if (!ctx->initiator_url.is_valid()) {
    return previous_result;
  }
  const std::string source_host = ctx->initiator_url.host();

  GURL url_to_check;
  if (canonical_url.has_value()) {
    url_to_check = *canonical_url;
  } else {
    url_to_check = ctx->request_url;
  }

  bool force_aggressive = SameDomainOrHost(
      ctx->initiator_url,
      url::Origin::CreateFromNormalizedTuple("https", "youtube.com", 80),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  SCOPED_UMA_HISTOGRAM_TIMER("Brave.Adblock.ShouldBlockRequest");
  g_brave_browser_process->ad_block_service()->ShouldStartRequest(
      url_to_check, ctx->resource_type, source_host,
      ctx->aggressive_blocking || force_aggressive,
      &previous_result.did_match_rule, &previous_result.did_match_exception,
      &previous_result.did_match_important, &ctx->adblock_replacement_url);

  if (previous_result.did_match_important ||
      (previous_result.did_match_rule &&
       !previous_result.did_match_exception)) {
    ctx->blocked_by = kAdBlocked;
  }

  // Check what type of adblock redirect to do, if any
  const GURL adblock_replacement_gurl(ctx->adblock_replacement_url);
  if (ctx->blocked_by == kAdBlocked && adblock_replacement_gurl.is_valid()) {
    // Check if it's a valid redirect URL match
    const auto should_redirect_url =
        base::FeatureList::IsEnabled(net::features::kAdblockRedirectUrl) &&
        adblock_replacement_gurl.SchemeIs(url::kHttpsScheme) &&
        IsValidPrivateCDNDomain(adblock_replacement_gurl);
    if (should_redirect_url) {
      ctx->new_url_spec = ctx->adblock_replacement_url;
      ctx->adblock_redirect_type = AdblockRedirectType::kRemote;  // UNUSED
    } else if (adblock_replacement_gurl.SchemeIs(url::kDataScheme)) {
      ctx->adblock_redirect_type = AdblockRedirectType::kLocal;
    } else {
      // To avoid breakage, if we can't do the adblock redirect, don't block
      // the resource. Note that this is only reached if there was a
      // Redirection::Resource or Redirection::Url set in lib.rs
      ctx->blocked_by = kNotBlocked;
    }
  }

  return previous_result;
}

void OnShouldBlockRequestResult(
    bool then_check_uncloaked,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx,
    EngineFlags result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (ctx->blocked_by == kAdBlocked) {
    brave_shields::BraveShieldsWebContentsObserver::DispatchBlockedEvent(
        ctx->request_url, ctx->frame_tree_node_id, brave_shields::kAds);
  } else if (then_check_uncloaked) {
    // This will be deleted by `AdblockCnameResolveHostClient::OnComplete`.
    new AdblockCnameResolveHostClient(std::move(next_callback), task_runner,
                                      ctx, result);
    return;
  }
  next_callback.Run();
}

void UseCnameResult(scoped_refptr<base::SequencedTaskRunner> task_runner,
                    const ResponseCallback& next_callback,
                    std::shared_ptr<BraveRequestInfo> ctx,
                    EngineFlags previous_result,
                    absl::optional<std::string> cname) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (cname.has_value() && ctx->request_url.host() != *cname &&
      !cname->empty()) {
    url::Replacements<char> replacements;
    replacements.SetHost(cname->c_str(),
                         url::Component(0, static_cast<int>(cname->length())));
    const GURL canonical_url = ctx->request_url.ReplaceComponents(replacements);

    task_runner->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&ShouldBlockRequestOnTaskRunner, ctx, previous_result,
                       absl::make_optional<GURL>(canonical_url)),
        base::BindOnce(&OnShouldBlockRequestResult, false, task_runner,
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
// For some reason, when DoH is enabled alongside a system HTTPS proxy, the
// CNAME queries here are also not proxied. So uncloaking is disabled in that
// case as well.
bool ProxySettingsAllowUncloaking(content::BrowserContext* browser_context,
                                  bool doh_enabled) {
  DCHECK(browser_context);

  bool can_uncloak = true;

  Profile* profile = Profile::FromBrowserContext(browser_context);

  std::unique_ptr<PrefProxyConfigTracker> config_tracker =
      ProxyServiceFactory::CreatePrefProxyConfigTrackerOfProfile(
          profile->GetPrefs(), nullptr);
  std::unique_ptr<net::ProxyConfigService> proxy_config_service =
      ProxyServiceFactory::CreateProxyConfigService(config_tracker.get(),
                                                    profile);

  net::ProxyConfigWithAnnotation config;
  net::ProxyConfigService::ConfigAvailability availability =
      proxy_config_service->GetLatestProxyConfig(&config);

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
  }

  config_tracker->DetachFromPrefService();

  return can_uncloak;
}

void OnBeforeURLRequestAdBlockTP(const ResponseCallback& next_callback,
                                 std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(ctx->request_identifier, 0UL);
  DCHECK(!ctx->request_url.is_empty());
  DCHECK(!ctx->initiator_url.is_empty());

  scoped_refptr<base::SequencedTaskRunner> task_runner =
      g_brave_browser_process->ad_block_service()->GetTaskRunner();

  SecureDnsConfig secure_dns_config =
      SystemNetworkContextManager::GetStubResolverConfigReader()
          ->GetSecureDnsConfiguration(false);

  bool doh_enabled = (secure_dns_config.mode() == net::SecureDnsMode::kSecure);

  // DoH or standard DNS queries won't be routed through Tor, so we need to
  // skip it.
  // Also, skip CNAME uncloaking if there is currently a configured proxy.
  bool should_check_uncloaked =
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCnameUncloaking) &&
      ctx->browser_context && !ctx->browser_context->IsTor() &&
      ProxySettingsAllowUncloaking(ctx->browser_context, doh_enabled);

  // When default 1p blocking is disabled, first-party requests should not be
  // CNAME uncloaked unless using aggressive blocking mode.
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockDefault1pBlocking) &&
      should_check_uncloaked && !ctx->aggressive_blocking &&
      SameDomainOrHost(
          ctx->request_url,
          url::Origin::CreateFromNormalizedTuple("https",
                                                 ctx->initiator_url.host(), 80),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    should_check_uncloaked = false;
  }

  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ShouldBlockRequestOnTaskRunner, ctx, EngineFlags(),
                     absl::nullopt),
      base::BindOnce(&OnShouldBlockRequestResult, should_check_uncloaked,
                     task_runner, next_callback, ctx));
}

int OnBeforeURLRequest_AdBlockTPPreWork(const ResponseCallback& next_callback,
                                        std::shared_ptr<BraveRequestInfo> ctx) {
  // If the following info isn't available, then proper content settings can't
  // be looked up, so do nothing.
  if (ctx->request_url.is_empty() ||
      ctx->initiator_url.is_empty() || !ctx->initiator_url.has_host() ||
      !ctx->allow_brave_shields || ctx->allow_ads ||
      ctx->resource_type == BraveRequestInfo::kInvalidResourceType) {
    return net::OK;
  }

  // Filter out unnecessary request schemes, to avoid passing large `data:`
  // URLs to the blocking engine.
  if (!ctx->request_url.SchemeIsHTTPOrHTTPS() &&
      !ctx->request_url.SchemeIsWSOrWSS()) {
    return net::OK;
  }

  // Also, until a better solution is available, we explicitly allow any
  // request from an extension.
  if (ctx->initiator_url.SchemeIs(kChromeExtensionScheme) &&
      !base::FeatureList::IsEnabled(
          ::brave_shields::features::kBraveExtensionNetworkBlocking)) {
    return net::OK;
  }

  // Requests for main frames are handled by DomainBlockNavigationThrottle,
  // which can display a custom interstitial with an option to proceed if a
  // block is made. We don't need to check these twice.
  if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame) {
    return net::OK;
  }

  OnBeforeURLRequestAdBlockTP(next_callback, ctx);

  return net::ERR_IO_PENDING;
}

}  // namespace brave

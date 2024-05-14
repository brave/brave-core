/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/supports_user_data.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "build/buildflag.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/url_constants.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"
#include "url/origin.h"


namespace {

// We have to check both domain and _dnslink.domain
// https://dnslink.io/#can-i-use-dnslink-in-non-dns-systems
const char kDnsDomainPrefix[] = "_dnslink.";

// IPFS HTTP gateways can return an x-ipfs-path header with each response.
// The value of the header is the IPFS path of the returned payload.
const char kIfpsPathHeader[] = "x-ipfs-path";

#if !BUILDFLAG(IS_ANDROID)
const char kIpfsFallbackOriginalUrlKey[] = "ipfs-fallback-original-url";
const char kIpfsFallbackBlockRedirectForNavEntryIdKey[] =
    "ipfs-fallback-block-redirect-for-nav-entry-id";

struct IpfsFallbackOriginalUrlData : public base::SupportsUserData::Data {
  explicit IpfsFallbackOriginalUrlData(const GURL& url) : original_url(url) {}
  ~IpfsFallbackOriginalUrlData() override = default;

  GURL original_url;
};
struct IpfsFallbackBlockRedirectData : public base::SupportsUserData::Data {
  explicit IpfsFallbackBlockRedirectData(const bool& enable_block)
      : enable_redirect_block(enable_block) {}
  ~IpfsFallbackBlockRedirectData() override = default;

  bool enable_redirect_block;
};

#endif  // !BUILDFLAG(IS_ANDROID)

// Sets current executable as default protocol handler in a system.
void SetupIPFSProtocolHandler(const std::string& protocol) {
  auto isDefaultCallback = [](const std::string& protocol,
                              shell_integration::DefaultWebClientState state) {
    if (state == shell_integration::IS_DEFAULT) {
      VLOG(1) << protocol << " already has a handler";
      return;
    }
    VLOG(1) << "Set as default handler for " << protocol;
    // The worker pointer is reference counted. While it is running, the
    // sequence it runs on will hold references it will be automatically
    // freed once all its tasks have finished.
    base::MakeRefCounted<shell_integration::DefaultSchemeClientWorker>(protocol)
        ->StartSetAsDefault(base::NullCallback());
  };

  base::MakeRefCounted<shell_integration::DefaultSchemeClientWorker>(protocol)
      ->StartCheckIsDefault(base::BindOnce(isDefaultCallback, protocol));
}

}  // namespace

namespace ipfs {


IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      IpfsImportController(*web_contents),
      content::WebContentsUserData<IPFSTabHelper>(*web_contents),
      pref_service_(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()))
{
  resolver_ = std::make_unique<IPFSHostResolver>(
      web_contents->GetBrowserContext(), kDnsDomainPrefix);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      kIPFSResolveMethod,
      base::BindRepeating(&IPFSTabHelper::UpdateDnsLinkButtonState,
                          base::Unretained(this)));
}

// static
bool IPFSTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  if (!ipfs::IpfsServiceFactory::GetForContext(
          web_contents->GetBrowserContext())) {
    return false;
  }

  CreateForWebContents(web_contents);
  return true;
}

void IPFSTabHelper::IPFSResourceLinkResolved(const GURL& ipfs) {
  ipfs_resolved_url_ = ipfs.is_valid() ? ipfs : GURL();
  UpdateLocationBar();
}

#if !BUILDFLAG(IS_ANDROID)
bool IPFSTabHelper::IsResolveMethod(
    const ipfs::IPFSResolveMethodTypes& resolution_method) {
  return pref_service_->GetInteger(kIPFSResolveMethod) ==
         static_cast<int>(resolution_method);
}
#endif  // !BUILDFLAG(IS_ANDROID)

void IPFSTabHelper::DNSLinkResolved(const GURL& ipfs,
                                    bool is_gateway_url,
                                    const bool& auto_redirect_blocked) {
  DCHECK(!ipfs.is_valid() || ipfs.SchemeIs(kIPNSScheme));
}

void IPFSTabHelper::HostResolvedCallback(
    const GURL& current,
    const GURL& target_url,
    bool is_gateway_url,
    std::optional<std::string> x_ipfs_path_header,
    const bool auto_redirect_blocked,
    const std::string& host,
    const std::optional<std::string>& dnslink) {
  // Check if user hasn't redirected to another host while dnslink was resolving
  if (current.host() != web_contents()->GetVisibleURL().host() ||
      !current.SchemeIsHTTPOrHTTPS())
    return;
  if (!dnslink || dnslink.value().empty()) {
    if (x_ipfs_path_header) {
      IPFSResourceLinkResolved(ResolveXIPFSPathUrl(x_ipfs_path_header.value()));
    }
    return;
  }

  DNSLinkResolved(ResolveDNSLinkUrl(target_url), is_gateway_url,
                  auto_redirect_blocked);
}

void IPFSTabHelper::LoadUrl(const GURL& gurl) {
  if (redirect_callback_for_testing_) {
    redirect_callback_for_testing_.Run(gurl);
    return;
  }

  content::OpenURLParams params(gurl, content::Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_LINK, false);
  params.should_replace_current_entry = true;
  web_contents()->OpenURL(params, /*navigation_handle_callback=*/{});
}

#if !BUILDFLAG(IS_ANDROID)
void IPFSTabHelper::LoadUrlForAutoRedirect(const GURL& gurl) {
  if (redirect_callback_for_testing_) {
    redirect_callback_for_testing_.Run(gurl);
    return;
  }

  content::NavigationController::LoadURLParams params(content::OpenURLParams(
      gurl, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_LINK, false));
  params.should_replace_current_entry = true;
  if (auto new_handle =
          web_contents()->GetController().LoadURLWithParams(params)) {
    new_handle->SetUserData(
        kIpfsFallbackOriginalUrlKey,
        std::make_unique<IpfsFallbackOriginalUrlData>(GetCurrentPageURL()));
  }
}

void IPFSTabHelper::LoadUrlForFallback(const GURL& gurl) {
  content::NavigationController::LoadURLParams params(content::OpenURLParams(
      gurl, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_TYPED, false));
  params.should_replace_current_entry = true;
  if (auto new_handle =
          web_contents()->GetController().LoadURLWithParams(params);
      new_handle) {
    new_handle->SetUserData(
        kIpfsFallbackBlockRedirectForNavEntryIdKey,
        std::make_unique<IpfsFallbackBlockRedirectData>(true));
  }
}
#endif  // !BUILDFLAG(IS_ANDROID)

void IPFSTabHelper::UpdateLocationBar() {

  if (web_contents()->GetDelegate())
    web_contents()->GetDelegate()->NavigationStateChanged(
        web_contents(), content::INVALIDATE_TYPE_URL);
}

GURL IPFSTabHelper::GetCurrentPageURL() const {
  if (current_page_url_for_testing_.is_valid())
    return current_page_url_for_testing_;
  // We use GetLastCommittedURL as the current url for IPFS related checks
  // because this checks are initiated after navigation commit.
  return web_contents()->GetLastCommittedURL();
}

GURL IPFSTabHelper::GetIPFSResolvedURL() const {
  if (!ipfs_resolved_url_.is_valid())
    return GURL();
  GURL current = GetCurrentPageURL();
  GURL::Replacements replacements;
  replacements.SetQueryStr(current.query_piece());
  replacements.SetRefStr(current.ref_piece());
  return ipfs_resolved_url_.ReplaceComponents(replacements);
}

void IPFSTabHelper::CheckDNSLinkRecord(
    const GURL& target,
    bool is_gateway_url,
    std::optional<std::string> x_ipfs_path_header,
    const bool& auto_redirect_blocked) {
  if (!target.SchemeIsHTTPOrHTTPS())
    return;

  auto host_port_pair = net::HostPortPair::FromURL(target);

  auto resolved_callback = base::BindOnce(
      &IPFSTabHelper::HostResolvedCallback, weak_ptr_factory_.GetWeakPtr(),
      GetCurrentPageURL(), target, is_gateway_url,
      std::move(x_ipfs_path_header), auto_redirect_blocked);

  const auto& network_anonymization_key =
      web_contents()->GetPrimaryMainFrame()
          ? web_contents()
                ->GetPrimaryMainFrame()
                ->GetIsolationInfoForSubresources()
                .network_anonymization_key()
          : net::NetworkAnonymizationKey();
  resolver_->Resolve(host_port_pair, network_anonymization_key,
                     net::DnsQueryType::TXT, std::move(resolved_callback));
}

bool IPFSTabHelper::IsDNSLinkCheckEnabled() const {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));

  return (resolve_method != ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
}

bool IPFSTabHelper::IsAutoRedirectIPFSResourcesEnabled() const {
  // auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
  //     pref_service_->GetInteger(kIPFSResolveMethod));
  // auto autoredirect_ipfs_resources_enabled =
  //     pref_service_->GetBoolean(kIPFSAutoRedirectToConfiguredGateway);

  // return (resolve_method != ipfs::IPFSResolveMethodTypes::IPFS_DISABLED) &&
  //        autoredirect_ipfs_resources_enabled;
  return false;
}

void IPFSTabHelper::UpdateDnsLinkButtonState() {
  if (!IsDNSLinkCheckEnabled()) {
    if (ipfs_resolved_url_.is_valid()) {
      ipfs_resolved_url_ = GURL();
      UpdateLocationBar();
    }
    return;
  }
  GURL current = GetCurrentPageURL();
  if (!ipfs_resolved_url_.is_valid() || (resolver_->host() != current.host()) ||
      !CanResolveURL(current)) {
    ipfs_resolved_url_ = GURL();
    UpdateLocationBar();
  }
}

bool IPFSTabHelper::CanResolveURL(const GURL& url) const {
  url::Origin url_origin = url::Origin::Create(url);
  bool resolve = url.SchemeIsHTTPOrHTTPS() &&
                 !IsAPIGateway(url_origin.GetURL(), chrome::GetChannel());
  if (!IsLocalGatewayConfigured(pref_service_)) {
    resolve = resolve && !IsDefaultGatewayURL(url, pref_service_);
  } else {
    resolve = resolve && !IsLocalGatewayURL(url);
  }
  return resolve;
}

// For x-ipfs-path header we are making urls like
// ipfs://<x-ipfs-path>
GURL IPFSTabHelper::ResolveXIPFSPathUrl(
    const std::string& x_ipfs_path_header_value) {
  return TranslateXIPFSPath(x_ipfs_path_header_value).value_or(GURL());
}

std::optional<GURL> IPFSTabHelper::ResolveIPFSUrlFromGatewayLikeUrl(
    const GURL& gurl) {
  bool api_gateway = IsAPIGateway(gurl, chrome::GetChannel());
  auto base_gateway =
      GetConfiguredBaseGateway(pref_service_, chrome::GetChannel());
  if (!api_gateway &&
      // Make sure we don't infinite redirect
      !gurl.DomainIs(base_gateway.host()) && !net::IsLocalhost(gurl)) {
    return ipfs::ExtractSourceFromGateway(gurl);
  }

  return std::nullopt;
}

// For _dnslink we just translate url to ipns:// scheme
GURL IPFSTabHelper::ResolveDNSLinkUrl(const GURL& url) {
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kIPNSScheme);
  return url.ReplaceComponents(replacements);
}

void IPFSTabHelper::MaybeCheckDNSLinkRecord(
    const net::HttpResponseHeaders* headers,
    const bool& auto_redirect_blocked) {
  UpdateDnsLinkButtonState();
  auto current_url = GetCurrentPageURL();
  auto dnslink_target = current_url;
  auto possible_redirect = ResolveIPFSUrlFromGatewayLikeUrl(current_url);
  if (possible_redirect && IsIPFSScheme(possible_redirect.value())) {
    if (IsAutoRedirectIPFSResourcesEnabled() && !auto_redirect_blocked) {
    } else {
      IPFSResourceLinkResolved(possible_redirect.value());
    }
    return;
  } else if (possible_redirect) {
    dnslink_target = possible_redirect.value();
  }

  if (!IsDNSLinkCheckEnabled() || !headers || ipfs_resolved_url_.is_valid() ||
      !CanResolveURL(dnslink_target)) {
    UpdateLocationBar();
    return;
  }
  int response_code = headers->response_code();
  std::string normalized_header;
  if ((response_code >= net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR &&
       response_code <= net::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED)) {
    CheckDNSLinkRecord(dnslink_target, possible_redirect.has_value(),
                       std::nullopt, auto_redirect_blocked);
  } else if (headers->GetNormalizedHeader(kIfpsPathHeader,
                                          &normalized_header)) {
    CheckDNSLinkRecord(dnslink_target, possible_redirect.has_value(),
                       normalized_header, auto_redirect_blocked);
  } else if (possible_redirect) {
    CheckDNSLinkRecord(dnslink_target, possible_redirect.has_value(),
                       std::nullopt, auto_redirect_blocked);
  }
}

void IPFSTabHelper::MaybeSetupIpfsProtocolHandlers(const GURL& url) {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));
  if (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_ASK &&
      IsDefaultGatewayURL(url, pref_service_)) {
    auto infobar_count = pref_service_->GetInteger(kIPFSInfobarCount);
    if (!infobar_count) {
      pref_service_->SetInteger(kIPFSInfobarCount, infobar_count + 1);
      SetupIPFSProtocolHandler(ipfs::kIPFSScheme);
      SetupIPFSProtocolHandler(ipfs::kIPNSScheme);
    }
  }
}

void IPFSTabHelper::DidFinishNavigation(content::NavigationHandle* handle) {
  DCHECK(handle);
  if (!handle->IsInMainFrame() || !handle->HasCommitted() ||
      handle->IsSameDocument()) {
    return;
  }

  if (handle->GetResponseHeaders() &&
      handle->GetResponseHeaders()->HasHeader(kIfpsPathHeader)) {
    MaybeSetupIpfsProtocolHandlers(handle->GetURL());
  }

#if !BUILDFLAG(IS_ANDROID)
  auto* block_redirect_nav_data = static_cast<IpfsFallbackBlockRedirectData*>(
      handle->GetUserData(kIpfsFallbackBlockRedirectForNavEntryIdKey));
  const bool auto_redirect_blocked =
      block_redirect_nav_data && block_redirect_nav_data->enable_redirect_block;

  MaybeCheckDNSLinkRecord(handle->GetResponseHeaders(), auto_redirect_blocked);
#else
  MaybeCheckDNSLinkRecord(handle->GetResponseHeaders(), false);
#endif  // !BUILDFLAG(IS_ANDROID)
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper);

}  // namespace ipfs

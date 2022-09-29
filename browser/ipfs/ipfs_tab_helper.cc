/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "brave/browser/ipfs/ipfs_host_resolver.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_delegate.h"
#include "net/http/http_status_code.h"
#include "url/origin.h"

namespace {

// We have to check both domain and _dnslink.domain
// https://dnslink.io/#can-i-use-dnslink-in-non-dns-systems
const char kDnsDomainPrefix[] = "_dnslink.";

// IPFS HTTP gateways can return an x-ipfs-path header with each response.
// The value of the header is the IPFS path of the returned payload.
const char kIfpsPathHeader[] = "x-ipfs-path";

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
    base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(
        protocol)
        ->StartSetAsDefault(base::NullCallback());
  };

  base::MakeRefCounted<shell_integration::DefaultProtocolClientWorker>(protocol)
      ->StartCheckIsDefault(base::BindOnce(isDefaultCallback, protocol));
}

}  // namespace

namespace ipfs {

IPFSTabHelper::~IPFSTabHelper() = default;

IPFSTabHelper::IPFSTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      IpfsImportController(web_contents),
      content::WebContentsUserData<IPFSTabHelper>(*web_contents) {
  pref_service_ = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  auto* storage_partition =
      web_contents->GetBrowserContext()->GetDefaultStoragePartition();

  resolver_ = std::make_unique<IPFSHostResolver>(
      storage_partition->GetNetworkContext(), kDnsDomainPrefix);
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

void IPFSTabHelper::XIPFSPathLinkResolved(const GURL& ipfs) {
  ipfs_resolved_url_ = ipfs.is_valid() ? ipfs : GURL();
  UpdateLocationBar();
}

void IPFSTabHelper::DNSLinkResolved(const GURL& ipfs) {
  DCHECK(!ipfs.is_valid() || ipfs.SchemeIs(kIPNSScheme));
  ipfs_resolved_url_ = ipfs.is_valid() ? ipfs : GURL();
  if (ipfs.is_valid() && pref_service_->GetBoolean(kIPFSAutoRedirectDNSLink)) {
    content::OpenURLParams params(GetIPFSResolvedURL(), content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  ui::PAGE_TRANSITION_LINK, false);
    params.should_replace_current_entry = true;
    web_contents()->OpenURL(params);
    return;
  }
  UpdateLocationBar();
}

void IPFSTabHelper::HostResolvedCallback(
    absl::optional<std::string> x_ipfs_path_header,
    const std::string& host,
    const absl::optional<std::string>& dnslink) {
  GURL current = web_contents()->GetURL();

  if (current.host() != host || !current.SchemeIsHTTPOrHTTPS())
    return;
  if (!dnslink || dnslink.value().empty()) {
    if (x_ipfs_path_header) {
      XIPFSPathLinkResolved(ResolveXIPFSPathUrl(x_ipfs_path_header.value()));
    }
    return;
  }
  DNSLinkResolved(ResolveDNSLinkUrl(current));
}

void IPFSTabHelper::UpdateLocationBar() {
  if (web_contents()->GetDelegate())
    web_contents()->GetDelegate()->NavigationStateChanged(
        web_contents(), content::INVALIDATE_TYPE_URL);
}

GURL IPFSTabHelper::GetCurrentPageURL() const {
  if (current_page_url_for_testing_.is_valid())
    return current_page_url_for_testing_;
  return web_contents()->GetVisibleURL();
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
    absl::optional<std::string> x_ipfs_path_header) {
  GURL current = web_contents()->GetURL();
  if (!current.SchemeIsHTTPOrHTTPS())
    return;

  const auto& host_port_pair = net::HostPortPair::FromURL(current);

  auto resolved_callback = base::BindOnce(&IPFSTabHelper::HostResolvedCallback,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          std::move(x_ipfs_path_header));
  const auto& key =
      web_contents()->GetPrimaryMainFrame()
          ? web_contents()->GetPrimaryMainFrame()->GetNetworkIsolationKey()
          : net::NetworkIsolationKey();
  resolver_->Resolve(host_port_pair, key, net::DnsQueryType::TXT,
                     std::move(resolved_callback));
}

bool IPFSTabHelper::IsDNSLinkCheckEnabled() const {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      pref_service_->GetInteger(kIPFSResolveMethod));

  return (resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL ||
          resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
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
// <gateway>/<x-ipfs-path>
GURL IPFSTabHelper::ResolveXIPFSPathUrl(
    const std::string& x_ipfs_path_header_value) {
  GURL gateway =
      ipfs::GetConfiguredBaseGateway(pref_service_, chrome::GetChannel());
  GURL::Replacements replacements;
  replacements.SetPathStr(x_ipfs_path_header_value);
  return gateway.ReplaceComponents(replacements);
}

// For _dnslink we just translate url to ipns:// scheme
GURL IPFSTabHelper::ResolveDNSLinkUrl(const GURL& url) {
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kIPNSScheme);
  return url.ReplaceComponents(replacements);
}

void IPFSTabHelper::MaybeCheckDNSLinkRecord(
    const net::HttpResponseHeaders* headers) {
  UpdateDnsLinkButtonState();
  auto current_url = GetCurrentPageURL();

  if (!IsDNSLinkCheckEnabled() || !headers || ipfs_resolved_url_.is_valid() ||
      !CanResolveURL(current_url)) {
    return;
  }

  int response_code = headers->response_code();
  std::string normalized_header;
  if ((response_code >= net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR &&
       response_code <= net::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED)) {
    CheckDNSLinkRecord(absl::nullopt);
  } else if (headers->GetNormalizedHeader(kIfpsPathHeader,
                                          &normalized_header)) {
    CheckDNSLinkRecord(normalized_header);
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
  MaybeCheckDNSLinkRecord(handle->GetResponseHeaders());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper);

}  // namespace ipfs

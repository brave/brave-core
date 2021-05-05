/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

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

namespace {

// We have to check both domain and _dnslink.domain
// https://dnslink.io/#can-i-use-dnslink-in-non-dns-systems
const char kDnsDomainPrefix[] = "_dnslink.";

// IPFS HTTP gateways can return an x-ipfs-path header with each response.
// The value of the header is the IPFS path of the returned payload.
const char kIfpsPathHeader[] = "x-ipfs-path";

// /ipfs/{cid}/path → ipfs://{cid}/path
// query and fragment are taken from source page url
GURL ParseURLFromHeader(const std::string& value) {
  if (value.empty())
    return GURL();
  std::vector<std::string> parts = base::SplitString(
      value, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  // Default length of header is /[scheme]/cid so we have 3 parts after split.
  const int minimalPartsRequired = 3;
  if (parts.size() < minimalPartsRequired || !parts.front().empty())
    return GURL();
  std::string scheme = parts[1];
  if (scheme != ipfs::kIPFSScheme && scheme != ipfs::kIPNSScheme)
    return GURL();
  std::string cid = parts[2];
  if (scheme.empty() || cid.empty())
    return GURL();
  std::string path;
  // Add all other parts to url path.
  if (parts.size() > minimalPartsRequired) {
    for (size_t i = minimalPartsRequired; i < parts.size(); i++) {
      if (parts[i].empty())
        continue;
      if (!path.empty())
        path += "/";
      path += parts[i];
    }
  }
  std::string spec = scheme + "://" + cid;
  if (!path.empty())
    spec += "/" + path;
  return GURL(spec);
}

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
      IpfsImportController(web_contents) {
  pref_service_ = user_prefs::UserPrefs::Get(web_contents->GetBrowserContext());
  auto* storage_partition =
      web_contents->GetBrowserContext()->GetDefaultStoragePartition();

  resolver_.reset(new IPFSHostResolver(storage_partition->GetNetworkContext(),
                                       kDnsDomainPrefix));
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

void IPFSTabHelper::IPFSLinkResolved(const GURL& ipfs) {
  ipfs_resolved_url_ = ipfs;
  if (pref_service_->GetBoolean(kIPFSAutoRedirectDNSLink)) {
    content::OpenURLParams params(GetIPFSResolvedURL(), content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  ui::PAGE_TRANSITION_LINK, false);
    web_contents()->OpenURL(params);
    return;
  }
  UpdateLocationBar();
}

void IPFSTabHelper::HostResolvedCallback(const std::string& host,
                                         const std::string& dnslink) {
  GURL current = web_contents()->GetURL();
  if (current.host() != host || !current.SchemeIsHTTPOrHTTPS())
    return;
  if (dnslink.empty())
    return;
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kIPNSScheme);
  GURL resolved_url(current.ReplaceComponents(replacements));
  if (resolved_url.is_valid())
    IPFSLinkResolved(resolved_url);
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
  std::string cid;
  std::string path;
  ipfs::ParseCIDAndPathFromIPFSUrl(ipfs_resolved_url_, &cid, &path);
  auto resolved_scheme = ipfs_resolved_url_.scheme();
  std::string resolved_path = current.path();
  std::vector<std::string> parts = base::SplitString(
      current.path(), "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  // If public gateway like https://ipfs.io/ipfs/{cid}/..
  // or for IPNS like ipns://branty.eth/path/..
  // skip duplication for /{scheme}/{cid}/ and add the rest parts
  if (parts.size() >= 3 && parts[2] == cid) {
    parts.erase(parts.begin() + 1, parts.begin() + 3);
    resolved_path = base::JoinString(parts, "/");
  }
  std::string current_ipfs_url = resolved_scheme + "://" + cid + resolved_path;
  GURL resolved_url(current_ipfs_url);
  return resolved_url.ReplaceComponents(replacements);
}

void IPFSTabHelper::ResolveIPFSLink() {
  GURL current = web_contents()->GetURL();
  if (!current.SchemeIsHTTPOrHTTPS())
    return;

  const auto& host_port_pair = net::HostPortPair::FromURL(current);

  auto resolved_callback = base::BindOnce(&IPFSTabHelper::HostResolvedCallback,
                                          weak_ptr_factory_.GetWeakPtr());
  const auto& key =
      web_contents()->GetMainFrame()
          ? web_contents()->GetMainFrame()->GetNetworkIsolationKey()
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
  return url.SchemeIsHTTPOrHTTPS() &&
         !IsAPIGateway(url.GetOrigin(), chrome::GetChannel());
}

void IPFSTabHelper::MaybeShowDNSLinkButton(
    const net::HttpResponseHeaders* headers) {
  UpdateDnsLinkButtonState();
  if (!IsDNSLinkCheckEnabled() || !headers || ipfs_resolved_url_.is_valid() ||
      !CanResolveURL(GetCurrentPageURL()))
    return;

  int response_code = headers->response_code();
  if (response_code >= net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR &&
      response_code <= net::HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED) {
    ResolveIPFSLink();
  } else if (headers->HasHeader(kIfpsPathHeader)) {
    std::string ipfs_path_value;
    if (!headers->GetNormalizedHeader(kIfpsPathHeader, &ipfs_path_value))
      return;
    GURL resolved_url = ParseURLFromHeader(ipfs_path_value);
    if (resolved_url.is_valid())
      IPFSLinkResolved(resolved_url);
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
  MaybeShowDNSLinkButton(handle->GetResponseHeaders());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(IPFSTabHelper)

}  // namespace ipfs

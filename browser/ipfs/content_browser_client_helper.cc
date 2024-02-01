/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <string>
#include <utility>

#include "base/strings/string_util.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"

namespace {

constexpr char kIpfsLocalhost[] = ".ipfs.localhost";
constexpr char kIpnsLocalhost[] = ".ipns.localhost";

bool IsIPFSLocalGateway(PrefService* prefs) {
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL;
}

std::size_t GetIPFSCidOrHostEndPos(GURL const* url,
                                   const std::size_t& ipfs_pos,
                                   const std::size_t& ipns_pos) {
  if (ipfs_pos != std::string::npos) {
    return ipfs_pos;
  }

  if (ipns_pos != std::string::npos) {
    return ipns_pos;
  }

  return url->host().length();
}

}  // namespace

namespace ipfs {

bool HandleIPFSURLRewrite(GURL* url, content::BrowserContext* browser_context) {
  if (!brave::IsRegularProfile(browser_context)) {
    return false;
  }
  // This is needed for triggering ReverseRewrite later.
  if (url->SchemeIs("http") &&
      (base::EndsWith(url->host_piece(), kIpfsLocalhost) ||
       base::EndsWith(url->host_piece(), kIpnsLocalhost))) {
    return true;
  }
  LOG(INFO) << "[IPFS] !!!! HandleIPFSURLRewrite url:" << *url;
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
  if (url->SchemeIs(content::kChromeUIScheme) && url->DomainIs(kIPFSScheme)) {
    GURL::Replacements host_replacements;
    host_replacements.SetHostStr(kIPFSWebUIHost);
    *url = url->ReplaceComponents(host_replacements);
    return true;
  }
#endif
  PrefService* prefs = user_prefs::UserPrefs::Get(browser_context);
  if (!IsIpfsResolveMethodDisabled(prefs) &&
      // When it's not the local gateway we don't want to show a ipfs:// URL.
      // We instead will translate the URL later.
      IsIPFSLocalGateway(prefs) &&
      (url->SchemeIs(kIPFSScheme) || url->SchemeIs(kIPNSScheme))) {
    return TranslateIPFSURI(
        *url, url, GetDefaultIPFSLocalGateway(chrome::GetChannel()), false);
  }

  if (url->DomainIs(kLocalhostIP)) {
    GURL::Replacements replacements;
    replacements.SetHostStr(kLocalhostDomain);
    if (IsDefaultGatewayURL(url->ReplaceComponents(replacements), prefs)) {
      *url = url->ReplaceComponents(replacements);
      return true;
    }
  }

  if (IsLocalGatewayConfigured(prefs)) {
    if (decentralized_dns::IsENSTLD(url->host_piece()) &&
        decentralized_dns::IsENSResolveMethodEnabled(
            g_browser_process->local_state())) {
      return true;
    }

    if (decentralized_dns::IsSnsTLD(url->host_piece()) &&
        decentralized_dns::IsSnsResolveMethodEnabled(
            g_browser_process->local_state())) {
      return true;
    }

    if (decentralized_dns::IsUnstoppableDomainsTLD(url->host_piece()) &&
        decentralized_dns::IsUnstoppableDomainsResolveMethodEnabled(
            g_browser_process->local_state())) {
      return true;
    }
  }

  return false;
}

bool HandleIPFSURLReverseRewrite(GURL* url,
                                 content::BrowserContext* browser_context) {
#if BUILDFLAG(ENABLE_IPFS_INTERNALS_WEBUI)
  if (url->SchemeIs(content::kChromeUIScheme) &&
      url->DomainIs(kIPFSWebUIHost)) {
    return true;
  }
#endif

  std::size_t ipfs_pos = url->host_piece().find(kIpfsLocalhost);
  std::size_t ipns_pos = url->host_piece().find(kIpnsLocalhost);

  if (ipfs_pos == std::string::npos && ipns_pos == std::string::npos)
    return false;

  const auto decoded_host = ipfs::DecodeSingleLabelForm(
      url->host().substr(0, GetIPFSCidOrHostEndPos(url, ipfs_pos, ipns_pos)));

  if (!ipfs::IsValidCIDOrDomain(decoded_host)) {
    return false;
  }

  GURL configured_gateway = GetConfiguredBaseGateway(
      user_prefs::UserPrefs::Get(browser_context), chrome::GetChannel());
  if (configured_gateway.port() != url->port())
    return false;
  GURL::Replacements scheme_replacements;
  GURL::Replacements host_replacements;
  if (ipfs_pos != std::string::npos) {
    scheme_replacements.SetSchemeStr(kIPFSScheme);
    host_replacements.SetHostStr(url->host_piece().substr(0, ipfs_pos));
    host_replacements.ClearPort();
  } else {  // ipns
    scheme_replacements.SetSchemeStr(kIPNSScheme);
    host_replacements.SetHostStr(decoded_host);
    host_replacements.ClearPort();
  }

  *url = url->ReplaceComponents(host_replacements);
  *url = url->ReplaceComponents(scheme_replacements);
  return true;
}

}  // namespace ipfs

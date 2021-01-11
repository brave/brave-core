/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <string>
#include <utility>

#include "base/strings/string_util.h"
#include "base/task/post_task.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/url_constants.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_gateway.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/ipfs/translate_ipfs_uri.h"
#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "url/gurl.h"

namespace {

constexpr char kIpfsLocalhost[] = ".ipfs.localhost";
constexpr char kIpnsLocalhost[] = ".ipns.localhost";

bool IsIPFSLocalGateway(content::BrowserContext* browser_context) {
  auto* prefs = user_prefs::UserPrefs::Get(browser_context);
  auto resolve_method = static_cast<ipfs::IPFSResolveMethodTypes>(
      prefs->GetInteger(kIPFSResolveMethod));
  return resolve_method == ipfs::IPFSResolveMethodTypes::IPFS_LOCAL;
}

}  // namespace

namespace ipfs {

bool HandleIPFSURLRewrite(
    GURL* url,
    content::BrowserContext* browser_context) {
  // This is needed for triggering ReverseRewrite later.
  if (url->SchemeIs("http") &&
      (base::EndsWith(url->host_piece(), kIpfsLocalhost) ||
       base::EndsWith(url->host_piece(), kIpnsLocalhost))) {
    return true;
  }

  if (!IsIpfsResolveMethodDisabled(browser_context) &&
      // When it's not the local gateway we don't want to show a ipfs:// URL.
      // We instead will translate the URL later.
      IsIPFSLocalGateway(browser_context) &&
      (url->SchemeIs(kIPFSScheme) || url->SchemeIs(kIPNSScheme))) {
    return TranslateIPFSURI(*url, url,
                            GetDefaultIPFSLocalGateway(chrome::GetChannel()));
  }

  return false;
}

bool HandleIPFSURLReverseRewrite(
    GURL* url,
    content::BrowserContext* browser_context) {

  std::size_t ipfs_pos = url->host_piece().find(kIpfsLocalhost);
  std::size_t ipns_pos = url->host_piece().find(kIpnsLocalhost);

  if (ipfs_pos == std::string::npos && ipns_pos == std::string::npos)
    return false;

  GURL::Replacements scheme_replacements;
  GURL::Replacements host_replacements;
  if (ipfs_pos != std::string::npos) {
    scheme_replacements.SetSchemeStr(kIPFSScheme);
    host_replacements.SetHostStr(url->host_piece().substr(0, ipfs_pos));
    host_replacements.ClearPort();
  } else {  // ipns
    scheme_replacements.SetSchemeStr(kIPNSScheme);
    host_replacements.SetHostStr(url->host_piece().substr(0, ipns_pos));
    host_replacements.ClearPort();
  }

  *url = url->ReplaceComponents(host_replacements);
  *url = url->ReplaceComponents(scheme_replacements);
  return true;
}

}  // namespace ipfs

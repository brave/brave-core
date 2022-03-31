/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kAddrPattern[] = "addr%3D(0x[[:xdigit:]]{40})";

// Given an origin and an account address, append the account address to the
// end of the host piece of the origin, then return it as the new origin.
bool AddAccountToHost(const url::Origin& old_origin,
                      const std::string& account,
                      url::Origin* new_origin) {
  if (old_origin.opaque() || account.empty() || !new_origin)
    return false;

  GURL::Replacements replacements;
  std::string new_host = base::StrCat({old_origin.host(), account});
  replacements.SetHostStr(new_host);

  *new_origin =
      url::Origin::Create(old_origin.GetURL().ReplaceComponents(replacements));

  return !new_origin->host().empty();
}

// Given the overwritten origin, such as https://test.com{addr=123&addr=456},
// extract all addresses and save into address_queue.
void ExtractAddresses(const url::Origin& origin,
                      std::queue<std::string>* address_queue) {
  static const base::NoDestructor<re2::RE2> kAddrRegex(kAddrPattern);
  DCHECK(!origin.opaque() && address_queue);

  std::string origin_string(origin.Serialize());
  re2::StringPiece input(origin_string);
  std::string match;
  while (re2::RE2::FindAndConsume(&input, *kAddrRegex, &match)) {
    address_queue->push(match);
  }
}

// Parse requesting origin in either sub-request format (one address) or
// non-sub-request format (all addresses).
bool ParseRequestingOriginInternal(const url::Origin& origin,
                                   bool sub_req_format,
                                   url::Origin* requesting_origin,
                                   std::string* account,
                                   std::queue<std::string>* address_queue) {
  if (origin.opaque())
    return false;

  std::string scheme_host_group;
  std::string address_group;
  std::string port_group;

  // Validate input format.
  std::string pattern = sub_req_format
                            ? "(.*)(0x[[:xdigit:]]{40})(:[0-9]+)*"
                            : "(.*)%7Baddr%3D0x[[:xdigit:]]{40}(%"
                              "26addr%3D0x[[:xdigit:]]{40})*%7D(:[0-9]+)*";
  RE2 full_pattern(pattern);
  if (!re2::RE2::FullMatch(origin.Serialize(), full_pattern, &scheme_host_group,
                           &address_group, &port_group)) {
    return false;
  }

  if (requesting_origin) {
    auto requesting_origin_string =
        base::StrCat({scheme_host_group, port_group});
    *requesting_origin = url::Origin::Create(GURL(requesting_origin_string));
  }

  if (sub_req_format && account) {
    *account = address_group;
  }

  if (!sub_req_format && address_queue) {
    ExtractAddresses(origin, address_queue);
  }

  return true;
}

}  // namespace

namespace brave_wallet {

bool GetConcatOriginFromWalletAddresses(
    const url::Origin& old_origin,
    const std::vector<std::string>& addresses,
    url::Origin* new_origin) {
  if (old_origin.opaque() || addresses.empty()) {
    return false;
  }

  std::string addresses_suffix = "{";
  for (auto it = addresses.begin(); it != addresses.end(); it++) {
    base::StrAppend(&addresses_suffix, {"addr=", *it});
    if (it != addresses.end() - 1)
      addresses_suffix += "&";
  }
  addresses_suffix += "}";

  return AddAccountToHost(old_origin, addresses_suffix, new_origin);
}

bool ParseRequestingOriginFromSubRequest(const url::Origin& origin,
                                         url::Origin* requesting_origin,
                                         std::string* account) {
  return ParseRequestingOriginInternal(origin, true /* sub_req_format */,
                                       requesting_origin, account,
                                       nullptr /* address_queue */);
}

bool ParseRequestingOrigin(const url::Origin& origin,
                           url::Origin* requesting_origin,
                           std::queue<std::string>* address_queue) {
  if (address_queue && !address_queue->empty())
    return false;
  return ParseRequestingOriginInternal(origin, false /* sub_req_format */,
                                       requesting_origin, nullptr /* account */,
                                       address_queue);
}

bool GetSubRequestOrigin(const url::Origin& old_origin,
                         const std::string& account,
                         url::Origin* new_origin) {
  return AddAccountToHost(old_origin, account, new_origin);
}

GURL GetConnectWithSiteWebUIURL(const GURL& webui_base_url,
                                const std::vector<std::string>& accounts,
                                const url::Origin& origin) {
  DCHECK(webui_base_url.is_valid() && !accounts.empty() && !origin.opaque());

  std::vector<std::string> query_parts;
  for (const auto& account : accounts) {
    query_parts.push_back(base::StringPrintf("addr=%s", account.c_str()));
  }

  mojom::OriginInfoPtr origin_info = MakeOriginInfo(origin);

  query_parts.push_back(base::StringPrintf(
      "origin-scheme=%s", origin_info->origin.scheme().c_str()));
  query_parts.push_back(
      base::StringPrintf("origin-host=%s", origin_info->origin.host().c_str()));
  query_parts.push_back(
      base::StringPrintf("origin-port=%d", origin_info->origin.port()));
  query_parts.push_back(
      base::StringPrintf("origin-spec=%s", origin_info->origin_spec.c_str()));
  query_parts.push_back(base::StringPrintf(
      "etld-plus-one=%s", origin_info->e_tld_plus_one.c_str()));

  std::string query_str = base::JoinString(query_parts, "&");
  url::Replacements<char> replacements;
  replacements.SetQuery(query_str.c_str(), url::Component(0, query_str.size()));
  std::string kConnectWithSite = "connectWithSite";
  replacements.SetRef(kConnectWithSite.c_str(),
                      url::Component(0, kConnectWithSite.size()));
  return webui_base_url.ReplaceComponents(replacements);
}

}  // namespace brave_wallet

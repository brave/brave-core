/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/permission_utils.h"

#include <optional>
#include <string_view>

#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// We keep the ethereum pattern is for backward compatibility because we
// already wrote some content setting using this pattern.
constexpr char kEthAddrPattern[] = "addr=(0x[[:xdigit:]]{40})";
// This is generic pattern for all coins, we put maximum length bump 128 is to
// prevent ReDoS attack.
constexpr char kAddrPattern[] = "addr=([[:alnum:]]{1,128})";

// Given an origin and an account address, append the account address to the
// end of the host piece of the origin, then return it as the new origin.
std::optional<url::Origin> AddAccountToHost(const url::Origin& old_origin,
                                            std::string_view account) {
  if (old_origin.opaque() || account.empty()) {
    return std::nullopt;
  }

  GURL::Replacements replacements;
  std::string new_host = base::StrCat({old_origin.host(), account});
  replacements.SetHostStr(new_host);

  auto new_origin =
      url::Origin::Create(old_origin.GetURL().ReplaceComponents(replacements));

  if (new_origin.host().empty()) {
    return std::nullopt;
  }
  return new_origin;
}

// Given the overwritten origin, such as https://test.com{addr=123&addr=456},
// extract all addresses and save into address_queue.
void ExtractAddresses(permissions::RequestType type,
                      const url::Origin& origin,
                      std::queue<std::string>* address_queue) {
  static base::NoDestructor<re2::RE2> kEthAddrRegex(kEthAddrPattern);
  static base::NoDestructor<re2::RE2> kAddrRegex(kAddrPattern);
  DCHECK(!origin.opaque() && address_queue);

  std::string origin_string(origin.Serialize());
  std::string_view input(origin_string);
  std::string match;
  re2::RE2* regex;
  if (type == permissions::RequestType::kBraveEthereum) {
    regex = kEthAddrRegex.get();
  } else {
    regex = kAddrRegex.get();
  }
  while (re2::RE2::FindAndConsume(&input, *regex, &match)) {
    address_queue->push(match);
  }
}

// Parse requesting origin in either sub-request format (one address) or
// non-sub-request format (all addresses).
bool ParseRequestingOriginInternal(permissions::RequestType type,
                                   const url::Origin& origin,
                                   bool sub_req_format,
                                   url::Origin* requesting_origin,
                                   std::string* account,
                                   std::queue<std::string>* address_queue) {
  if (origin.opaque() || (type != permissions::RequestType::kBraveEthereum &&
                          type != permissions::RequestType::kBraveSolana)) {
    return false;
  }

  std::string scheme_host_group;
  std::string address_group;
  std::string port_group;

  // Validate input format.
  std::string pattern;
  if (type == permissions::RequestType::kBraveEthereum) {
    pattern = sub_req_format ? "(.*)(0x[[:xdigit:]]{40})(:[0-9]+)*"
                             : "(.*){addr=0x[[:xdigit:]]{40}(&"
                               "addr=0x[[:xdigit:]]{40})*}(:[0-9]+)*";
  } else {
    pattern = sub_req_format ? "(.*)__([[:alnum:]]{1,128})(:[0-9]+)*"
                             : "(.*){addr=[[:alnum:]]{1,128}(&"
                               "addr=[[:alnum:]]{1,128})*}(:[0-9]+)*";
  }
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
    ExtractAddresses(type, origin, address_queue);
  }

  return true;
}

}  // namespace

namespace brave_wallet {

std::optional<url::Origin> GetConcatOriginFromWalletAddresses(
    const url::Origin& old_origin,
    const std::vector<std::string>& addresses) {
  if (old_origin.opaque() || addresses.empty()) {
    return std::nullopt;
  }

  std::string addresses_suffix = "{";
  for (auto it = addresses.begin(); it != addresses.end(); it++) {
    base::StrAppend(&addresses_suffix, {"addr=", *it});
    if (it != addresses.end() - 1) {
      addresses_suffix += "&";
    }
  }
  addresses_suffix += "}";

  return AddAccountToHost(old_origin, addresses_suffix);
}

bool ParseRequestingOriginFromSubRequest(permissions::RequestType type,
                                         const url::Origin& origin,
                                         url::Origin* requesting_origin,
                                         std::string* account) {
  return ParseRequestingOriginInternal(type, origin, true /* sub_req_format */,
                                       requesting_origin, account,
                                       nullptr /* address_queue */);
}

bool ParseRequestingOrigin(permissions::RequestType type,
                           const url::Origin& origin,
                           url::Origin* requesting_origin,
                           std::queue<std::string>* address_queue) {
  if (address_queue && !address_queue->empty()) {
    return false;
  }
  return ParseRequestingOriginInternal(type, origin, false /* sub_req_format */,
                                       requesting_origin, nullptr /* account */,
                                       address_queue);
}

std::optional<url::Origin> GetSubRequestOrigin(permissions::RequestType type,
                                               const url::Origin& old_origin,
                                               std::string_view account) {
  if (type != permissions::RequestType::kBraveEthereum &&
      type != permissions::RequestType::kBraveSolana) {
    return std::nullopt;
  }
  std::string account_with_separator;
  if (type == permissions::RequestType::kBraveEthereum) {
    account_with_separator = account;
  } else {
    account_with_separator =
        account.empty() ? account : base::StrCat({"__", account});
  }

  return AddAccountToHost(old_origin, account_with_separator);
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

  query_parts.push_back(
      base::StringPrintf("origin-spec=%s", origin_info->origin_spec.c_str()));
  query_parts.push_back(base::StringPrintf(
      "etld-plus-one=%s", origin_info->e_tld_plus_one.c_str()));

  std::string query_str = base::JoinString(query_parts, "&");
  GURL::Replacements replacements;
  replacements.SetQueryStr(query_str);
  replacements.SetRefStr("connectWithSite");
  return webui_base_url.ReplaceComponents(replacements);
}

std::optional<blink::PermissionType> CoinTypeToPermissionType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return blink::PermissionType::BRAVE_ETHEREUM;
    case mojom::CoinType::SOL:
      return blink::PermissionType::BRAVE_SOLANA;
    default:
      return std::nullopt;
  }
}

std::optional<permissions::RequestType> CoinTypeToPermissionRequestType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return permissions::RequestType::kBraveEthereum;
    case mojom::CoinType::SOL:
      return permissions::RequestType::kBraveSolana;
    default:
      return std::nullopt;
  }
}

}  // namespace brave_wallet

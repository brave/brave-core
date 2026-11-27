/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/permission_utils.h"

#include <optional>
#include <string_view>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/origin.h"

// TODO(https://github.com/brave/brave-browser/issues/47669) this file should be
// in content/browser subfolder of a layered brave_wallet component.

namespace {

// Given an origin and an account address, append the account address to the
// end of the host piece of the origin, then return it as the new origin.
std::optional<url::Origin> AddAccountToHost(const url::Origin& old_origin,
                                            std::string_view separator,
                                            std::string_view account) {
  if (old_origin.opaque() || account.empty()) {
    return std::nullopt;
  }

  GURL::Replacements replacements;
  std::string new_host = base::StrCat({old_origin.host(), separator, account});
  replacements.SetHostStr(new_host);

  auto new_origin =
      url::Origin::Create(old_origin.GetURL().ReplaceComponents(replacements));

  if (new_origin.host().empty()) {
    return std::nullopt;
  }
  return new_origin;
}

// Parse requesting origin in either sub-request format (one address) or
// non-sub-request format (all addresses).
bool ParseRequestingOriginInternal(ContentSettingsType type,
                                   const url::Origin& origin,
                                   url::Origin* requesting_origin,
                                   std::string* account) {
  if (origin.opaque() || (type != ContentSettingsType::BRAVE_ETHEREUM &&
                          type != ContentSettingsType::BRAVE_SOLANA &&
                          type != ContentSettingsType::BRAVE_CARDANO &&
                          type != ContentSettingsType::BRAVE_POLKADOT)) {
    return false;
  }

  std::string host_group;
  std::string address_group;

  // Split host part in origin into original host and address parts.
  std::string pattern;
  switch (type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      pattern = "(.*)(0x[[:xdigit:]]{40})";
      break;
    case ContentSettingsType::BRAVE_CARDANO:
    case ContentSettingsType::BRAVE_POLKADOT:
      // AccountId->unique_key is used as account identifier for cardano and
      // polkadot.
      pattern = "(.*)__([0-9_]+)";
      break;
    case ContentSettingsType::BRAVE_SOLANA:
      pattern = "(.*)__([[:alnum:]]{1,128})";
      break;
    default:
      NOTREACHED();
  }
  RE2 full_pattern(pattern);
  if (!re2::RE2::FullMatch(origin.host(), full_pattern, &host_group,
                           &address_group)) {
    return false;
  }

  if (requesting_origin) {
    *requesting_origin = url::Origin::CreateFromNormalizedTuple(
        origin.scheme(), host_group, origin.port());
  }
  if (account) {
    *account = address_group;
  }
  return true;
}

}  // namespace

namespace brave_wallet {

bool ParseRequestingOriginFromSubRequest(ContentSettingsType type,
                                         const url::Origin& origin,
                                         url::Origin* requesting_origin,
                                         std::string* account) {
  return ParseRequestingOriginInternal(type, origin, requesting_origin,
                                       account);
}

std::optional<url::Origin> GetSubRequestOrigin(ContentSettingsType type,
                                               const url::Origin& old_origin,
                                               std::string_view account) {
  if (type != ContentSettingsType::BRAVE_ETHEREUM &&
      type != ContentSettingsType::BRAVE_SOLANA &&
      type != ContentSettingsType::BRAVE_CARDANO &&
      type != ContentSettingsType::BRAVE_POLKADOT) {
    return std::nullopt;
  }
  if (account.empty()) {
    return std::nullopt;
  }

  std::string_view separator;
  if (type != ContentSettingsType::BRAVE_ETHEREUM) {
    separator = "__";
  }

  return AddAccountToHost(old_origin, separator, account);
}

GURL GetConnectWithSiteWebUIURL(const GURL& webui_base_url,
                                const std::vector<std::string>& accounts,
                                const url::Origin& origin) {
  DCHECK(webui_base_url.is_valid() && !accounts.empty() && !origin.opaque());

  std::vector<std::string> query_parts;
  for (const auto& account : accounts) {
    query_parts.push_back(absl::StrFormat("addr=%s", account));
  }

  mojom::OriginInfoPtr origin_info = MakeOriginInfo(origin);

  query_parts.push_back(
      absl::StrFormat("origin-spec=%s", origin_info->origin_spec));
  query_parts.push_back(
      absl::StrFormat("etld-plus-one=%s", origin_info->e_tld_plus_one));

  std::string query_str = base::JoinString(query_parts, "&");
  GURL::Replacements replacements;
  replacements.SetQueryStr(query_str);
  replacements.SetRefStr("connectWithSite");
  return webui_base_url.ReplaceComponents(replacements);
}

std::optional<ContentSettingsType> CoinTypeToContentSettingsType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return ContentSettingsType::BRAVE_ETHEREUM;
    case mojom::CoinType::SOL:
      return ContentSettingsType::BRAVE_SOLANA;
    case mojom::CoinType::ADA:
      return ContentSettingsType::BRAVE_CARDANO;
    case mojom::CoinType::DOT:
      return ContentSettingsType::BRAVE_POLKADOT;
    default:
      return std::nullopt;
  }
}

}  // namespace brave_wallet

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/permission_utils.h"

#include <optional>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "components/permissions/request_type.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/gurl.h"
#include "url/origin.h"

// TODO(https://github.com/brave/brave-browser/issues/47669) this file should be
// in content/browser subfolder of a layered brave_wallet component.

namespace brave_wallet {

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

std::optional<blink::PermissionType> CoinTypeToPermissionType(
    mojom::CoinType coin_type) {
  switch (coin_type) {
    case mojom::CoinType::ETH:
      return blink::PermissionType::BRAVE_ETHEREUM;
    case mojom::CoinType::SOL:
      return blink::PermissionType::BRAVE_SOLANA;
    case mojom::CoinType::ADA:
      return blink::PermissionType::BRAVE_CARDANO;
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
    case mojom::CoinType::ADA:
      return permissions::RequestType::kBraveCardano;
    default:
      return std::nullopt;
  }
}

}  // namespace brave_wallet

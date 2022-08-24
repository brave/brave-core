/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/wallet_connect/wallet_connect_utils.h"

#include "base/logging.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "url/gurl.h"
#include "url/third_party/mozilla/url_parse.h"

namespace wallet_connect {

namespace {
constexpr char kWalletConnectScheme[] = "wc";
constexpr char kWalletConnectParamsV1Key[] = "key";
constexpr char kWalletConnectParamsV1Bridge[] = "bridge";
constexpr char kWalletConnectParamsV2SymKey[] = "symKey";
constexpr char kWalletConnectParamsV2RelayProtocol[] = "relay-protocol";
constexpr char kWalletConnectParamsV2RelayData[] = "relay-data";
}  // namespace

mojom::WalletConnectURIDataPtr ParseWalletConnectURI(const std::string& uri) {
  mojom::WalletConnectURIData data;
  GURL url(uri);
  if (!url.is_valid() || url.scheme() != kWalletConnectScheme) {
    VLOG(1) << "uri is not valid: " << uri;
    return nullptr;
  }

  std::vector<std::string> paths = base::SplitString(
      url.path(), "@", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  if (paths.size() != 2) {
    VLOG(1) << "not a valid topic@version: " << url.path();
    return nullptr;
  }
  data.topic = paths[0];
  unsigned version = 0;
  if (!base::StringToUint(paths[1], &version)) {
    VLOG(1) << "version is not valid: " << paths[1];
    return nullptr;
  }
  data.version = version;
  if (version != 1 && version != 2) {
    VLOG(1) << "version is not supported: " << version;
    return nullptr;
  }

  std::string query_str = url.query();
  url::Component query(0, query_str.length());
  url::Component key, value;
  mojom::WalletConnectURIParametersV1 v1_params;
  mojom::WalletConnectURIParametersV2 v2_params;
  while (url::ExtractQueryKeyValue(query_str.c_str(), &query, &key, &value)) {
    std::string key_string = query_str.substr(key.begin, key.len);
    std::string unescaped = base::UnescapeURLComponent(
        query_str.substr(value.begin, value.len),
        base::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
            base::UnescapeRule::PATH_SEPARATORS);
    if (version == 1) {
      if (key_string == kWalletConnectParamsV1Key) {
        v1_params.key = unescaped;
      } else if (key_string == kWalletConnectParamsV1Bridge) {
        v1_params.bridge = GURL(unescaped);
      }
    } else if (version == 2) {
      if (key_string == kWalletConnectParamsV2SymKey) {
        v2_params.sym_key = unescaped;
      } else if (key_string == kWalletConnectParamsV2RelayProtocol) {
        v2_params.relay_protocol = unescaped;
      } else if (key_string == kWalletConnectParamsV2RelayData) {
        v2_params.relay_data = unescaped;
      }
    }
  }
  if (version == 1) {
    if (v1_params.key.empty() || v1_params.bridge.is_empty()) {
      VLOG(1) << "missing v1 params";
      return nullptr;
    }
    data.params =
        mojom::WalletConnectURIParameters::NewV1Params(v1_params.Clone());
  } else if (version == 2) {
    if (v2_params.sym_key.empty() || v2_params.relay_protocol.empty()) {
      VLOG(1) << "missing v2 params";
      return nullptr;
    }
    data.params =
        mojom::WalletConnectURIParameters::NewV2Params(v2_params.Clone());
  }
  return data.Clone();
}

}  // namespace wallet_connect

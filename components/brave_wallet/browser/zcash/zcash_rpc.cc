// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"

#include "base/big_endian.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace brave_wallet::zcash_rpc {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("zcash_rpc", R"(
      semantics {
        sender: "ZCash RPC"
        description:
          "This service is used to communicate with ZCash Lightwalletd nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "ZCash JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

bool UrlPathEndsWithSlash(const GURL& base_url) {
  auto path_piece = base_url.path_piece();
  return !path_piece.empty() && path_piece.back() == '/';
}

const GURL MakeGetAddressUtxosURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetAddressUtxos"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

// Prefixes provided serialized protobuf with compression byte and 4 bytes of
// message size. See
// https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
std::string GetPrefixedProtobuf(const std::string& serialized_proto) {
  char compression = 0;  // 0 means no compression
  char buff[4];          // big-endian 4 bytes of message size
  base::WriteBigEndian<uint32_t>(buff, serialized_proto.size());
  std::string result;
  result.append(&compression, 1);
  result.append(buff, 4);
  result.append(serialized_proto);
  return result;
}

std::string MakeGetAddressUtxosURLParams(const std::string& address) {
  zcash::GetAddressUtxosRequest request;
  request.add_addresses(address);
  request.set_maxentries(1);
  request.set_startheight(0);
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::unique_ptr<network::SimpleURLLoader> MakeGRPCLoader(
    const GURL& url,
    const std::string& body) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;

  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DISABLE_CACHE;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->AttachStringForUpload(body, "application/grpc+proto");

  url_loader->SetRetryOptions(
      5, network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  url_loader->SetAllowHttpErrorResults(true);
  return url_loader;
}

// Resolves serialized protobuf from length-prefixed string
absl::optional<std::string> ResolveSerializedMessage(
    const std::string& grpc_response_body) {
  if (grpc_response_body.size() < 5) {
    return absl::nullopt;
  }
  if (grpc_response_body[0] != 0) {
    // Compression is not supported yet
    return absl::nullopt;
  }
  uint32_t size = 0;
  base::ReadBigEndian(
      reinterpret_cast<const uint8_t*>(&(grpc_response_body[1])), &size);

  if (grpc_response_body.size() != size + 5) {
    return absl::nullopt;
  }

  return grpc_response_body.substr(5);
}

}  // namespace

ZCashRpc::ZCashRpc(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs), url_loader_factory_(url_loader_factory) {}

ZCashRpc::~ZCashRpc() = default;

void ZCashRpc::GetUtxoList(const std::string& chain_id,
                           const std::string& address,
                           ZCashRpc::GetUtxoListCallback callback) {
  GURL request_url = MakeGetAddressUtxosURL(
      GetNetworkURL(prefs_, chain_id, mojom::CoinType::ZEC));

  if (!request_url.is_valid()) {
    std::move(callback).Run(base::unexpected("Request URL is invalid."));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetAddressUtxosURLParams(address));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnGetUtxosResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      5000);
}

void ZCashRpc::OnGetUtxosResponse(
    ZCashRpc::GetUtxoListCallback callback,
    UrlLoadersList::iterator it,
    const std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(base::unexpected("Network error"));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(base::unexpected("Response body is empty"));
    return;
  }

  auto message = ResolveSerializedMessage(*response_body);
  if (!message) {
    std::move(callback).Run(base::unexpected("Wrong response format"));
    return;
  }

  zcash::GetAddressUtxosResponse response;
  if (!response.ParseFromString(message.value())) {
    std::move(callback).Run(base::unexpected("Can't parse response"));
    return;
  }

  std::vector<zcash::ZCashUtxo> result;
  for (const auto& item : response.addressutxos()) {
    result.push_back(item);
  }

  std::move(callback).Run(result);
}

}  // namespace brave_wallet::zcash_rpc

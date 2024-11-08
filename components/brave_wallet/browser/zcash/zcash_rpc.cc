// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/services/brave_wallet/public/cpp/brave_wallet_utils_service.h"
#include "brave/components/services/brave_wallet/public/cpp/utils/protobuf_utils.h"
#include "brave/components/services/brave_wallet/public/proto/zcash_grpc_data.pb.h"
#include "components/grit/brave_components_strings.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr size_t kMaxBodySize = 5000;

// Checks if provided stream contains any messages.
class IsKnownAddressTxStreamHandler : public GRrpcMessageStreamHandler {
 public:
  using ResultCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;

  IsKnownAddressTxStreamHandler() = default;
  ~IsKnownAddressTxStreamHandler() override = default;

  void set_callback(ResultCallback callback) {
    callback_ = std::move(callback);
  }

  bool is_message_found() { return tx_found_; }

 private:
  bool ProcessMessage(std::string_view message) override {
    tx_found_ = true;
    return false;
  }

  void OnComplete(bool success) override {
    if (!success) {
      std::move(callback_).Run(base::unexpected(
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
      return;
    }
    std::move(callback_).Run(tx_found_);
  }

 private:
  bool tx_found_ = false;
  ResultCallback callback_;
};

class GetCompactBlocksGrpcStreamHandler : public GRrpcMessageStreamHandler {
 public:
  using ResultCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, std::string>)>;

  GetCompactBlocksGrpcStreamHandler() = default;
  ~GetCompactBlocksGrpcStreamHandler() override = default;

  void set_callback(ResultCallback callback) {
    callback_ = std::move(callback);
  }

 private:
  bool ProcessMessage(std::string_view message) override {
    messages_.push_back(std::string(message));
    return true;
  }

  void OnComplete(bool success) override {
    if (!success) {
      std::move(callback_).Run(base::unexpected(
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
      return;
    }
    std::move(callback_).Run(std::move(messages_));
  }

 private:
  std::vector<std::string> messages_;
  ResultCallback callback_;
};

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("zcash_rpc", R"(
      semantics {
        sender: "Zcash RPC"
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

const GURL MakeGetTreeStateUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetTreeState"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetLatestTreeStateUrl(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(),
       "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetLatestTreeState"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
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

const GURL MakeSendTransactionURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/SendTransaction"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetTaddressTxURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path = base::StrCat(
      {base_url.path(),
       "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetTaddressTxids"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetLatestBlockHeightURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetLatestBlock"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetTransactionURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetTransaction"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetCompactBlocksURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetBlockRange"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

const GURL MakeGetSubtreeRootsURL(const GURL& base_url) {
  if (!base_url.is_valid()) {
    return GURL();
  }
  if (!UrlPathEndsWithSlash(base_url)) {
    return GURL();
  }

  GURL::Replacements replacements;
  std::string path =
      base::StrCat({base_url.path(),
                    "cash.z.wallet.sdk.rpc.CompactTxStreamer/GetSubtreeRoots"});
  replacements.SetPathStr(path);

  return base_url.ReplaceComponents(replacements);
}

std::string MakeGetTreeStateURLParams(
    const zcash::mojom::BlockIDPtr& block_id) {
  ::zcash::BlockID request;
  auto hash = block_id->hash;
  request.set_hash(std::string(hash.begin(), hash.end()));
  request.set_height(block_id->height);
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetLatestTreeStateURLParams() {
  ::zcash::Empty request;
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetAddressUtxosURLParams(const std::string& address) {
  ::zcash::GetAddressUtxosRequest request;
  request.add_addresses(address);
  request.set_startheight(0);
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetLatestBlockHeightParams() {
  ::zcash::ChainSpec request;
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetTransactionParams(const std::string& tx_hash) {
  ::zcash::TxFilter request;
  std::string as_bytes;
  base::HexStringToString(tx_hash, &as_bytes);
  std::reverse(as_bytes.begin(), as_bytes.end());
  request.set_hash(as_bytes);
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeSendTransactionParams(base::span<const uint8_t> data) {
  ::zcash::RawTransaction request;
  request.set_data(reinterpret_cast<const char*>(data.data()), data.size());
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetAddressTxParams(const std::string& address,
                                   uint64_t block_start,
                                   uint64_t block_end) {
  ::zcash::TransparentAddressBlockFilter request;
  request.New();

  ::zcash::BlockRange range;
  ::zcash::BlockID bottom;
  ::zcash::BlockID top;

  bottom.set_height(block_start);
  top.set_height(block_end);

  request.set_address(address);
  range.mutable_start()->CopyFrom(bottom);
  range.mutable_end()->CopyFrom(top);
  request.mutable_range()->CopyFrom(range);
  return GetPrefixedProtobuf(request.SerializeAsString());
}

std::string MakeGetCompactBlocksParams(uint32_t block_start,
                                       uint32_t block_end) {
  ::zcash::BlockRange range;

  ::zcash::BlockID bottom;
  ::zcash::BlockID top;

  bottom.set_height(block_start);
  top.set_height(block_end);

  range.mutable_start()->CopyFrom(bottom);
  range.mutable_end()->CopyFrom(top);

  return GetPrefixedProtobuf(range.SerializeAsString());
}

std::string MakeGetSubtreeRootsParams(uint32_t start, uint32_t entries) {
  ::zcash::GetSubtreeRootsArg arg;

  arg.set_startindex(start);
  arg.set_maxentries(entries);
  arg.set_shieldedprotocol(::zcash::ShieldedProtocol::orchard);

  return GetPrefixedProtobuf(arg.SerializeAsString());
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

}  // namespace

ZCashRpc::ZCashRpc(
    NetworkManager* network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : network_manager_(network_manager),
      url_loader_factory_(url_loader_factory) {}

ZCashRpc::~ZCashRpc() = default;

void ZCashRpc::GetTreeState(const std::string& chain_id,
                            zcash::mojom::BlockIDPtr block_id,
                            GetTreeStateCallback callback) {
  GURL request_url = MakeGetTreeStateUrl(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetTreeStateURLParams(block_id));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnGetTreeStateResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      kMaxBodySize);
}

void ZCashRpc::GetLatestTreeState(const std::string& chain_id,
                                  GetTreeStateCallback callback) {
  GURL request_url = MakeGetLatestTreeStateUrl(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetLatestTreeStateURLParams());

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnGetTreeStateResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      kMaxBodySize);
}

void ZCashRpc::GetUtxoList(const std::string& chain_id,
                           const std::string& address,
                           ZCashRpc::GetUtxoListCallback callback) {
  GURL request_url = MakeGetAddressUtxosURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
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
      kMaxBodySize);
}

void ZCashRpc::GetLatestBlock(const std::string& chain_id,
                              GetLatestBlockCallback callback) {
  GURL request_url = MakeGetLatestBlockHeightURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetLatestBlockHeightParams());

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnGetLatestBlockResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      kMaxBodySize);
}

void ZCashRpc::GetTransaction(const std::string& chain_id,
                              const std::string& tx_hash,
                              GetTransactionCallback callback) {
  GURL request_url = MakeGetTransactionURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetTransactionParams(tx_hash));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnGetTransactionResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      200000 /* custom amount since transaction may contain orchard part */);
}

void ZCashRpc::GetCompactBlocks(const std::string& chain_id,
                                uint32_t from,
                                uint32_t to,
                                GetCompactBlocksCallback callback) {
  GURL request_url = MakeGetCompactBlocksURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetCompactBlocksParams(from, to));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  auto compact_blocks_stream_handler =
      std::make_unique<GetCompactBlocksGrpcStreamHandler>();
  compact_blocks_stream_handler->set_message_data_limit(2 * 1000 * 1000);

  StreamHandlersList::iterator handler_it = stream_handlers_list_.insert(
      stream_handlers_list_.begin(), std::move(compact_blocks_stream_handler));

  static_cast<GetCompactBlocksGrpcStreamHandler*>(handler_it->get())
      ->set_callback(base::BindOnce(&ZCashRpc::OnGetCompactBlocksResponse,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    std::move(callback), it, handler_it));

  (*it)->DownloadAsStream(url_loader_factory_.get(), handler_it->get());
}

void ZCashRpc::GetSubtreeRoots(const std::string& chain_id,
                               uint32_t start,
                               uint32_t entries,
                               GetSubtreeRootsCallback callback) {
  GURL request_url = MakeGetSubtreeRootsURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeGetSubtreeRootsParams(start, entries));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  StreamHandlersList::iterator handler_it = stream_handlers_list_.insert(
      stream_handlers_list_.begin(),
      std::make_unique<GetCompactBlocksGrpcStreamHandler>());

  static_cast<GetCompactBlocksGrpcStreamHandler*>(handler_it->get())
      ->set_callback(base::BindOnce(&ZCashRpc::OnGetSubtreeRootsResponse,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    std::move(callback), it, handler_it));

  (*it)->DownloadAsStream(url_loader_factory_.get(), handler_it->get());
}

void ZCashRpc::OnGetCompactBlocksResponse(
    ZCashRpc::GetCompactBlocksCallback callback,
    UrlLoadersList::iterator it,
    StreamHandlersList::iterator handler_it,
    base::expected<std::vector<std::string>, std::string> result) {
  url_loaders_list_.erase(it);
  stream_handlers_list_.erase(handler_it);

  if (!result.has_value()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  GetDecoder()->ParseCompactBlocks(
      *result,
      base::BindOnce(&ZCashRpc::OnParseCompactBlocks,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnGetSubtreeRootsResponse(
    ZCashRpc::GetSubtreeRootsCallback callback,
    UrlLoadersList::iterator it,
    StreamHandlersList::iterator handler_it,
    base::expected<std::vector<std::string>, std::string> result) {
  url_loaders_list_.erase(it);
  stream_handlers_list_.erase(handler_it);

  if (!result.has_value()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  GetDecoder()->ParseSubtreeRoots(
      *result,
      base::BindOnce(&ZCashRpc::OnParseSubtreeRoots,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnGetUtxosResponse(ZCashRpc::GetUtxoListCallback callback,
                                  UrlLoadersList::iterator it,
                                  std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
    return;
  }

  GetDecoder()->ParseGetAddressUtxos(
      *response_body,
      base::BindOnce(
          &ZCashRpc::OnParseResult<zcash::mojom::GetAddressUtxosResponsePtr>,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnParseCompactBlocks(
    GetCompactBlocksCallback callback,
    std::optional<std::vector<zcash::mojom::CompactBlockPtr>> compact_blocks) {
  if (compact_blocks) {
    std::move(callback).Run(std::move(compact_blocks.value()));
  } else {
    std::move(callback).Run(base::unexpected("Cannot parse blocks"));
  }
}

void ZCashRpc::OnParseSubtreeRoots(
    GetSubtreeRootsCallback callback,
    std::optional<std::vector<zcash::mojom::SubtreeRootPtr>> subtree_roots) {
  if (subtree_roots) {
    std::move(callback).Run(std::move(subtree_roots.value()));
  } else {
    std::move(callback).Run(base::unexpected("Cannot parse subtree roots"));
  }
}

template <typename T>
void ZCashRpc::OnParseResult(
    base::OnceCallback<void(base::expected<T, std::string>)> callback,
    T value) {
  if (value) {
    std::move(callback).Run(std::move(value));
    return;
  }

  std::move(callback).Run(
      base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
}

void ZCashRpc::OnGetLatestBlockResponse(
    ZCashRpc::GetLatestBlockCallback callback,
    UrlLoadersList::iterator it,
    std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
    return;
  }

  GetDecoder()->ParseBlockID(
      *response_body,
      base::BindOnce(&ZCashRpc::OnParseResult<zcash::mojom::BlockIDPtr>,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnGetTransactionResponse(
    ZCashRpc::GetTransactionCallback callback,
    UrlLoadersList::iterator it,
    std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
    return;
  }

  GetDecoder()->ParseRawTransaction(
      *response_body,
      base::BindOnce(&ZCashRpc::OnParseResult<zcash::mojom::RawTransactionPtr>,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::SendTransaction(const std::string& chain_id,
                               base::span<const uint8_t> data,
                               SendTransactionCallback callback) {
  GURL request_url = MakeSendTransactionURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader =
      MakeGRPCLoader(request_url, MakeSendTransactionParams(data));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  (*it)->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&ZCashRpc::OnSendTransactionResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), it),
      kMaxBodySize);
}

void ZCashRpc::IsKnownAddress(const std::string& chain_id,
                              const std::string& addr,
                              uint64_t block_start,
                              uint64_t block_end,
                              IsKnownAddressCallback callback) {
  GURL request_url = MakeGetTaddressTxURL(GetNetworkURL(chain_id));

  if (!request_url.is_valid()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  auto url_loader = MakeGRPCLoader(
      request_url, MakeGetAddressTxParams(addr, block_start, block_end));

  UrlLoadersList::iterator it = url_loaders_list_.insert(
      url_loaders_list_.begin(), std::move(url_loader));

  StreamHandlersList::iterator handler_it = stream_handlers_list_.insert(
      stream_handlers_list_.begin(),
      std::make_unique<IsKnownAddressTxStreamHandler>());
  static_cast<IsKnownAddressTxStreamHandler*>(handler_it->get())
      ->set_callback(base::BindOnce(&ZCashRpc::OnGetAddressTxResponse,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    std::move(callback), it, handler_it));

  (*it)->DownloadAsStream(url_loader_factory_.get(), handler_it->get());
}

void ZCashRpc::OnSendTransactionResponse(
    ZCashRpc::SendTransactionCallback callback,
    UrlLoadersList::iterator it,
    std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
    return;
  }

  GetDecoder()->ParseSendResponse(
      *response_body,
      base::BindOnce(&ZCashRpc::OnParseResult<zcash::mojom::SendResponsePtr>,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnGetTreeStateResponse(
    ZCashRpc::GetTreeStateCallback callback,
    UrlLoadersList::iterator it,
    std::unique_ptr<std::string> response_body) {
  auto current_loader = std::move(*it);
  url_loaders_list_.erase(it);
  if (current_loader->NetError()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  if (!response_body) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR)));
    return;
  }

  GetDecoder()->ParseTreeState(
      *response_body,
      base::BindOnce(&ZCashRpc::OnParseResult<zcash::mojom::TreeStatePtr>,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ZCashRpc::OnGetAddressTxResponse(
    ZCashRpc::IsKnownAddressCallback callback,
    UrlLoadersList::iterator it,
    StreamHandlersList::iterator handler_it,
    base::expected<bool, std::string> result) {
  auto current_loader = std::move(*it);
  auto current_handler = std::move(*handler_it);

  url_loaders_list_.erase(it);
  stream_handlers_list_.erase(handler_it);

  if (!result.has_value()) {
    std::move(callback).Run(
        base::unexpected(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  std::move(callback).Run(result.value());
}

mojo::AssociatedRemote<zcash::mojom::ZCashDecoder>& ZCashRpc::GetDecoder() {
  if (zcash_decoder_.is_bound()) {
    return zcash_decoder_;
  }
  BraveWalletUtilsService::GetInstance()->CreateZCashDecoder(
      zcash_decoder_.BindNewEndpointAndPassReceiver());
  zcash_decoder_.reset_on_disconnect();
  return zcash_decoder_;
}

GURL ZCashRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::ZEC);
}

}  // namespace brave_wallet

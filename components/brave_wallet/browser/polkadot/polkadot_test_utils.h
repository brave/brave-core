/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

inline constexpr char kAssetHubMnemonic[] =
    "lazy february across turn unique syrup gasp pass pelican achieve cable "
    "canal";

base::DictValue RequestBodyToJsonDict(const network::ResourceRequest& req);
std::string ReadMetadataFixtureJson(std::string_view file_name);
std::vector<uint8_t> ReadMetadataFixture(std::string_view file_name);

// Replaces the zero-based nth occurrence of `needle` in `bytes` with
// `replacement`. Returns false if `needle` is empty or if the requested
// occurrence is not found.
//
// Examples:
//   ReplaceNthOccurrence("a b a b", "a", "x", 0) -> "x b a b"
//   ReplaceNthOccurrence("a b a b", "a", "x", 1) -> "a b x b"
//   ReplaceNthOccurrence("a b", "z", "x", 0) -> false
bool ReplaceNthOccurrence(std::vector<uint8_t>& bytes,
                          std::string_view needle,
                          std::string_view replacement,
                          size_t occurrence);

struct PolkadotMockRpc {
 public:
  PolkadotMockRpc(network::TestURLLoaderFactory* url_loader_factory,
                  NetworkManager* network_manager);

  ~PolkadotMockRpc();

  // Call these before the other methods. Configure various knobs/properties of
  // the RPC calls we want to make.

  void UseInvalidChainMetadata();
  void UseInvalidFinalizedBlockHash();
  void RejectExtrinsicSubmission();
  void RejectAccountInfoRequest();

  // Reject an individual step of the signing-payload assembly with a JSON-RPC
  // error, so the corresponding RPC call fails. Configure exactly one before
  // calling AddReqResPairs() to exercise a specific failure path.
  void RejectInitialChainHeader();
  void RejectParentBlockHeader();
  void RejectFinalizedHead();
  void RejectFinalizedBlockHeader();
  void RejectGenesisBlockHash();
  void RejectRuntimeVersion();
  void SetSenderPubKey(
      base::span<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey);
  void SetExpectedExtrinsic(std::string extrinsic);
  void SetSubmittedExtrinsicHash(std::string extrinsic_hash);
  void SetFinalizedBlockHeader(std::string_view json_str);

  // Used to map requests for a block hash given a block's number.
  void SetBlockHashMap(base::flat_map<uint32_t, std::string> block_hash_map);

  // Used to map requests for an entire block given its hash.
  void SetBlockMap(base::flat_map<std::string, PolkadotBlock> block_map);

  // Used to simulate a network failure request for chain_getBlock. Should match
  // a key present in block_map_.
  void SetBadBlockMapKey(std::string bad_block_map_key);

  // Used to map block hashes to a series of events, as a hex string.
  void SetEventsMap(base::flat_map<std::string, std::string> events_map);

  // Add individual request-response pairs for each phase in the RPC for
  // assembling the signing payload.
  // For configuration, call the above methods first such as SetSenderPubKey.

  void AddGetInitialChainHeader();
  void AddGetParentBlockHeader();
  void AddGetFinalizedBlockHash();
  void AddGetSigningBlockHash();
  void AddGetRuntimeInfo();
  void AddGetGenesisBlockHash();

  // Registers a response for the metadata provider's latest runtime-version
  // probe (state_getRuntimeVersion with no block hash).
  void AddGetLatestRuntimeVersion(uint32_t spec_version);

  // Convenience wrapper for the above Add* family of functions.
  void AddReqResPairs();
  void AddWestendAssetHubReqResPairs();
  void AddPaseoAssetHubReqResPairs();
  void AddPolkadotAssetHubReqResPairs();

  // Must be called last. Sets the interceptor for the TestURLLoaderFactory and
  // ensures that the configured request-response mapping is used.
  void FinalizeSetup();

 private:
  void RequestInterceptor(const network::ResourceRequest& req);

  bool HandleMetadataRequest(const network::ResourceRequest& req,
                             const base::DictValue& req_body);

  bool HandleGetAccountInfoRequest(const network::ResourceRequest& req,
                                   const base::DictValue& req_body);

  bool HandleGetFinalizedBlockHeader(const network::ResourceRequest& res,
                                     const base::DictValue& req_body);

  bool HandleAuthorSubmitExtrinsic(const network::ResourceRequest& req,
                                   const base::DictValue& req_body);

  bool HandlePaymentInfoRequest(const network::ResourceRequest& req,
                                const base::DictValue& req_body);

  bool HandleBlockHashRequest(const network::ResourceRequest& req,
                              const base::DictValue& req_body);

  bool HandleBlockRequest(const network::ResourceRequest& req,
                          const base::DictValue& req_body);

  bool HandleEventsRequest(const network::ResourceRequest& req,
                           const base::DictValue& req_body);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender_pubkey_ = {};
  raw_ptr<network::TestURLLoaderFactory> url_loader_factory_ = nullptr;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  base::flat_map<base::DictValue, std::string> req_res_pairs_;
  std::string testnet_url_;
  std::string mainnet_url_;
  std::string westend_asset_hub_url_;
  std::string paseo_asset_hub_url_;
  std::string polkadot_asset_hub_url_;
  std::optional<std::string> expected_extrinsic_;
  std::string submitted_extrinsic_hash_;
  std::string account_info_response_json_;
  std::string finalized_block_hash_;  // Hex-encoded, no leading 0x.
  std::string finalized_block_header_json_;
  base::flat_map<std::string, std::string> block_header_map_;
  base::flat_map<uint32_t, std::string> block_hash_map_;
  base::flat_map<std::string, PolkadotBlock> block_map_;
  std::string bad_block_map_key_;
  base::flat_map<std::string, std::string> events_map_;

  bool use_invalid_metadata_ = false;
  bool use_invalid_finalized_block_hash_ = false;
  bool reject_extrinsic_submission_ = false;
  bool reject_account_info_request_ = false;
  bool reject_initial_chain_header_ = false;
  bool reject_parent_block_header_ = false;
  bool reject_finalized_head_ = false;
  bool reject_finalized_block_header_ = false;
  bool reject_genesis_block_hash_ = false;
  bool reject_runtime_version_ = false;
};

// Build metadata from a known relay/parachain name returned by system_chain.
// Returns std::nullopt for unknown names. The returned metadata has an
// unknown spec_version (set to 0); callers must populate spec_version from
// state_getRuntimeVersion before using it for version-sensitive operations.
std::optional<PolkadotChainMetadata> PolkadotMetadataFromChainName(
    std::string_view chain_name);

PolkadotChainMetadata MakeWestendMetadata();
PolkadotChainMetadata MakePolkadotMetadata();
PolkadotChainMetadata MakeWestendAssetHubMetadata();
PolkadotChainMetadata MakePolkadotAssetHubMetadata();
PolkadotChainMetadata MakePaseoAssetHubMetadata();

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_

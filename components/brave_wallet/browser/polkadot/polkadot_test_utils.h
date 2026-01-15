/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "services/network/test/test_url_loader_factory.h"

namespace brave_wallet {

base::DictValue RequestBodyToJsonDict(const network::ResourceRequest& req);

struct PolkadotMockRpc {
 public:
  PolkadotMockRpc(network::TestURLLoaderFactory* url_loader_factory,
                  NetworkManager* network_manager);

  ~PolkadotMockRpc();

  // Call these before the other methods. Configure various knobs/properties of
  // the RPC calls we want to make.

  void UseInvalidChainMetadata();
  void RejectExtrinsicSubmission();
  void RejectAccountInfoRequest();
  void SetSenderPubKey(
      base::span<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey);

  // Add individual request-response pairs for each phase in the RPC for
  // assembling the signing payload.
  // For configuration, call the above methods first such as SetSenderPubKey.

  void AddGetInitialChainHeader();
  void AddGetParentBlockHeader();
  void AddGetFinalizedBlockHash();
  void AddGetFinalizedBlockHeader();
  void AddGetSigningBlockHash();
  void AddGetRuntimeInfo();
  void AddGetGenesisBlockHash();

  // Convenience wrapper for the above Add* family of functions.
  void AddReqResPairs();

  // Must be called last. Sets the interceptor for the TestURLLoaderFactory and
  // ensures that the configured request-response mapping is used.
  void FinalizeSetup();

 private:
  void RequestInterceptor(const network::ResourceRequest& req);

  bool HandleMetadataRequest(const network::ResourceRequest& req,
                             const base::DictValue& req_body);

  bool HandleGetAccountInfoRequest(const network::ResourceRequest& req,
                                   const base::DictValue& req_body);

  bool HandleAuthorSubmitExtrinsic(const network::ResourceRequest& req,
                                   const base::DictValue& req_body);

  bool HandlePaymentInfoRequest(const network::ResourceRequest& req,
                                const base::DictValue& req_body);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> sender_pubkey_ = {};
  raw_ptr<network::TestURLLoaderFactory> url_loader_factory_ = nullptr;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  base::flat_map<base::DictValue, std::string_view> req_res_pairs_;
  std::string testnet_url_;
  std::string mainnet_url_;
  bool use_invalid_metadata_ = false;
  bool reject_extrinsic_submission_ = false;
  bool reject_account_info_request_ = false;
};
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TEST_UTILS_H_

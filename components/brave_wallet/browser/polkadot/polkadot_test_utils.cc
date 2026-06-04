/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"

#include <algorithm>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/containers/map_util.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

namespace {

// Default pubkey from our test wallet for account id 0.
constexpr std::string_view kDefaultPubkey =
    "14BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E";

constexpr std::string_view kDefaultSubmittedExtrinsicHash =
    "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a4"
    "23968d";

constexpr std::string_view kDefaultAccountInfoResponse = R"(
  {
    "jsonrpc":"2.0",
    "id":8,
    "result":[
      {
        "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
        "changes":[
          [
            "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
            "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
          ]
        ]
      }
    ]
  })";

bool IsEmpty(
    const std::array<uint8_t, kPolkadotSubstrateAccountIdSize>& pubkey) {
  return pubkey == decltype(pubkey){};
}

std::string ReadMetadataFixtureJsonImpl(std::string_view file_name) {
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII(file_name);
  std::string fixture_contents;
  CHECK(base::ReadFileToString(fixture_path, &fixture_contents));
  return fixture_contents;
}

bool IsCommand(const base::DictValue& req_body, std::string_view method) {
  if (const auto* json_method = req_body.FindString("method");
      json_method && *json_method == method) {
    return true;
  }
  return false;
}

const base::ListValue* FindParamsOrNull(const base::DictValue& req_body) {
  return req_body.FindList("params");
}

}  // namespace

base::DictValue RequestBodyToJsonDict(const network::ResourceRequest& req) {
  const auto* body_elems = req.request_body->elements();
  CHECK(body_elems->size() == 1u);

  const auto& element = body_elems->at(0);
  auto sv = element.As<network::DataElementBytes>().AsStringView();
  return base::test::ParseJsonDict(sv);
}

std::string ReadMetadataFixtureJson(std::string_view file_name) {
  return ReadMetadataFixtureJsonImpl(file_name);
}

std::vector<uint8_t> ReadMetadataFixture(std::string_view file_name) {
  auto json =
      base::JSONReader::ReadDict(ReadMetadataFixtureJsonImpl(file_name), 0);
  CHECK(json.has_value());

  const std::string* metadata_hex = json->FindString("result");
  CHECK(metadata_hex);

  std::vector<uint8_t> metadata_bytes;
  CHECK(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));
  return metadata_bytes;
}

bool ReplaceNthOccurrence(std::vector<uint8_t>& bytes,
                          std::string_view needle,
                          std::string_view replacement,
                          size_t occurrence) {
  DCHECK(!needle.empty());
  if (needle.empty()) {
    return false;
  }

  const auto needle_bytes = base::as_byte_span(needle);
  const auto replacement_bytes = base::as_byte_span(replacement);
  auto it = bytes.begin();
  size_t num_found = 0;

  while (it != bytes.end()) {
    auto match = std::ranges::search(it, bytes.end(), needle_bytes.begin(),
                                     needle_bytes.end());
    if (match.begin() == bytes.end()) {
      return false;
    }

    if (num_found == occurrence) {
      auto pos = bytes.erase(match.begin(), match.end());
      bytes.insert(pos, replacement_bytes.begin(), replacement_bytes.end());
      return true;
    }

    ++num_found;
    it = match.end();
  }

  return false;
}

std::optional<PolkadotChainMetadata> PolkadotMetadataFromChainName(
    std::string_view chain_name) {
  // spec_version is unknown when constructing from chain name alone; callers
  // must update it from state_getRuntimeVersion before use.
  // This means synthetic metadata is also unusable if it slips into production,
  // as no validator will accept a spec version of 0.
  constexpr uint32_t kUnknownSpecVersion = 0;

  // https://github.com/paritytech/polkadot-sdk/blob/69f210b33fce91b23570f3bda64f8e3deff04843/polkadot/runtime/westend/src/lib.rs#L1853-L1854
  if (chain_name == "Westend") {
    return PolkadotChainMetadata::FromFields(
        /*system_pallet_index=*/0,
        /*balances_pallet_index=*/4,
        /*transaction_payment_pallet_index=*/0x1a,
        /*transfer_allow_death_call_index=*/0,
        /*transfer_keep_alive_call_index=*/3,
        /*transfer_all_call_index=*/4,
        /*ss58_prefix=*/42, kUnknownSpecVersion,
        /*asset_tx_payment=*/false,
        /*has_assets_pallet=*/false,
        /*assets_pallet_index=*/0,
        /*assets_transfer_all_call_index=*/0,
        /*assets_transfer_keep_alive_call_index=*/0);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-kusama-json.json#L969
  if (chain_name == "Westend Asset Hub") {
    return PolkadotChainMetadata::FromFields(
        /*system_pallet_index=*/0,
        /*balances_pallet_index=*/10,
        /*transaction_payment_pallet_index=*/0x0b,
        /*transfer_allow_death_call_index=*/0,
        /*transfer_keep_alive_call_index=*/3,
        /*transfer_all_call_index=*/4,
        /*ss58_prefix=*/42, kUnknownSpecVersion,
        /*asset_tx_payment=*/true,
        /*has_assets_pallet=*/true,
        /*assets_pallet_index=*/50,
        /*assets_transfer_all_call_index=*/32,
        /*assets_transfer_keep_alive_call_index=*/9);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/polkadot-json.json#L1096
  if (chain_name == "Polkadot") {
    return PolkadotChainMetadata::FromFields(
        /*system_pallet_index=*/0,
        /*balances_pallet_index=*/5,
        /*transaction_payment_pallet_index=*/0x20,
        /*transfer_allow_death_call_index=*/0,
        /*transfer_keep_alive_call_index=*/3,
        /*transfer_all_call_index=*/4,
        /*ss58_prefix=*/0, kUnknownSpecVersion,
        /*asset_tx_payment=*/false,
        /*has_assets_pallet=*/false,
        /*assets_pallet_index=*/0,
        /*assets_transfer_all_call_index=*/0,
        /*assets_transfer_keep_alive_call_index=*/0);
  }

  // https://github.com/polkadot-js/api/blob/f45dfc72ec320cab7d69f08010c9921d2a21065f/packages/types-support/src/metadata/v15/asset-hub-polkadot-json.json#L969
  if (chain_name == "Polkadot Asset Hub") {
    return PolkadotChainMetadata::FromFields(
        /*system_pallet_index=*/0,
        /*balances_pallet_index=*/10,
        /*transaction_payment_pallet_index=*/0x0b,
        /*transfer_allow_death_call_index=*/0,
        /*transfer_keep_alive_call_index=*/3,
        /*transfer_all_call_index=*/4,
        /*ss58_prefix=*/0, kUnknownSpecVersion,
        /*asset_tx_payment=*/true,
        /*has_assets_pallet=*/true,
        /*assets_pallet_index=*/50,
        /*assets_transfer_all_call_index=*/32,
        /*assets_transfer_keep_alive_call_index=*/9);
  }

  return std::nullopt;
}

PolkadotChainMetadata MakeWestendMetadata() {
  return PolkadotMetadataFromChainName("Westend").value();
}

PolkadotChainMetadata MakePolkadotMetadata() {
  return PolkadotMetadataFromChainName("Polkadot").value();
}

PolkadotChainMetadata MakeWestendAssetHubMetadata() {
  return PolkadotMetadataFromChainName("Westend Asset Hub").value();
}

PolkadotChainMetadata MakePolkadotAssetHubMetadata() {
  return PolkadotMetadataFromChainName("Polkadot Asset Hub").value();
}

PolkadotMockRpc::PolkadotMockRpc(
    network::TestURLLoaderFactory* url_loader_factory,
    NetworkManager* network_manager)
    : url_loader_factory_(url_loader_factory),
      network_manager_(network_manager),
      account_info_response_json_(kDefaultAccountInfoResponse) {
  if (IsEmpty(sender_pubkey_)) {
    EXPECT_TRUE(base::HexStringToSpan(kDefaultPubkey, sender_pubkey_));
  }
}

PolkadotMockRpc::~PolkadotMockRpc() = default;

void PolkadotMockRpc::UseInvalidChainMetadata() {
  use_invalid_metadata_ = true;
}

void PolkadotMockRpc::UseInvalidFinalizedBlockHash() {
  use_invalid_finalized_block_hash_ = true;
}

void PolkadotMockRpc::RejectExtrinsicSubmission() {
  reject_extrinsic_submission_ = true;
}

void PolkadotMockRpc::RejectAccountInfoRequest() {
  reject_account_info_request_ = true;
}

void PolkadotMockRpc::SetSenderPubKey(
    base::span<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey) {
  base::span(sender_pubkey_).copy_from_nonoverlapping(pubkey);
}

void PolkadotMockRpc::SetExpectedExtrinsic(std::string extrinsic) {
  expected_extrinsic_ = std::move(extrinsic);
}

void PolkadotMockRpc::SetSubmittedExtrinsicHash(std::string extrinsic_hash) {
  submitted_extrinsic_hash_ = std::move(extrinsic_hash);
}

void PolkadotMockRpc::SetFinalizedBlockHeader(std::string_view json_str) {
  finalized_block_header_json_ = json_str;
}

void PolkadotMockRpc::SetBlockHashMap(
    base::flat_map<uint32_t, std::string> block_hash_map) {
  block_hash_map_ = std::move(block_hash_map);
}

void PolkadotMockRpc::SetBlockMap(
    base::flat_map<std::string, PolkadotBlock> block_map) {
  block_map_ = std::move(block_map);
}

void PolkadotMockRpc::SetBadBlockMapKey(std::string bad_block_map_key) {
  bad_block_map_key_ = std::move(bad_block_map_key);
}

void PolkadotMockRpc::SetEventsMap(
    base::flat_map<std::string, std::string> events_map) {
  events_map_ = std::move(events_map);
}

void PolkadotMockRpc::AddGetInitialChainHeader() {
  // Our initial call to get the most recent block header in the chain.
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");
}

void PolkadotMockRpc::AddGetParentBlockHeader() {
  // Chained call from grabbing the most recent block header in the chain.
  // Grab the header of the corresponding parent hash for mortality
  // calculations, like how polkadot-js's api package does. Implement the same
  // algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");
}

void PolkadotMockRpc::AddGetFinalizedBlockHash() {
  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  if (use_invalid_finalized_block_hash_) {
    req_res_pairs_.emplace(
        base::test::ParseJsonDict(
            R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
        R"({"jsonrpc":"2.0","id":11,"result":"0xcat!!!"})");
    return;
  }

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");
}

void PolkadotMockRpc::AddGetSigningBlockHash() {
  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just
  // directly hashing the block headers locally.
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");
}

void PolkadotMockRpc::AddGetRuntimeInfo() {
  // Grab the runtime version of whichever block we're going to use for
  // signing.
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "result":{
                "specName":"westend",
                "implName":"parity-westend",
                "authoringVersion":2,
                "specVersion":1021000,
                "implVersion":0,
                "apis":[
                  ["0xdf6acb689907609b",5],["0x37e397fc7c91f5e4",2],["0xccd9de6396c899ca",1],["0x40fe3ad401f8959a",6],
                  ["0xd2bc9897eed08f15",3],["0xf78b278be53f454c",2],["0xaf2c0297a23e6d3d",15],["0x49eaaf1b548a0cb0",6],
                  ["0x91d5df18b0d2cf58",3],["0x2a5e924655399e60",1],["0xed99c5acb25eedf5",3],["0xcbca25e39f142387",2],
                  ["0x687ad44ad37f03c2",1],["0xab3c0572291feb8b",1],["0xbc9d89904f5b923f",1],["0x37c8bb1350a9a2a8",4],
                  ["0xf3ff14d5ab527059",3],["0x6ff52ee858e6c5bd",2],["0x91b1c8b16328eb92",2],["0x9ffb505aa738d69c",1],
                  ["0x17a6bc0d0062aeb3",1],["0x18ef58a3b67ba770",1],["0xfbc577b9d747efd6",1],["0x2609be83ac4468dc",1]
                ],
                "transactionVersion":27,
                "systemVersion":1,
                "stateVersion":1
              }
            })");
}

void PolkadotMockRpc::AddGetGenesisBlockHash() {
  // We need to grab the genesis block hash.
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");
}

void PolkadotMockRpc::AddReqResPairs() {
  AddGetInitialChainHeader();
  AddGetParentBlockHeader();
  AddGetFinalizedBlockHash();
  AddGetSigningBlockHash();
  AddGetRuntimeInfo();
  AddGetGenesisBlockHash();
}

void PolkadotMockRpc::AddWestendAssetHubReqResPairs() {
  account_info_response_json_ = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[{
      "block":"0xa3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470",
      "changes":[[
        "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da96f72e0a390db2406281323ac697d46f10e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e",
        "0x010000000000000001000000000000002549373a460700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
      ]]
    }]
  })";

  block_header_map_.emplace("", R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0xa3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470",
      "number":"0xe812ec",
      "stateRoot":"0x660fd3729bf44973d4a0d8e7fe8c7641e5ee75f06e07938328f40f3360404e95",
      "extrinsicsRoot":"0x6683b881be64726d41b7046208424987636461d6c2d27fafd1ef27305c208ecf",
      "digest":{"logs":[
        "0x06434d4c53100101010c",
        "0x06434d4c530c020001",
        "0x066175726120be576b0400000000",
        "0x045250535290bef1467f9f2659ebdc4b394b1eb32dd468706c031b74dc1377221d782b00ea6296b87007",
        "0x056175726101017642ed848bf7839d8d522c3f13808bb24365679e13674cb75ff820757d5b57126af41c54d7afa758c09820c0879d44e039366ee6132d2f35bcc6f01d7e2d4986"
      ]}
    }
  })");

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xd5c74ee2e5347f396b637f3b25bfed717ceb533798fa5c470c626b09245dc4ca"})");
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x67f9723393ef76214df0118c34bbbd3dbebc8ed46a10973a8c969d48fe7598c9"})");

  block_header_map_.emplace(
      "a3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470",
      R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0xcf7164032bad56873d6753e8c7c3a99232699204e680d793ab3172cdf64c2bb3",
      "number":"0xe812eb",
      "stateRoot":"0x605dec0661e9d051dee09953449cbbb60f0b2476cb867b1a6b869d6aad2eb3eb",
      "extrinsicsRoot":"0xb8bdaf98b502f3d36120a24d3cdcf60af96e6ac1c10f7a9d36bce3e272ac6722",
      "digest":{"logs":[
        "0x06434d4c53100100010c",
        "0x06434d4c530c020001",
        "0x066175726120be576b0400000000",
        "0x045250535290bef1467f9f2659ebdc4b394b1eb32dd468706c031b74dc1377221d782b00ea6296b87007",
        "0x05617572610101f23a5412e25899a1820896ddf2b2889162911bafe77933758267624cf137c334d1adb71555a9d6ee2086f6d02901b269d9228853b3bc2e53b643313534474187"
      ]}
    }
  })");

  block_header_map_.emplace(
      "d5c74ee2e5347f396b637f3b25bfed717ceb533798fa5c470c626b09245dc4ca",
      R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0xcd5d0b2d2ebb6c07e930e57d13aa86c145bc03545508283e403e3e8c230b7416",
      "number":"0xe812dc",
      "stateRoot":"0x4b6049a34903b556c6289baa3d087bd5907972c89d12bc83fa82e347a0761f59",
      "extrinsicsRoot":"0xf5bfd271857b1b5053f4945d151aec4a5361c8d6fb7c731e3c1e590b4979837a",
      "digest":{"logs":[
        "0x06434d4c53100100010c",
        "0x06434d4c530c020001",
        "0x066175726120bd576b0400000000",
        "0x0452505352900580d4ec4ea12acf8aba8911ec5e81d96715aa74ecb955253a89386c7e770f4182b87007",
        "0x0561757261010154e8737cf43c00597d6771589b3d0577f4f16ab4dc983ee0deb5c99e4be04751ebc8d7ae4d81924e2eaeb993ec21dc7d9222cf837e5e93c28a707f29a3371b88"
      ]}
    }
  })");

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf7164032bad56873d6753e8c7c3a99232699204e680d793ab3172cdf64c2bb3"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "specName":"westmint",
          "implName":"westmint",
          "authoringVersion":1,
          "specVersion":1022006,
          "implVersion":0,
          "apis":[["0x40fe3ad401f8959a",6]],
          "transactionVersion":16,
          "systemVersion":1,
          "stateVersion":1
        }
      })");
}

void PolkadotMockRpc::AddPolkadotAssetHubReqResPairs() {
  account_info_response_json_ = R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":[{
      "block":"0xe89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3",
      "changes":[[
        "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da96f72e0a390db2406281323ac697d46f10e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e",
        "0x01000000000000000100000000000000bdd98fa7040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
      ]]
    }]
  })";

  block_header_map_.emplace("", R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0xe89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3",
      "number":"0xf56f96",
      "stateRoot":"0xdfdef333dc49f9083ea3bc8f075476d4a72c5f34596c10ef81cd83026e1153e9",
      "extrinsicsRoot":"0x4005234663e7885ea59d6e1a21f41e6a55e7c363098778c8d966d6d9f4e876c3",
      "digest":{"logs":[
        "0x06434d4c53100101010c",
        "0x06617572612028b1d60800000000",
        "0x0452505352900552a633f21315079c06500c78a0b198c2fec6b776d2e5a89fb31629b83a1e75522f7907",
        "0x05617572610101e3ff2ff746c0fe5ae99e7c66d05a99b6ebe79b5316e1796ac21f2a276b9672fc7348e77fbf6b8cf63368cc0ca0c5b593490c5af31361b61262244bb42393820e"
      ]}
    }
  })");

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x64fc8fc096c5ae976c484d4cf1d0ab0ab45ba7386185db73c2a25852135da861"})");
  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x68d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d539d7c3de2f"})");

  block_header_map_.emplace(
      "e89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3",
      R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0x910f058dca1089e202af72fa36a1f218b9807aefb2804845deec476c7df76101",
      "number":"0xf56f95",
      "stateRoot":"0x627c70115ad17fe48b12203e5c4f64741da2c4066b9229af893af4c1ce5866fb",
      "extrinsicsRoot":"0xa6da7aaaf9f88a7d405c2305fbfd33948cbe3d46f919f1abfaba10818e795689",
      "digest":{"logs":[
        "0x06434d4c53100100010c",
        "0x06617572612028b1d60800000000",
        "0x0452505352900552a633f21315079c06500c78a0b198c2fec6b776d2e5a89fb31629b83a1e75522f7907",
        "0x05617572610101698bb38428085853a155742448491abb966497763e77f51717165da2430a895ffeca232bbb72449adaa77a0dbc53a56dd27a8f03ee8f76abc9042493c5627707"
      ]}
    }
  })");

  block_header_map_.emplace(
      "64fc8fc096c5ae976c484d4cf1d0ab0ab45ba7386185db73c2a25852135da861",
      R"({
    "jsonrpc":"2.0",
    "id":1,
    "result":{
      "parentHash":"0xa4b930e78d719ecd8b1fe7188d252a64125be73be8caf8bf79a905f039890164",
      "number":"0xf56f87",
      "stateRoot":"0x2f7207c00e4bb61a780f8826f2acdbc71a542be73b894cf5cb05ac100477b075",
      "extrinsicsRoot":"0xf33f960ece73152b37f52adbd4fad63242ed640d6644325348b174d4a1346e86",
      "digest":{"logs":[
        "0x06434d4c53100101010c",
        "0x06617572612025b1d60800000000",
        "0x04525053529013b127db899f033548408932652dcabd2739e46499b041a96feeb89b0f918a173e2f7907",
        "0x05617572610101cb41ac1516773fda2511bec42139b7a23287e8d7699eb2bfb63d870831fe143ad03bf2744fed53fcfa992ecd9a32d21d0399864f76fd9f0e22d8103500b93201"
      ]}
    }
  })");

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["910f058dca1089e202af72fa36a1f218b9807aefb2804845deec476c7df76101"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "specName":"statemint",
          "implName":"statemint",
          "authoringVersion":1,
          "specVersion":2002001,
          "implVersion":0,
          "apis":[["0xbc9d89904f5b923f",1]],
          "transactionVersion":15,
          "systemVersion":1,
          "stateVersion":1
        }
      })");
}

void PolkadotMockRpc::FinalizeSetup() {
  url_loader_factory_->ClearResponses();

  testnet_url_ =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  mainnet_url_ =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  if (auto westend_asset_hub = network_manager_->GetKnownChain(
          mojom::kPolkadotTestnetAssetHub, mojom::CoinType::DOT)) {
    westend_asset_hub_url_ = westend_asset_hub->rpc_endpoints.front().spec();
  }

  if (auto polkadot_asset_hub = network_manager_->GetKnownChain(
          mojom::kPolkadotMainnetAssetHub, mojom::CoinType::DOT)) {
    polkadot_asset_hub_url_ = polkadot_asset_hub->rpc_endpoints.front().spec();
  }

  EXPECT_EQ(testnet_url_, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url_, "https://polkadot-mainnet.wallet.brave.com/");

  url_loader_factory_->SetInterceptor(base::BindRepeating(
      &PolkadotMockRpc::RequestInterceptor, base::Unretained(this)));
}

void PolkadotMockRpc::RequestInterceptor(const network::ResourceRequest& req) {
  url_loader_factory_->ClearResponses();

  CHECK(req.url == mainnet_url_ || req.url == testnet_url_ ||
        req.url == westend_asset_hub_url_ || req.url == polkadot_asset_hub_url_)
      << "Incorrect URL supplied to PolkadotMockRpc: " << req.url;

  auto req_body = RequestBodyToJsonDict(req);

  if (HandleMetadataRequest(req, req_body)) {
    return;
  }

  if (HandleGetAccountInfoRequest(req, req_body)) {
    return;
  }

  if (HandleGetFinalizedBlockHeader(req, req_body)) {
    return;
  }

  if (HandleAuthorSubmitExtrinsic(req, req_body)) {
    return;
  }

  if (HandlePaymentInfoRequest(req, req_body)) {
    return;
  }

  if (HandleBlockHashRequest(req, req_body)) {
    return;
  }

  if (HandleBlockRequest(req, req_body)) {
    return;
  }

  if (HandleEventsRequest(req, req_body)) {
    return;
  }

  auto pos = req_res_pairs_.find(req_body);
  if (pos != req_res_pairs_.end()) {
    return url_loader_factory_->AddResponse(req.url.spec(), pos->second);
  }
}

bool PolkadotMockRpc::HandleMetadataRequest(const network::ResourceRequest& req,
                                            const base::DictValue& req_body) {
  const auto* method = req_body.FindString("method");
  if (!method) {
    return false;
  }

  if (*method == "state_getMetadata") {
    if (use_invalid_metadata_) {
      url_loader_factory_->AddResponse(req.url.spec(), R"(
        { "jsonrpc": "2.0",
          "error": { "code": 1234 },
          "id": 1 })");
      return true;
    }

    if (req.url == GURL(polkadot_asset_hub_url_)) {
      url_loader_factory_->AddResponse(
          req.url.spec(), ReadMetadataFixtureJsonImpl(
                              "state_getMetadata_assethub_polkadot.json"));
      return true;
    }

    if (req.url == GURL(westend_asset_hub_url_)) {
      url_loader_factory_->AddResponse(
          req.url.spec(), ReadMetadataFixtureJsonImpl(
                              "state_getMetadata_assethub_westend.json"));
      return true;
    }

    if (req.url == GURL(mainnet_url_)) {
      url_loader_factory_->AddResponse(
          req.url.spec(),
          ReadMetadataFixtureJsonImpl("state_getMetadata_polkadot.json"));
      return true;
    }

    if (req.url == GURL(testnet_url_)) {
      url_loader_factory_->AddResponse(
          req.url.spec(),
          ReadMetadataFixtureJsonImpl("state_getMetadata_westend.json"));
      return true;
    }

    return false;
  }

  if (*method == "system_chain") {
    if (use_invalid_metadata_) {
      if (req.url == GURL(testnet_url_)) {
        url_loader_factory_->AddResponse(req.url.spec(), R"(
          { "jsonrpc": "2.0",
            "error": { "code": 1234 },
            "id": 1 })");
        return true;
      }

      if (req.url == GURL(mainnet_url_)) {
        url_loader_factory_->AddResponse(req.url.spec(), R"(
          { "jsonrpc": "2.0",
            "error": { "code": 4321 },
            "id": 1 })");
        return true;
      }

      return false;
    }

    if (req.url == GURL(westend_asset_hub_url_)) {
      url_loader_factory_->AddResponse(req.url.spec(), R"(
        { "jsonrpc": "2.0",
          "result": "Westend Asset Hub",
          "id": 1 })");
      return true;
    }

    if (req.url == GURL(polkadot_asset_hub_url_)) {
      url_loader_factory_->AddResponse(req.url.spec(), R"(
        { "jsonrpc": "2.0",
          "result": "Polkadot Asset Hub",
          "id": 1 })");
      return true;
    }

    if (req.url == GURL(testnet_url_)) {
      url_loader_factory_->AddResponse(req.url.spec(), R"(
        { "jsonrpc": "2.0",
          "result": "Westend",
          "id": 1 })");
      return true;
    }

    if (req.url == GURL(mainnet_url_)) {
      url_loader_factory_->AddResponse(req.url.spec(), R"(
        { "jsonrpc": "2.0",
          "result": "Polkadot",
          "id": 1 })");
      return true;
    }

    return false;
  }

  return false;
}

bool PolkadotMockRpc::HandleGetAccountInfoRequest(
    const network::ResourceRequest& req,
    const base::DictValue& req_body) {
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  // Note that it's the account's pubkey that comprises the last 64 characters
  // of "params" here.

  const auto* method = req_body.FindString("method");
  if (!method) {
    return false;
  }

  if (*method != "state_queryStorageAt") {
    return false;
  }

  if (reject_account_info_request_) {
    url_loader_factory_->AddResponse(req.url.spec(), R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": { "code": 1234 }
      })");
    return true;
  }

  base::DictValue req_json;
  req_json.Set("id", 1);
  req_json.Set("jsonrpc", "2.0");
  req_json.Set("method", "state_queryStorageAt");

  // Storage keys for Polkadot block chains are shaped:
  // xxhash("System") | xxhash("Account") | <checksum> | <pubkey>
  auto checksum = Blake2bHash<16>({sender_pubkey_});
  req_json.Set(
      "params",
      base::ListValue().Append(base::ListValue().Append(base::StrCat(
          {"0x", "26AA394EEA5630E07C48AE0C9558CEF7",
           "B99D880EC681799C0CF30E8886371DA9", base::HexEncode(checksum),
           base::HexEncode(sender_pubkey_)}))));

  if (req_body != req_json) {
    url_loader_factory_->AddResponse(req.url.spec(), R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": { "code": 1234 }
      })");

    return true;
  }

  url_loader_factory_->AddResponse(req.url.spec(), account_info_response_json_);
  return true;
}

bool PolkadotMockRpc::HandleGetFinalizedBlockHeader(
    const network::ResourceRequest& req,
    const base::DictValue& req_body) {
  if (finalized_block_hash_.empty()) {
    finalized_block_hash_ =
        "46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21";
  }

  if (finalized_block_header_json_.empty()) {
    finalized_block_header_json_ = R"(
        {
          "jsonrpc":"2.0",
          "id":13,
          "result":{
            "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
            "number":"0x1c06355",
            "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
            "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
            "digest":{
              "logs":[
                "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
              ]
            }
          }
        })";
  }

  if (IsCommand(req_body, "chain_getHeader")) {
    if (const auto* params = FindParamsOrNull(req_body)) {
      if (params->empty()) {
        if (const auto* header = base::FindOrNull(block_header_map_, "")) {
          url_loader_factory_->AddResponse(req.url.spec(), *header);
          return true;
        }

        url_loader_factory_->AddResponse(req.url.spec(),
                                         finalized_block_header_json_);

        return true;
      }

      if (const auto* hash = (*params)[0].GetIfString()) {
        auto block_header = block_header_map_.find(*hash);
        if (block_header != block_header_map_.end()) {
          url_loader_factory_->AddResponse(req.url.spec(),
                                           block_header->second);
          return true;
        }
      }

      if (const auto* hash = (*params)[0].GetIfString();
          hash && *hash == finalized_block_hash_) {
        url_loader_factory_->AddResponse(req.url.spec(),
                                         finalized_block_header_json_);

        return true;
      }
    }
  }
  return false;
}

bool PolkadotMockRpc::HandleAuthorSubmitExtrinsic(
    const network::ResourceRequest& req,
    const base::DictValue& req_body) {
  if (const auto* method = req_body.FindString("method")) {
    if (*method == "author_submitExtrinsic") {
      const auto* params = req_body.FindList("params");
      CHECK_EQ(params->size(), 1u);

      const auto* extrinsic = (*params)[0].GetIfString();
      CHECK(extrinsic && !extrinsic->empty());

      if (reject_extrinsic_submission_) {
        url_loader_factory_->AddResponse(req.url.spec(), R"(
          {
            "jsonrpc": "2.0",
            "id": 1,
            "error": { "code": 1234 }
          })");

      } else {
        if (expected_extrinsic_.has_value()) {
          if (*expected_extrinsic_ != *extrinsic) {
            url_loader_factory_->AddResponse(req.url.spec(), R"(
              {
                "jsonrpc": "2.0",
                "id": 1,
                "error": { "code": 4321, "message": "Invalid extrinsic" }
              })");

            return true;
          }
        }

        const std::string_view extrinsic_hash =
            submitted_extrinsic_hash_.empty() ? kDefaultSubmittedExtrinsicHash
                                              : submitted_extrinsic_hash_;
        const std::string response = absl::StrFormat(R"({
          "jsonrpc": "2.0",
          "id": 1,
          "result": "%s"
        })",
                                                     extrinsic_hash);
        url_loader_factory_->AddResponse(req.url.spec(), response);
      }

      return true;
    }
  }

  return false;
}
bool PolkadotMockRpc::HandlePaymentInfoRequest(
    const network::ResourceRequest& req,
    const base::DictValue& req_body) {
  if (const auto* method = req_body.FindString("method")) {
    if (*method == "state_call") {
      if (const auto* params_list = req_body.FindList("params")) {
        CHECK(!params_list->empty());
        if (params_list->front().GetString() ==
            "TransactionPaymentApi_query_info") {
          // Because this is a fee estimate call, we shouldn't be using a
          // real signature here, so we need to probe for our dummy
          // signature to ensure correctness here, which in binary winds up
          // being encoded as: <signature type> <signature>
          // In our case, signature type is 0x01 (sr25519) and our dummy
          // signature is all 0x01.

          CHECK_EQ(params_list->size(), 2u);
          const auto& extrinsic = (*params_list)[1].GetString();
          EXPECT_NE(extrinsic.find(base::HexEncodeLower(
                        std::vector<uint8_t>(1 + 64, 0x01))),
                    std::string::npos);

          url_loader_factory_->AddResponse(
              req.url.spec(),
              R"({"jsonrpc":"2.0","id":18,"result":"0x82ab80766da800dc8df1b5030000000000000000000000"})");
          return true;
        }
      }
    }
  }

  return false;
}

bool PolkadotMockRpc::HandleBlockHashRequest(
    const network::ResourceRequest& req,
    const base::DictValue& req_body) {
  if (const auto* method = req_body.FindString("method");
      method && *method == "chain_getBlockHash") {
    if (const auto* params_list = req_body.FindList("params")) {
      CHECK(!params_list->empty());

      uint32_t block_num = 0;
      CHECK(
          base::HexStringToUInt(params_list->front().GetString(), &block_num));

      const auto* pos = base::FindOrNull(block_hash_map_, block_num);
      if (pos) {
        url_loader_factory_->AddResponse(
            req.url.spec(),
            absl::StrFormat(R"({"jsonrpc":"2.0","id":18,"result":"%s"})",
                            *pos));
        return true;
      }
    }
  }

  return false;
}

base::DictValue PolkadotBlockToJson(const PolkadotBlock& chain_block) {
  base::ListValue logs;

  base::DictValue digest;
  digest.Set("logs", std::move(logs));

  base::DictValue header;
  header.Set("parentHash",
             "0x" + base::HexEncodeLower(chain_block.header.parent_hash));
  header.Set("number", "0x" + base::HexEncodeLower(base::byte_span_from_ref(
                                  chain_block.header.block_number)));
  header.Set("stateRoot",
             "0x" + base::HexEncodeLower(chain_block.header.state_root));
  header.Set("extrinsicsRoot",
             "0x" + base::HexEncodeLower(chain_block.header.extrinsics_root));

  header.Set("digest", std::move(digest));

  base::DictValue block;
  block.Set("header", std::move(header));

  base::ListValue extrinsics;
  for (const auto& extrinsic : chain_block.extrinsics) {
    extrinsics.Append(extrinsic);
  }
  block.Set("extrinsics", std::move(extrinsics));

  base::DictValue value;
  value.Set("block", std::move(block));

  return value;
}

bool PolkadotMockRpc::HandleBlockRequest(const network::ResourceRequest& req,
                                         const base::DictValue& req_body) {
  if (const auto* method = req_body.FindString("method");
      method && *method == "chain_getBlock") {
    if (const auto* params_list = req_body.FindList("params")) {
      CHECK(!params_list->empty());

      const auto& block_hash = params_list->front().GetString();

      if (bad_block_map_key_ == block_hash) {
        url_loader_factory_->AddResponse(
            req.url.spec(),
            R"({"jsonrpc":"2.0","id":18,"result":"some bad data here, not a block."})");

        return true;
      }

      auto pos = block_map_.find(block_hash);
      if (pos != block_map_.end()) {
        url_loader_factory_->AddResponse(
            req.url.spec(),
            absl::StrFormat(
                R"({"jsonrpc":"2.0","id":18,"result":%s})",
                base::WriteJson(PolkadotBlockToJson(pos->second)).value()));

        return true;
      }
    }
  }

  return false;
}
bool PolkadotMockRpc::HandleEventsRequest(const network::ResourceRequest& req,
                                          const base::DictValue& req_body) {
  if (const auto* method = req_body.FindString("method");
      method && *method == "state_getStorage") {
    if (const auto* params_list = req_body.FindList("params")) {
      CHECK(!params_list->empty());

      auto pos = params_list->begin();
      if (pos->GetString() ==
          // xxhash(System) | xxhash(Events)
          "26aa394eea5630e07c48ae0c9558cef780d41e5e16056765bc8461851072c9d7") {
        ++pos;
        if (pos != params_list->end()) {
          const auto& block_hash = pos->GetString();

          auto events_iter = events_map_.find(block_hash);
          if (events_iter != events_map_.end()) {
            url_loader_factory_->AddResponse(
                req.url.spec(),
                absl::StrFormat(R"({"jsonrpc":"2.0","id":18,"result":"%s"})",
                                events_iter->second));

            return true;
          }
        }
      }
    }
  }

  return false;
}

}  // namespace brave_wallet

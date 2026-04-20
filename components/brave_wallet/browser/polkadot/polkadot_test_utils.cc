/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"

#include "base/base_paths.h"
#include "base/containers/map_util.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Default pubkey from our test wallet for account id 0.
constexpr std::string_view kDefaultPubkey =
    "D6B2A5CC606EA86342001DD036B301C15A5CBA63C413CAD5CA0E8F47E6FA9516";

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

PolkadotMockRpc::PolkadotMockRpc(
    network::TestURLLoaderFactory* url_loader_factory,
    NetworkManager* network_manager)
    : url_loader_factory_(url_loader_factory),
      network_manager_(network_manager) {
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

void PolkadotMockRpc::AddGetFinalizedBlockHeader() {
  // Chained call, grab the block header using the hash of the finalized head.

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

  req_res_pairs_.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"]})"),
      finalized_block_header_json_);
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
  AddGetFinalizedBlockHeader();
  AddGetSigningBlockHash();
  AddGetRuntimeInfo();
  AddGetGenesisBlockHash();
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

  EXPECT_EQ(testnet_url_, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url_, "https://polkadot-mainnet.wallet.brave.com/");

  url_loader_factory_->SetInterceptor(base::BindRepeating(
      &PolkadotMockRpc::RequestInterceptor, base::Unretained(this)));
}

void PolkadotMockRpc::RequestInterceptor(const network::ResourceRequest& req) {
  url_loader_factory_->ClearResponses();

  CHECK(req.url == mainnet_url_ || req.url == testnet_url_)
      << "Incorrect URL supplied to PolkadotMockRpc: " << req.url;

  auto req_body = RequestBodyToJsonDict(req);

  if (HandleMetadataRequest(req, req_body)) {
    return;
  }

  if (HandleGetAccountInfoRequest(req, req_body)) {
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

  url_loader_factory_->AddResponse(req.url.spec(), R"(
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
    })");

  return true;
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

        url_loader_factory_->AddResponse(req.url.spec(), R"(
          {
            "jsonrpc": "2.0",
            "id": 1,
            "result": "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a423968d"
          })");
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

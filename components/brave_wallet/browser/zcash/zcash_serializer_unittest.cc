/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_ORCHARD)
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#endif

namespace brave_wallet {

TEST(ZCashSerializerTest, HashPrevouts) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 3224808575;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {193, 161, 45,  18,  123, 87,  200, 19,
                                   137, 118, 231, 145, 1,   59,  1,   95,
                                   6,   166, 36,  245, 33,  182, 238, 4,
                                   236, 152, 8,   147, 199, 229, 224, 26};
    tx_input.utxo_outpoint.index = 1493393971;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_outpoint.txid = {208, 145, 48,  246, 53,  17,  218, 84,
                                   131, 45,  233, 19,  107, 57,  244, 89,
                                   159, 90,  165, 223, 187, 69,  218, 96,
                                   205, 206, 171, 126, 239, 222, 137, 190};
    tx_input.utxo_outpoint.index = 3237475171;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x7db761d908021c98a19c43f75c6486275eaca3c11f9dc6cbaf66d3050c23b515",
      ToHex(ZCashSerializer::HashPrevouts(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashOutputs) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("630063ac", &tx_output.script_pubkey);
    tx_output.amount = 1264123119664452;
    zcash_transaciton.transparent_part().outputs.push_back(
        std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    base::HexStringToBytes("636a5351520065ac65", &tx_output.script_pubkey);
    tx_output.amount = 810835337737746;
    zcash_transaciton.transparent_part().outputs.push_back(
        std::move(tx_output));
  }

  ASSERT_EQ(
      "0x0dc9291fc891c10bdecedde449fa319cfa3f45cf7779423c2272c013d7fe0080",
      ToHex(ZCashSerializer::HashOutputs(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashSequences) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 1290119100;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 3797894359;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.n_sequence = 4015866081;
    zcash_transaciton.transparent_part().inputs.push_back(std::move(tx_input));
  }

  ASSERT_EQ(
      "0x17cae6cde4962f6eb86b350eb5a80d5576a958b4bd3438689e94ee387eb80f8e",
      ToHex(ZCashSerializer::HashSequences(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashHeader) {
  ZCashTransaction zcash_transaciton;
  zcash_transaciton.set_consensus_brach_id(0xc2d6d0b4);
  zcash_transaciton.set_expiry_height(10000);
  zcash_transaciton.set_locktime(1);
  EXPECT_EQ(
      "0xc632e4b84e69afe329c646d3eaa71935a8922f8f2236ba3603c439bdb939db83",
      ToHex(ZCashSerializer::HashHeader(zcash_transaciton)));
}

TEST(ZCashSerializerTest, HashTxIn) {
  {
    ZCashTransaction::TxInput tx_input;

    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 3224808575;

    tx_input.utxo_value = 1848924248978091;
    tx_input.n_sequence = 1290119100;
    base::HexStringToBytes("ac0000", &tx_input.script_pub_key);

    ASSERT_EQ(
        "0xb39969f0fba708491e480d80d4d675a1f1552cc7d479d7942f75fa31ad9c6ad6",
        ToHex(ZCashSerializer::HashTxIn(tx_input)));
  }
}

// https://zcashblockexplorer.com/transactions/360d056309669faf0d7937f41581418be5e46b04e2cea0a7b14261d7bff1d825/raw
TEST(ZCashSerializerTest, TxId_TransparentOnly) {
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_expiry_height(2283846);
  tx.set_locktime(2283826);

  {
    ZCashTransaction::TxInput tx_input;

    std::vector<uint8_t> vec;
    base::HexStringToBytes(
        "be9ef0f2091d0ef49f7f32c57ec826877175e9a703bef5989261e42bdfd69171",
        &vec);
    std::reverse(vec.begin(), vec.end());
    std::copy_n(vec.begin(), 32, tx_input.utxo_outpoint.txid.begin());
    tx_input.utxo_outpoint.index = 1;

    tx_input.n_sequence = 4294967295;
    tx_input.utxo_value = 751000;
    tx_input.utxo_address = "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi";
    tx_input.script_pub_key = ZCashAddressToScriptPubkey(
        "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false);

    tx.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi";
    tx_output.amount = 100000;
    tx_output.script_pubkey = ZCashAddressToScriptPubkey(
        "t1KrG29yWzoi7Bs2pvsgXozZYPvGG4D3sGi", false);

    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  {
    ZCashTransaction::TxOutput tx_output;
    tx_output.address = "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi";
    tx_output.amount = 649000;
    tx_output.script_pubkey = ZCashAddressToScriptPubkey(
        "t1cRrYHciuivZZ32jceb7btTpakYBaPW7yi", false);

    tx.transparent_part().outputs.push_back(std::move(tx_output));
  }

  auto tx_id = ZCashSerializer::CalculateTxIdDigest(tx);

  ASSERT_EQ(
      ToHex(tx_id),
      "0x360d056309669faf0d7937f41581418be5e46b04e2cea0a7b14261d7bff1d825");
}

#if BUILDFLAG(ENABLE_ORCHARD)
TEST(ZCashSerializerTest, OrchardBundle) {
  ZCashKeyring keyring(
      std::vector<uint8_t>({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}),
      mojom::KeyringId::kZCashMainnet);

  auto key_id = mojom::ZCashKeyId::New(0, 0, 0);
  auto address = keyring.GetTransparentAddress(*key_id)->address_string;
  ZCashTransaction tx;
  tx.set_consensus_brach_id(0xc2d6d0b4);
  tx.set_expiry_height(1687144);
  tx.set_locktime(0);

  {
    ZCashTransaction::TxInput tx_input;
    tx_input.utxo_address = address;
    tx_input.utxo_outpoint.txid = {20,  107, 157, 73,  221, 140, 120, 53,
                                   244, 58,  55,  220, 160, 120, 126, 62,
                                   201, 246, 96,  82,  35,  213, 186, 122,
                                   224, 171, 144, 37,  183, 59,  192, 63};
    tx_input.utxo_outpoint.index = 1;
    tx_input.utxo_value = 115000;
    tx_input.script_pub_key = ZCashAddressToScriptPubkey(address, false);

    tx.transparent_part().inputs.push_back(std::move(tx_input));
  }

  {
    ZCashTransaction::OrchardOutput output;
    output.value = 100000;
    output.addr = {212, 113, 78,  231, 97,  209, 174, 130, 59,  105, 114,
                   21,  46,  32,  149, 127, 239, 163, 246, 227, 18,  158,
                   164, 223, 176, 169, 233, 135, 3,   166, 61,  171, 146,
                   149, 137, 214, 220, 81,  201, 112, 249, 53,  179};
    tx.orchard_part().outputs.push_back(std::move(output));
  }

  std::vector<OrchardOutput> outputs;
  for (const auto& output : tx.orchard_part().outputs) {
    outputs.push_back(OrchardOutput{output.value, output.addr});
  }

  OrchardBundleManager::OverrideRandomSeedForTesting(0);
  auto orchard_bundle_manager = OrchardBundleManager::Create(
      std::vector<uint8_t>() /* Use empty orchard tree */,
      OrchardSpendsBundle(), std::move(outputs));

  tx.orchard_part().digest = orchard_bundle_manager->GetOrchardDigest();

  EXPECT_EQ(
      "0x5af5dcc1436a1746e8a702a1d7763e8c7b2857f0037c2bcf3b02bea8c36fe6d5",
      ToHex(tx.orchard_part().digest.value()));

  auto transparent_signature_digest = ZCashSerializer::CalculateSignatureDigest(
      tx, tx.transparent_part().inputs[0]);
  EXPECT_EQ(
      "0xd55f293e03d6f5b9e7275becb99749d5268b650bf193da862123bc5b0fd50dd9",
      ToHex(transparent_signature_digest));
  auto transparent_signature =
      keyring.SignMessage(*key_id, transparent_signature_digest);
  ZCashSerializer::SerializeSignature(tx, tx.transparent_part().inputs[0],
                                      keyring.GetPubkey(*key_id).value(),
                                      transparent_signature.value());
  EXPECT_EQ(
      "0x47304402203b9fdd43a88444b0d649079e21d0c8e71176ae23298696dd164bb5551f05"
      "182c02206f8321ed7a738e864f7b79e94310d50714be99e82148f1e4dc5feb39903e5780"
      "0121022b19592aabd4f5cff59b4f842ae09155cb0440019ad1fa80086ab35b3acf565b",
      ToHex(tx.transparent_part().inputs[0].script_sig));

  auto shielded_sighash =
      ZCashSerializer::CalculateSignatureDigest(tx, std::nullopt);

  EXPECT_EQ(
      "0x58fec6023369e7fe37e800ab83fda11e182dd7100a6a204d164a950bd3f72dca",
      ToHex(shielded_sighash));

  auto orchard_raw_part = tx.orchard_part().raw_tx =
      orchard_bundle_manager->ApplySignature(shielded_sighash)->GetRawTxBytes();

  EXPECT_EQ(
      ToHex(tx.orchard_part().raw_tx.value()),
      "0x024330112c31d0cbb0db4f77cefc9036ed215ea86ba2aee9a5de08953a8216282f5136"
      "e62a35a7a471c0a68f7a2b09915993244010af0ce69e863a0cdc68042b103dd62d315080"
      "c0bec76397d7c6ad365d84a6354d07049e484d0240410c589389eeb33ccbb564cc383b01"
      "383d4bac03648d6f155fc5d3d280f0f85a3687f17b0e90a1dc6793503417e53794980e4b"
      "8b5e1a18e454da912fa17818988a7b0b1006fedd657b795cc584843cb90f9cdf67912f20"
      "17dccdb3858439dbea6fadbeeabf0ac892bc6f2c59cc9344d9f1b781671ae7c9df2f4f69"
      "fce6c8215fa7feae4be87a0b5bb08266bead9d4ee0f80433cd17b713643a5f02ce53392c"
      "b015418fc4ff336ea32d7a749448802b7f828d51d51d78e707d268959c856d9239a6eff5"
      "090d1813581a19c24df421a03eea8bd7e9f8a9cf938c35fe65cf213a1a4b6c727e9401cc"
      "519f3ef3f2c936079149bc2af3083d8a963811c761138e44383c08c34de0d483f62879c4"
      "cb80c5cb84124da36abe113d68eadd017c5c268d6fbdd9c661753fe5375c8043fa3e9f9c"
      "94a8e1035bdd2e8eaa3a814d0a62657bc5fe689568b5a41ab718342b4790520e1bceb54b"
      "987d2590354b54fe695ca24c6585bb591cdc2ac395f06a07194812aeceaac0cdbc4178b6"
      "8be11e30352956b250378495bfaa4b326d264db71b83b28ef164300dee9a17f187617596"
      "c071f63d9383aeecec6f28e1080013e674b2412d15dd667e2532339caed06f4db6da25e7"
      "fcc326a1a58469d94df082166dbb24a1117898bf7c497466690d13949cd1db05a516622c"
      "b2d746469d04e716acebb04f1c519f00d26dac539a71c2e4c9f1efa0cec80ba0cf37a29c"
      "d375e0fa70146becfc66ef4cace008fc8a22ae77aa718ce5a149256df6407a24d33e5eb1"
      "e9d51b2bd2ba4d71b71f3bb54538e984f719bbb2daebec2a54476a4104daa47d591fd9db"
      "9820f68d48cf52dee3f8772fdd25de4ae436ea1cf125296c30e91262b407c80188224c35"
      "6572acdcca3f41fcefb5e618993b1214a542a06425439ac7c8812be6a0bb8ccd9abedd75"
      "202451ca522cff1643fda3e399bf60b2b7393a6f89dd8dca6a1c1ede5965975a62c0088d"
      "3a5e68c0b635535c053509c7f55bf9261ecf9b5c2ca045762df3966fe4c1e97af1c6e3a1"
      "ec3eb3f9b57bfdf44069a9d45105bc00a04f80f9e15f02b34aa42ca95ec8527ea52d52ce"
      "05750a1d67216a3b2f334682f65ce5ea49599bd7e01f98d14c07095635740c3bf644a7d5"
      "eb61c16d28a5bdfdfacb311a29f6b62fe50b71864f70937ba483b96ab0c4f6c566ddd1bb"
      "55536a33a20a8b4f9642f67e8e16662017b947df57320a51cd1141d8e46a40a8c9de24b3"
      "0c0b08957050b693dc18c5acc1482e07204cf113ac974dde5d62044ad29910f5a59256ad"
      "08352c7141f6b65043a86e93a3a25fdba8f6f72b60a9e5276da3e20f4e36573ed72aedcb"
      "4ae68ac15a576671933c1a102d172031fb0d586b1ffb70828ff5a3ff02b38d67153388bf"
      "cf5b6d02bb476225fc50786083d09fad59b0b97da176b7b4787277d4f90e090b8e6f370b"
      "38eada32da32734553d64b4681c21af7ffe2bcb8d914e43cf24a953a0f94edd975409bf5"
      "927de38e324ea2e52bf4590b0391dff3ded4354d0bfe2ae3acb4a9302124c73718ca2d2a"
      "30667be950fad78bb9fe49f8ffa40afb40a2ab26ecd17bc9ed16dc0e848550825fd093c6"
      "f1c4eafcb1c7c9859602f7fede5fbe5becf84f28bf74abc880e56b77ed7a7242448da180"
      "f07ffe854a7fa321a2722cdb9c1c46b93186ad15001b32c0a7de4a02a1526b334d97c2b3"
      "6f6d0fe83d3f219837c2adc186d6ef1a3f1e0a50d6ca2fc1c2c2a2ea95373d6a2fd9906f"
      "49296f3e8bc141a8c6bbc904722c2541756622d7284e45697c6f0f1a141b786e80fe8823"
      "91d7c5919bb98e2fa1719da21c9e4d04a0c30888b0b608d46a054c32b75bee0396efdeb3"
      "10e8c5211d41728ab796a44cdb543468e0d655cf64afd62e71dd4688eb7e2e196025f1ef"
      "0c2cbc99cc08075cb36b99cca1600edd3f6451eedff9c3adeb7b843deb83a5582e51ad32"
      "881b8ca6064986a41d26d91d0c48adef928d766c7f58bd9a31da0096c7bf18a9f8c307f1"
      "4bc82ae3601b82407069e4dd8a2c4b72909e2810435816ed3747e85302c90381e0dee223"
      "2d6c21b55310b76fbdaba0aa2deb8a1c25641604b1bf523281c8f7c5a80a93d967ced997"
      "8830b45c2dbf64d9ca8cdc6498082ae003d50eafee3c930d40d253f4bfe65ec69e7260d3"
      "b99dbed28acab956ba79ff0107d586c878760caa9ac9036079feffffffffffae2935f1df"
      "d8a24aed7c70df7de3a668eb7a49b1319880dde2bbd9031ae5d82ffd601c6d8bb1d60347"
      "0a10040bb50333498933da7cac03eb16937cef9d35d9371046b2d13387711416e9c51d06"
      "6f32fdfae7203369a1449da2700c37bfdb099d24a2be7f8abd0833762284617c9150b81e"
      "04d4051e0aaccc7100179b997b8fa7bfdd305d4183f78f8385336c189426bf0ff42bb5c7"
      "66a93d2ebb4f3fe89835da45c106c9dc861332d6f3b6705239303cd9b60210a9a2665329"
      "34511322f6ba8eac228a2df4ddddd778bad54694afc2f7d4159a3363361c4b677d723a7f"
      "b56a6f0f2914e7710b0ba8b8d1c209db43bb4911677f1bc1f7fa3faf5d4498753761c08b"
      "5129fe69223b94ebb00548ab19310d28c02a3930c68b0cdfa548106476c7d98b46b56e44"
      "1701fa115d58c76a523ee2711d13b2b75e45ac466ed3ca501b690d89a92047c72cfe3dbb"
      "ddd9ba784458ac8520e369993b31535f3585b7e0679a5dabbcac553e904db5b8f5884679"
      "ff1fd49ff4cb9f57ee3fba42e5dfce4de71f26f11c94da728f9d159ce803c145fd1fa307"
      "0d6c6d4318d303d050334374a84eaf66ba2f9672239e32ad301a542d3d569c94d23a9a9b"
      "424069c4a66261fee5ff3c66282f6ed9e89898f5ca0c0b26ff37f21633bf35cacfc848fc"
      "192a7be3641eeadd1d34641912dd53e4400391bc5341ff004f14dbd35fe96960f86062df"
      "c49092178ab0b05a87867c4656825f5c818da35d74628ec85d08d398644573789a1018c1"
      "a21a4acd1012a10086b00f96d6083abfd79337c0a3c44dae81cd14c2a4cad615668e0407"
      "bd305f77179d26b17bf314c0437848334eaab3ea92f27ed59ef928b7d03b5bf1b910033e"
      "b416772a28e363999839175d75bb67ad350df97ad2d20a262e2e8c72e0ee08371d838264"
      "5a4237e2327d6c1ed602a04e0f5658776884234c32b15a98a7cb4310cacbdfddf80cbde4"
      "28705cd193a7bd2fcae9b8541f06557348226a5d18f9668b4bd86ef9171305ad9649ad36"
      "481766eb4da18047bb78528fc839226c0f4b13815fd65501a5f43ba7f5cac5249115baa6"
      "c78d6922a92035497f93377b38483e9902ecb495027105254667ac1039e6abb8a70ad170"
      "642d79004bb9e5dea33d6caa8faaf6ce0f8edd82b5db64401228e8e590f2af64bfb58dc7"
      "fd07f449e6a57fd1245c5f3b21cde2e32d73f28497654d53fe4a9fc6a9d9c5e07f1f8b1c"
      "ec16aa279ae4a89b88a58f04854b5c29fe6ddfce9906f8fbdb3454fdcba69b599935523b"
      "8dbffa48e0ebbb6c7e49f8b6cde4991ae5ae8e73ad4e3c88d1be12dae46e9c23a50ce7ae"
      "0b9e07c1a9efc075dd2d7e2a215042b19ddc8c28f41a20415620825bccfb9cfa07e1280b"
      "646d591a863fe6b7b007a8b946e51062a3370aed325d2e7dda1ae4bf5c412c41b2aada6f"
      "583af999aa55e63ab52672218c1efa4197e73d1e1eaf8cb9a0f0f790db781952ecc670a0"
      "bf0cac8285ec02133c206d186cafd560b6de563c813d57993db2e57733999c09e1171dc6"
      "f9c3fa3a4da170b4e8895587f389d24b395b5eeca1d8bb9445e022624cfbad4cf20b115c"
      "549e8e6235bc3ebeb07c30275dedb5fa2912f948b372c4a58c51afe63f62eb90e73d7f51"
      "e8db044719959b769753725c1e67975c8ccd51a5480c8ef45dd5aff6bda0038ac67132a3"
      "74fa6d6b75e0a4941a9e362fe6f5f2dc72f8066ca9489c81dc2fecc7c3c14dc19c8f89c7"
      "cf38dcb0f89c95819c022f7765de3eb69c28c29d2b2a428018cc80a0c8ef8698ed8d2568"
      "19f8dcac8b2b5cc499fc10dec93deef6fca8c2d80ae283f9668db5329eb2cd1c9cc92ec7"
      "79cf0014c8bffc5b4f283ba484369f057e0c075d5276dc6731186889941fbe199076c458"
      "0dca0bdafaacd7c6c13494d42c948abeede8dad58dce340e041296befb48b3a2738f0948"
      "cfc1e556043283154a9779c3fff7afb164b44af1e13738ee3544cb8c34530e9408278eea"
      "c1249f7491983b21b09e85536cc62132105f98030f4b64827db9e9e4e2184d80100ef264"
      "e5e22708fd7c7e8cb3333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a1bc8f3a712ea0"
      "431273a24c245a449e9c8e441343029afec9edeb101c1d8c9dbffabc1d17c6818d4d4f7b"
      "96217bc5c7aeb843ae250aa8f946dade5df1e9e6882695ef58e43995b8f68b6fec10c01b"
      "0fc4a37d94e9c3880effda57ccdf9d72e5a1206288db08812639cf3a3b3bfa76646af70b"
      "7927e49cff1977a4368021c0f604b836e9b876f6b9140f5045e88f268bf798a72b04b8df"
      "18a4aae9be79c27d483eb39f7632a18a351d87870fa1c5105af75dff136f44e5edfa6e77"
      "e280330c51a8e841a8a3581b60794cfbe6ca3998373ea4a2a589d6346a9b0988ba57a852"
      "368d762ef9c4c3de464f971212a4ec6fd7dfa9a525d0be17f43b97d3240b09b33c3e8cc5"
      "ae2dbc2a736af44d55d3acb18c226d292157d6a59243483efb6547464208004e07dbd0d0"
      "4f4487d1fdeda1a3df63a31239beadf1bb0368a4677d5147ef2d18a9a91cf0105dccbf56"
      "f04516bca3fee8194c9b76ae71ae2f2f2e2cfc58c70dbf6054c721b0c5b08551b0387c3e"
      "82ca272128c70ccd0057ed4eba9f9d08663753d57d8c5ce309799361f8009ede59963f46"
      "57b2ff7c95b997e577f6242ebe1c137c1c3f03b4eba314097927e7fb98fc9ba76c141bbf"
      "fa705526a511e75ca52d08c1bd88908df10d307adf603119db56217c2db92d47643d972c"
      "6e5c431d5d32c0783e464bc058a2e3207fec16f0a0cfe44d5586260b82a625b383710a37"
      "fa33f662733dd364e49137712cd57223b76773cfcfdf845b7659ccdada5211c2f12889a4"
      "465caf3f138d7a90b2d202c0451aef799175be5f3af781272e730985c62c30e445005801"
      "3e0b2ca758c0b27e9213788cf187a1e4ec8ad456bdf9a0308305f98f304560216d3a95f0"
      "78291e66cb9e8f6276cb758b4940a997b6063d41372cbc9bc67edc56510d572eb0bf7d35"
      "226dc5795fc6c952cd6799212ac527907e18fe78da43121faa1e3ef77d39bb201a5fd779"
      "6c8bc04b0dfd6a84d9d12cda260b183bd0fde7095c1a78ca0f3de835f91c1b075356bb04"
      "4f61f90ecaae69b9293fd8560912245cf7914230706f611dfa4c47a19f2b2472922fddc8"
      "667b9cdc4f212c4e1e63d8308ef9558e248141e4006be8b860a049435ad183427619c0a6"
      "fa15e81b8a16a438f061a178381791e4026127c378a1299ab51937bea30f99ccd63be778"
      "05e274e597571f7ed0dc534eb700605ea3b2a5f6d31e4f228c6fcd56fc2abfb234dccf80"
      "351ec3ad9024d844bbc3b22b476fc846465c900e9ac021594e0c7e46cd1ad81a6b6aa4dd"
      "5921adad1f15b87b2b0b0236ab0356dd7e127c854d11fcebdf43cce59acd70dc405a98ad"
      "51dd6cc2858baaa81eb5a56abab22799c30c152627af052ef94c1d80f28afe0aa1498168"
      "07baf5bd1c85da9d40d2336ee0287461a026266af7cfaa64b952a5eeea67117db0243c33"
      "747a68e60e07facbaa01d6e4e7f992493b594b2b0d839705e8942207b2983a9ac8084734"
      "42a9fb47b3292cae218986071254c4065eb41c28fd4e68c0c55b36984ebe755d712e5014"
      "672b88b3efd71c9a70b1b2af8da4c5484f0708aa9eb7d64f42dc08bf5cbddb2d1e38831d"
      "7a511ec2f4026caf2e443a86f2f9afdaa21b681d80088e98500f96f0bd31ad386015b85d"
      "ceb49180ed9360b862b3dacc3613c83b5bdf636c55c7a2e5a22a9a94ac085197b741b650"
      "f262a6ca4a02db4ee1bf04387b1cfe838a2013f66a026b101842fa4e44965a80bcc9ff05"
      "69503a39fe35e7469a90541674a04440e70353ac24a7d2d98862da84c34fb7b79983e209"
      "f29e4ad6684deab378079333f42c1513a986063d8243d453148c42f10f88fdf4610fada4"
      "91b781b7a1d9d890ea1883862bd620399b1611d543613d2202fe86fff14c155568461498"
      "d0780e3ebf1fe2c0f2f857f6cfa2ac4ca1088cc7051463dd4fc6c1c297b627c54af3ea97"
      "250af20e585fa06fe2212f751f397073e8ab67630a75bb5c4d6f2f343b4cf0ba851c16fb"
      "904e5a1fe66f0b2e8bac618fd2cc490c83216efe87f759705feef7e1df3c0ca649f641c1"
      "488aa2fdfc4159fc34c0d61b06c9d5ad1816bf7210361497d73392ab294d3e6214deb3e6"
      "429037ef38add1d172b9157ddc7d078f5578704da7292a03cffff71145c5124a8fe9a46a"
      "7da10c071196d1f9175c6e53585756980124ed7744922acf08378ad0776d4e82df020b14"
      "d1c2384ece2091373a5a0c8b7301391a4c73066178a1515285b2e4f1482858ea91c59af4"
      "9bc5e3d268f5140ae02d78edfc715fb5052b8fa99d2fb2287832231ed38d98432f5afdd9"
      "57a06ad11b0a58da42228f3106508cb04702ce79ffe526a5f5dee1e9d026ced1ee06eb92"
      "1109120034556432b89f6565d10f4b9cf7248faeddbd94a317da93ab8d7c93524d0fa2d2"
      "70398acd02d36a43f6e8e460b4fbec62194b73469be9066968cbc10bea2acb1b98369a38"
      "42975b940fbc875476c1fa7400ee95733ac6eeddf8353e70d60c164d4b229bc361d7e38a"
      "60453445190caf1dd3684c491efa3e0abee650c21c37734b6f8cb2ef851e7e8d98aa5d23"
      "99f1768a2cc81020b59371d16510bc1e38374922ef26159f629fd7bd7bc40b3ddb4ea2a1"
      "c7bc4da6504f83beca6e5163a12f72b26ed6fd219e5edb7d316f712311f57ee16ebdd215"
      "3e981a4890ab63440208b3dbbcf7f886ea217cb82608a1f606559268d4f0d1ab16f4bc0d"
      "cef971d8d3218bfccab87f704de90c418ebbda7f088f4a52130d0fd12bcbbbe6465b96ad"
      "8e111042560de47aad79bd1473c882bad2fce71b4c78a3b99e4f9b405e84fddd6815c6c4"
      "059cfb4b896880fb01e01fee33c9113a075c1107824b342545f984d90d0cd29e951dd482"
      "ec068a6c4fbc02f86ba5ca1bda0d847972287e6715cf5975d135c14bc8a9703b1aa6294c"
      "44a50979d0d789fa997a41f00899002a0f2928b6bf2b57dda37ea5d38d06e12734b88f56"
      "6e5ac6d9be1f48afa815d7f2f0841d3177105b348379525fcc9a6a5fd14953ca3947ca43"
      "cde56eabf0be2867679a632c291b8354fa0e4a0f9ba5ba2f2122c4e31d2001c0ea858b46"
      "6dcc3c1c45cf8b19392073a45ed261f64878021d18f43520238d0205dee2cd3e6ef878d4"
      "07f5f5cb3832204f4b80ee92174bf3f470c6e1800499d8d16f03c0d2946c460127a873dc"
      "46021ceeb84c34d136efdfdce8283daf3aeab90aa4f1256d5509838c96349cb88f1b817c"
      "94177e89271f3c464b7dab45868a80f52740ec504334b87b1ffe7315d13e76ea8379873b"
      "89c652e7ac0984475231e71c67144a0c59924f96dc800d09b00a3a27f86f62f79317f7fc"
      "439a5e235fce80908a60256543b5f3564a8205500b26e4a7d35bf60251ad8d52c8d44a6e"
      "3d0b111aaccbd813f1aa042b554b911bd3385e26be1af29cd0d3afa204e922a2ee039875"
      "df3fdb5c65636939a186304f9e3095bb82cba060b043e138f36ff275e00ca27ec7a57a91"
      "853f19ee520d9a3a9e1ab97d57683e6091229180b6167d566982ef0865a70e487e27116d"
      "7ca227e7500765d4ca712cffd73dea70f138714221f308548c2fe9697a5a8204cb00dab4"
      "55223b21dbda4ace383b4c2d5cb8a80b43a1cf80a85bcdbae443e7cc65931b7d9c395a09"
      "d85aa04a0b896a1ababc3de81b258f22af403b0e45923c8ef7d2f68aa224aeb1261cfa4f"
      "645649c5f9597037eebfc066e517495d983e1f0461c651b4e425aca81ac92f7d624688b6"
      "15d61b0dc20bacf56710803c549ebf9a558f652ba4052026c5725e0f17a016eee0e070c1"
      "a857b8bd6d920397072b5cda654653a4ae18167007127ede81aea309b7a226293309449c"
      "3df4bb06bcfeab20b9fd0f7a682a557269bfb1030c7d87fa7337fec56a7126e553a65869"
      "b94b8d41925d9fe4ca15de3b916d497206bf80fbd657658c3cd59efe2d3e16f9886d48eb"
      "21ac931e9831fef5e1d8b0c28b0935378da0ab01455fb9e7e0305b638995f6dc7c29cde4"
      "293f871fd0318419a5fd289a1dbc78bff788fe84c5d85b0eabde25192665f1a9611dafc3"
      "64dd2c76548e56c0c09f33ddcddacdb858e689ef60bc9024069c836cd9231a25c204a450"
      "c7a141ca2b6283b46f39aab490f368d5ecd8dbc2856ef94d48106550406fce98f9cac770"
      "3694869d9e9d9d56adadd6e5d7938503fed3e7f11f13ec55c87f9582e6b6b803f4359e4f"
      "1b1f0fd64137140da024169208957ec32301644b68fea7730fe73b03be378ab66d6ba0d9"
      "a95f44572e577ce0af4d5b169d0f5262e70a8056d0e9c9aa8c7e199b0853126de575fcba"
      "6c7157789f46cd81870369218875e27ed132b80688035798300e01d621ce19a80a712a15"
      "2d04eb4a1329eef9c2a76b3d8ef87aa5a4e5a0df0b5d86b5f6cf2a30a14e0eea06fc7bbd"
      "9f31bb761e527b1a302a709e048db0829bd834cfc38ff0d8656eaf35dde2948b9001848b"
      "61ca80842be9eb2d821f4968e4a5ba17c407d0614e305b4867eb10f6fe0fde52c620f171"
      "ee0e8431d95c2fe6ebea9dd3edef4e38e1c06a447f238aaeb00aeb3a2deeaf11def8c703"
      "06e30f361000bbab84bd2b8d00f7489e11cef6ccf22b2f73f5df1bd8ceaa96c72c2f7347"
      "9ef034457bd48f180f8c25885f7317578934adb4d88d0b9e0716f011c7ca6bdba21d9a22"
      "87bba225bdac9ff40456ecfe0c110dd2a1d02ed119a5219b5532def6e59da498dd52fd04"
      "50fc239de044a71e773c66435a1abb5a717ddb44bf0bfe38e606f3eebf545a6932428ac4"
      "e03891d6d124f97eb62b06e3ba4dd1c881a96de13e30a1dd4f609568cf3509c4aeb66df6"
      "ba0aa368ba6840f117e499d1a2db7f300623ed3fadde6513cd414a8767a153f8e514260e"
      "5fddc12f74cd7b2abe5a8a12268d3e0f0ce4aeb1cb02619795b7a9564f04bf7a66751f45"
      "b8c55581fdbb1def6bec2bc591361e6c5f655c154bebe62bda03d0cc13781351a6383481"
      "7768044125204b2ccbe4a34ad8787913d19c51c5fe119692ad88701e41f410d2e551b4b4"
      "485cc6104ff8f3a96e60d441b981914b6f3a6e0a103874f5c8b5279c822380e1388f8c4e"
      "051806df43d088919b6a96651a1fd18978d9f7b7303d81086fc802e80dbacddca3c67f75"
      "c31f9296a6a15671b828ffa2144d7d28aff2ef13981c08f29ceeeae61a4abf670a177bb6"
      "6ee9eab7ba051fcea86c57279c74792c0db9d7be07b568a29be798e5cabe3f5922cb9bfe"
      "291d5cc733bedd48be1e0b42580ce82b2d192bc91cf834eed96e2640493f452eb90eebfe"
      "decbb6eef637fdfb36c58fbbf19f34d15c965e67f1bd19f8a86954190813ec1f32f9858a"
      "d282f3c3bb9e6599a60ff5a450bbee8a0fda246f1d98ec49f71d3dddd8a3b89412327982"
      "0f5e5eab5ee35be19ccdba7484205dac689e5af43f30de1eea0415c1d5844265ceb11975"
      "5aab460fbb69450d1385e0917389dd2a8a254c9e11779fd80912f282c97cda530d180258"
      "8e37c57ebbc0fdd6b34afcd00008e3f2860c029e87bd0c9cf9b7c875c78138e181c6b821"
      "80f1c2a324157dc1821a868d1fe05337df91051b0a712662f8174fd1363548e629e1a7ac"
      "2468ef99a4249640812f0cc2793c330e7ae5d8e6f58ba598a75d378dd30fc43825e54938"
      "2f0998cc63a91bd59562fc019463e2ee0288ce8b972d02bfd6931c71e96f1438941f73a9"
      "fb0645e21bd830afa898cfadeae7092dc212ba3af1bfede863ac1868072674b962c28f9f"
      "fcc7c0b41eb3ce3f9b233bfeb7c73c1af34eb770f211e36d213d1327c5e115ec13741598"
      "df6924ecd5c49d92198e0690e5853163868077885525e2149476914ecd2da886cca521d9"
      "d4326fd0767d4760e02ebd300cd94f158439b3bce3e80daada688d0d82201e03704a0b7e"
      "817b822ff42671ab1178c0fbef25872f62408a9918696ebfaa016ac83ab0ae6d7e564e4d"
      "ab9cdf72c2be75e11a2e3bd421fab646c18bffe2b414db0ae47a2178b1fedb660994277d"
      "f443daba0335987650196c1459fec34300902fa7e093a319815ae9ca77a05e36cfc6ca1e"
      "921d38e5ce1d435f0780add12a7866d0e09039ee77696fa71b98b2ddf7cda608aa3d711b"
      "36c0d7b7f00562173c3b1fff6d4b8bda89b59946a0702a1ec432d3369e03707faa4c8e40"
      "725e3a93b1b3250990aba031b44511c2427f69e82b75f5ebdf39365ef9785786c9db0219"
      "b79f2520478d52490d3cd7f8c369f82ebd7401588c282ee868d6a578cc55fe5bbe83f32e"
      "a45b1da7169669275ab8de98ff73a74296359fcfba3a8b0fc1ba5c077096896e30077237"
      "231fb2573b4b564fb2f4eaab2b1c23da8f350ae14bbed8724bf9f0a99634fbfb703448d3"
      "0d0e23df250781c655397703676e87a1c194988c4c986c72fda703fb3a3e6147e35a50e8"
      "2780b7299218452340cf7493fc24fedf145ccdc6db06dd4d5bfd42cb9651b9530109d467"
      "8f110beb595c0d34e894bcd2fed27f0d5bd494852591ad791987f09f1484a4d504052943"
      "5f83c2ca21a450f450eb7641c8555a7ad97932d2bf415190e91ca48a9d2ad7c5f66198b4"
      "0c33f2319aa0f877081061399259a141abe5b8501e3e4597960d65f6e1d3779cbc2f2962"
      "d86cfc3a791b8f7b819fa9eac682de213ed1fe66ed39f7f5be6973d00ee63c9d6d7ebcd4"
      "a418b2ba25eb7790f8a9be0849c254e7123d05f8a95a277ab53929d5b270349b51fdd4b0"
      "0cdb7f90843da55e01544592f637c94a736de566ce1febb0f7ac8996b0b5f8d2681ead3b"
      "9a20705c79793445fc22f993a1e80a5cef83bd897dc24974e9dd779d6f8a532f614a6886"
      "e424eef1be0a2b93fffd2a0224f8efbc65306d275419a42395fed06402da279a57cf7127"
      "b6171548af2ec8c14000eca0ac27c16779a6f75600121b7461f4c9df3c10e33b649f5b4c"
      "b450c9d18fdd199b1817f092f4709e33e3bcdcbb5ab10345568bcb4a7537187e85111007"
      "4a356ac21e4bc7456b5c9039a0a8cc82655a5ad404d0a8aeca1f7c6432972230d605468c"
      "674b86e1ef0e84677733db6d44ade25808d3802ab1114b56e553bc8ae55f75fc61ce203a"
      "d84726396eb136932b66d0c948ee67929c1abfedefe6b7d6799743e0d6971f518315652d"
      "5ef0b780da21fbf4b99d543d1d122ceac03d3e08584d692aedba5250387f71159efcf212"
      "0779c907bb358badcf9f72bf7817cf8f5936c19a5159cdecbabd6a7954dc46ed2dd50b58"
      "264f823b8dbfcb37e5a5fc8d418d9e144fee2a4bd1dec0ad923ab884f229e3aa247011f2"
      "c78f46c15ceacb0cd5b579d425ab9478411df734ed88962b8ba32abd9a4e7e02299736dc"
      "2c4c6303c80bb434a062461eb7b9fa7dced4e993427744488fa96d03cf019230ddafcc0d"
      "32a14c2808eff7dfb87a5088c0dd3a838394b696b33d8b5d9823923a58ad90a24582ea95"
      "485ea91296e22c646448a4baf64ab5b2f831aee378ad5e05444029f3b885160e2790ab96"
      "566f008dc84582921951e0ee071b51a5c1ab2b81d1e30e2b2511ab57d85413895fb33b62"
      "71f742b5d2a14c4ebbbdbcb1f8a7ba0e6efef4d95c1ac0d6e400a96e468e6ab948e73d8d"
      "ae59e30ff39742598d3c64cada3a37d590784368d3db26cdbaa3185f8085a8264b10ccbc"
      "43ec9f6eeb363f006f169f0aa4168acde5599e16b161b885761f0c9dcfd2b33fdbbe86f2"
      "a88bef56fc46dfb075e01c84304ad1ea44aca68032de60acc3875e4b41630674ca07a941"
      "d72ec4c212e885edd4a7ae77910059d344fd0545cdfb4170b2b735d575a56d743fc7e46d"
      "ad742b4dfdd228cd30b2f49e86761f6c57f6d292a040444817b74d341127d5ca13ed730f"
      "c558f182b994a37565f0e05765df6004e7f288a54f311003c965b06dd41de117f54d41a0"
      "482d3276fe3ed5535b228e95c12135b7d190b11c4f56fb77f1136ec0d76acf538abdd7d8"
      "fccfe0d69d23c46942283ec4c996b87e421d8219b3fa486d6d0fd83577b8e58f222d3528"
      "65df77f25515eed1c8bdf7c7bf97eee9ac8e4b7ab35342cab4a96ad1be12e936a048e20c"
      "9c86a3281a32663f1552ce084cecf9cb1bcbfc8cff5a90042be4bf8eeeef16db1ba48f2c"
      "04998b9a4e896405f50d5e1ba8a4c490bcdf07187b90940bc36f973671d349aa772ffe6e"
      "708866f97ea7af8fb582e6031c12c999a48fa092af5b5c18690a2577c58ac5447b20ab04"
      "13c6620b3928ece273a1d107e78b5e2f3a7489e1b871685814048f5dd9f03771ccca4fe9"
      "f8d717687a13c6b6a1444417985815f3a850fb0f6d36a1ae176b49a4a154fdc436821c5d"
      "6b1eca1012c46d5c6458fca872a2ad3e12befaab636abd61bf440b5d3decbfead9ac3ea4"
      "a8f6342fa4dbb663401836c67a126fe16fb35eee5104ee9d4b2da52d52e42d62ad866286"
      "a32305904e1f267f3b0e71696b8eb1892a2bf0bbf0bc86abf6144d12c3f90a110d69766b"
      "14dcc06f8c20b9ef4bfa181390c2a78cc3b72c3687caaa7f3e4135dccb63cd2467d49af7"
      "6784f6ace55bd2bc12bca438a4d99807e91d5d1571742922b099a46ebdea8f161720");

  EXPECT_EQ(
      "0x050000800a27a726b4d0d6c20000000068be190001146b9d49dd8c7835f43a37dc"
      "a0787e3ec9f6605223d5ba7ae0ab9025b73bc03f010000006a47304402203b9fdd43"
      "a88444b0d649079e21d0c8e71176ae23298696dd164bb5551f05182c02206f8321ed"
      "7a738e864f7b79e94310d50714be99e82148f1e4dc5feb39903e57800121022b1959"
      "2aabd4f5cff59b4f842ae09155cb0440019ad1fa80086ab35b3acf565bffffffff00"
      "0000024330112c31d0cbb0db4f77cefc9036ed215ea86ba2aee9a5de08953a821628"
      "2f5136e62a35a7a471c0a68f7a2b09915993244010af0ce69e863a0cdc68042b103d"
      "d62d315080c0bec76397d7c6ad365d84a6354d07049e484d0240410c589389eeb33c"
      "cbb564cc383b01383d4bac03648d6f155fc5d3d280f0f85a3687f17b0e90a1dc6793"
      "503417e53794980e4b8b5e1a18e454da912fa17818988a7b0b1006fedd657b795cc5"
      "84843cb90f9cdf67912f2017dccdb3858439dbea6fadbeeabf0ac892bc6f2c59cc93"
      "44d9f1b781671ae7c9df2f4f69fce6c8215fa7feae4be87a0b5bb08266bead9d4ee0"
      "f80433cd17b713643a5f02ce53392cb015418fc4ff336ea32d7a749448802b7f828d"
      "51d51d78e707d268959c856d9239a6eff5090d1813581a19c24df421a03eea8bd7e9"
      "f8a9cf938c35fe65cf213a1a4b6c727e9401cc519f3ef3f2c936079149bc2af3083d"
      "8a963811c761138e44383c08c34de0d483f62879c4cb80c5cb84124da36abe113d68"
      "eadd017c5c268d6fbdd9c661753fe5375c8043fa3e9f9c94a8e1035bdd2e8eaa3a81"
      "4d0a62657bc5fe689568b5a41ab718342b4790520e1bceb54b987d2590354b54fe69"
      "5ca24c6585bb591cdc2ac395f06a07194812aeceaac0cdbc4178b68be11e30352956"
      "b250378495bfaa4b326d264db71b83b28ef164300dee9a17f187617596c071f63d93"
      "83aeecec6f28e1080013e674b2412d15dd667e2532339caed06f4db6da25e7fcc326"
      "a1a58469d94df082166dbb24a1117898bf7c497466690d13949cd1db05a516622cb2"
      "d746469d04e716acebb04f1c519f00d26dac539a71c2e4c9f1efa0cec80ba0cf37a2"
      "9cd375e0fa70146becfc66ef4cace008fc8a22ae77aa718ce5a149256df6407a24d3"
      "3e5eb1e9d51b2bd2ba4d71b71f3bb54538e984f719bbb2daebec2a54476a4104daa4"
      "7d591fd9db9820f68d48cf52dee3f8772fdd25de4ae436ea1cf125296c30e91262b4"
      "07c80188224c356572acdcca3f41fcefb5e618993b1214a542a06425439ac7c8812b"
      "e6a0bb8ccd9abedd75202451ca522cff1643fda3e399bf60b2b7393a6f89dd8dca6a"
      "1c1ede5965975a62c0088d3a5e68c0b635535c053509c7f55bf9261ecf9b5c2ca045"
      "762df3966fe4c1e97af1c6e3a1ec3eb3f9b57bfdf44069a9d45105bc00a04f80f9e1"
      "5f02b34aa42ca95ec8527ea52d52ce05750a1d67216a3b2f334682f65ce5ea49599b"
      "d7e01f98d14c07095635740c3bf644a7d5eb61c16d28a5bdfdfacb311a29f6b62fe5"
      "0b71864f70937ba483b96ab0c4f6c566ddd1bb55536a33a20a8b4f9642f67e8e1666"
      "2017b947df57320a51cd1141d8e46a40a8c9de24b30c0b08957050b693dc18c5acc1"
      "482e07204cf113ac974dde5d62044ad29910f5a59256ad08352c7141f6b65043a86e"
      "93a3a25fdba8f6f72b60a9e5276da3e20f4e36573ed72aedcb4ae68ac15a57667193"
      "3c1a102d172031fb0d586b1ffb70828ff5a3ff02b38d67153388bfcf5b6d02bb4762"
      "25fc50786083d09fad59b0b97da176b7b4787277d4f90e090b8e6f370b38eada32da"
      "32734553d64b4681c21af7ffe2bcb8d914e43cf24a953a0f94edd975409bf5927de3"
      "8e324ea2e52bf4590b0391dff3ded4354d0bfe2ae3acb4a9302124c73718ca2d2a30"
      "667be950fad78bb9fe49f8ffa40afb40a2ab26ecd17bc9ed16dc0e848550825fd093"
      "c6f1c4eafcb1c7c9859602f7fede5fbe5becf84f28bf74abc880e56b77ed7a724244"
      "8da180f07ffe854a7fa321a2722cdb9c1c46b93186ad15001b32c0a7de4a02a1526b"
      "334d97c2b36f6d0fe83d3f219837c2adc186d6ef1a3f1e0a50d6ca2fc1c2c2a2ea95"
      "373d6a2fd9906f49296f3e8bc141a8c6bbc904722c2541756622d7284e45697c6f0f"
      "1a141b786e80fe882391d7c5919bb98e2fa1719da21c9e4d04a0c30888b0b608d46a"
      "054c32b75bee0396efdeb310e8c5211d41728ab796a44cdb543468e0d655cf64afd6"
      "2e71dd4688eb7e2e196025f1ef0c2cbc99cc08075cb36b99cca1600edd3f6451eedf"
      "f9c3adeb7b843deb83a5582e51ad32881b8ca6064986a41d26d91d0c48adef928d76"
      "6c7f58bd9a31da0096c7bf18a9f8c307f14bc82ae3601b82407069e4dd8a2c4b7290"
      "9e2810435816ed3747e85302c90381e0dee2232d6c21b55310b76fbdaba0aa2deb8a"
      "1c25641604b1bf523281c8f7c5a80a93d967ced9978830b45c2dbf64d9ca8cdc6498"
      "082ae003d50eafee3c930d40d253f4bfe65ec69e7260d3b99dbed28acab956ba79ff"
      "0107d586c878760caa9ac9036079feffffffffffae2935f1dfd8a24aed7c70df7de3"
      "a668eb7a49b1319880dde2bbd9031ae5d82ffd601c6d8bb1d603470a10040bb50333"
      "498933da7cac03eb16937cef9d35d9371046b2d13387711416e9c51d066f32fdfae7"
      "203369a1449da2700c37bfdb099d24a2be7f8abd0833762284617c9150b81e04d405"
      "1e0aaccc7100179b997b8fa7bfdd305d4183f78f8385336c189426bf0ff42bb5c766"
      "a93d2ebb4f3fe89835da45c106c9dc861332d6f3b6705239303cd9b60210a9a26653"
      "2934511322f6ba8eac228a2df4ddddd778bad54694afc2f7d4159a3363361c4b677d"
      "723a7fb56a6f0f2914e7710b0ba8b8d1c209db43bb4911677f1bc1f7fa3faf5d4498"
      "753761c08b5129fe69223b94ebb00548ab19310d28c02a3930c68b0cdfa548106476"
      "c7d98b46b56e441701fa115d58c76a523ee2711d13b2b75e45ac466ed3ca501b690d"
      "89a92047c72cfe3dbbddd9ba784458ac8520e369993b31535f3585b7e0679a5dabbc"
      "ac553e904db5b8f5884679ff1fd49ff4cb9f57ee3fba42e5dfce4de71f26f11c94da"
      "728f9d159ce803c145fd1fa3070d6c6d4318d303d050334374a84eaf66ba2f967223"
      "9e32ad301a542d3d569c94d23a9a9b424069c4a66261fee5ff3c66282f6ed9e89898"
      "f5ca0c0b26ff37f21633bf35cacfc848fc192a7be3641eeadd1d34641912dd53e440"
      "0391bc5341ff004f14dbd35fe96960f86062dfc49092178ab0b05a87867c4656825f"
      "5c818da35d74628ec85d08d398644573789a1018c1a21a4acd1012a10086b00f96d6"
      "083abfd79337c0a3c44dae81cd14c2a4cad615668e0407bd305f77179d26b17bf314"
      "c0437848334eaab3ea92f27ed59ef928b7d03b5bf1b910033eb416772a28e3639998"
      "39175d75bb67ad350df97ad2d20a262e2e8c72e0ee08371d8382645a4237e2327d6c"
      "1ed602a04e0f5658776884234c32b15a98a7cb4310cacbdfddf80cbde428705cd193"
      "a7bd2fcae9b8541f06557348226a5d18f9668b4bd86ef9171305ad9649ad36481766"
      "eb4da18047bb78528fc839226c0f4b13815fd65501a5f43ba7f5cac5249115baa6c7"
      "8d6922a92035497f93377b38483e9902ecb495027105254667ac1039e6abb8a70ad1"
      "70642d79004bb9e5dea33d6caa8faaf6ce0f8edd82b5db64401228e8e590f2af64bf"
      "b58dc7fd07f449e6a57fd1245c5f3b21cde2e32d73f28497654d53fe4a9fc6a9d9c5"
      "e07f1f8b1cec16aa279ae4a89b88a58f04854b5c29fe6ddfce9906f8fbdb3454fdcb"
      "a69b599935523b8dbffa48e0ebbb6c7e49f8b6cde4991ae5ae8e73ad4e3c88d1be12"
      "dae46e9c23a50ce7ae0b9e07c1a9efc075dd2d7e2a215042b19ddc8c28f41a204156"
      "20825bccfb9cfa07e1280b646d591a863fe6b7b007a8b946e51062a3370aed325d2e"
      "7dda1ae4bf5c412c41b2aada6f583af999aa55e63ab52672218c1efa4197e73d1e1e"
      "af8cb9a0f0f790db781952ecc670a0bf0cac8285ec02133c206d186cafd560b6de56"
      "3c813d57993db2e57733999c09e1171dc6f9c3fa3a4da170b4e8895587f389d24b39"
      "5b5eeca1d8bb9445e022624cfbad4cf20b115c549e8e6235bc3ebeb07c30275dedb5"
      "fa2912f948b372c4a58c51afe63f62eb90e73d7f51e8db044719959b769753725c1e"
      "67975c8ccd51a5480c8ef45dd5aff6bda0038ac67132a374fa6d6b75e0a4941a9e36"
      "2fe6f5f2dc72f8066ca9489c81dc2fecc7c3c14dc19c8f89c7cf38dcb0f89c95819c"
      "022f7765de3eb69c28c29d2b2a428018cc80a0c8ef8698ed8d256819f8dcac8b2b5c"
      "c499fc10dec93deef6fca8c2d80ae283f9668db5329eb2cd1c9cc92ec779cf0014c8"
      "bffc5b4f283ba484369f057e0c075d5276dc6731186889941fbe199076c4580dca0b"
      "dafaacd7c6c13494d42c948abeede8dad58dce340e041296befb48b3a2738f0948cf"
      "c1e556043283154a9779c3fff7afb164b44af1e13738ee3544cb8c34530e9408278e"
      "eac1249f7491983b21b09e85536cc62132105f98030f4b64827db9e9e4e2184d8010"
      "0ef264e5e22708fd7c7e8cb3333fd64f8296076c3c870c6e5e7d836b6c9ae1b8a1bc"
      "8f3a712ea0431273a24c245a449e9c8e441343029afec9edeb101c1d8c9dbffabc1d"
      "17c6818d4d4f7b96217bc5c7aeb843ae250aa8f946dade5df1e9e6882695ef58e439"
      "95b8f68b6fec10c01b0fc4a37d94e9c3880effda57ccdf9d72e5a1206288db088126"
      "39cf3a3b3bfa76646af70b7927e49cff1977a4368021c0f604b836e9b876f6b9140f"
      "5045e88f268bf798a72b04b8df18a4aae9be79c27d483eb39f7632a18a351d87870f"
      "a1c5105af75dff136f44e5edfa6e77e280330c51a8e841a8a3581b60794cfbe6ca39"
      "98373ea4a2a589d6346a9b0988ba57a852368d762ef9c4c3de464f971212a4ec6fd7"
      "dfa9a525d0be17f43b97d3240b09b33c3e8cc5ae2dbc2a736af44d55d3acb18c226d"
      "292157d6a59243483efb6547464208004e07dbd0d04f4487d1fdeda1a3df63a31239"
      "beadf1bb0368a4677d5147ef2d18a9a91cf0105dccbf56f04516bca3fee8194c9b76"
      "ae71ae2f2f2e2cfc58c70dbf6054c721b0c5b08551b0387c3e82ca272128c70ccd00"
      "57ed4eba9f9d08663753d57d8c5ce309799361f8009ede59963f4657b2ff7c95b997"
      "e577f6242ebe1c137c1c3f03b4eba314097927e7fb98fc9ba76c141bbffa705526a5"
      "11e75ca52d08c1bd88908df10d307adf603119db56217c2db92d47643d972c6e5c43"
      "1d5d32c0783e464bc058a2e3207fec16f0a0cfe44d5586260b82a625b383710a37fa"
      "33f662733dd364e49137712cd57223b76773cfcfdf845b7659ccdada5211c2f12889"
      "a4465caf3f138d7a90b2d202c0451aef799175be5f3af781272e730985c62c30e445"
      "0058013e0b2ca758c0b27e9213788cf187a1e4ec8ad456bdf9a0308305f98f304560"
      "216d3a95f078291e66cb9e8f6276cb758b4940a997b6063d41372cbc9bc67edc5651"
      "0d572eb0bf7d35226dc5795fc6c952cd6799212ac527907e18fe78da43121faa1e3e"
      "f77d39bb201a5fd7796c8bc04b0dfd6a84d9d12cda260b183bd0fde7095c1a78ca0f"
      "3de835f91c1b075356bb044f61f90ecaae69b9293fd8560912245cf7914230706f61"
      "1dfa4c47a19f2b2472922fddc8667b9cdc4f212c4e1e63d8308ef9558e248141e400"
      "6be8b860a049435ad183427619c0a6fa15e81b8a16a438f061a178381791e4026127"
      "c378a1299ab51937bea30f99ccd63be77805e274e597571f7ed0dc534eb700605ea3"
      "b2a5f6d31e4f228c6fcd56fc2abfb234dccf80351ec3ad9024d844bbc3b22b476fc8"
      "46465c900e9ac021594e0c7e46cd1ad81a6b6aa4dd5921adad1f15b87b2b0b0236ab"
      "0356dd7e127c854d11fcebdf43cce59acd70dc405a98ad51dd6cc2858baaa81eb5a5"
      "6abab22799c30c152627af052ef94c1d80f28afe0aa149816807baf5bd1c85da9d40"
      "d2336ee0287461a026266af7cfaa64b952a5eeea67117db0243c33747a68e60e07fa"
      "cbaa01d6e4e7f992493b594b2b0d839705e8942207b2983a9ac808473442a9fb47b3"
      "292cae218986071254c4065eb41c28fd4e68c0c55b36984ebe755d712e5014672b88"
      "b3efd71c9a70b1b2af8da4c5484f0708aa9eb7d64f42dc08bf5cbddb2d1e38831d7a"
      "511ec2f4026caf2e443a86f2f9afdaa21b681d80088e98500f96f0bd31ad386015b8"
      "5dceb49180ed9360b862b3dacc3613c83b5bdf636c55c7a2e5a22a9a94ac085197b7"
      "41b650f262a6ca4a02db4ee1bf04387b1cfe838a2013f66a026b101842fa4e44965a"
      "80bcc9ff0569503a39fe35e7469a90541674a04440e70353ac24a7d2d98862da84c3"
      "4fb7b79983e209f29e4ad6684deab378079333f42c1513a986063d8243d453148c42"
      "f10f88fdf4610fada491b781b7a1d9d890ea1883862bd620399b1611d543613d2202"
      "fe86fff14c155568461498d0780e3ebf1fe2c0f2f857f6cfa2ac4ca1088cc7051463"
      "dd4fc6c1c297b627c54af3ea97250af20e585fa06fe2212f751f397073e8ab67630a"
      "75bb5c4d6f2f343b4cf0ba851c16fb904e5a1fe66f0b2e8bac618fd2cc490c83216e"
      "fe87f759705feef7e1df3c0ca649f641c1488aa2fdfc4159fc34c0d61b06c9d5ad18"
      "16bf7210361497d73392ab294d3e6214deb3e6429037ef38add1d172b9157ddc7d07"
      "8f5578704da7292a03cffff71145c5124a8fe9a46a7da10c071196d1f9175c6e5358"
      "5756980124ed7744922acf08378ad0776d4e82df020b14d1c2384ece2091373a5a0c"
      "8b7301391a4c73066178a1515285b2e4f1482858ea91c59af49bc5e3d268f5140ae0"
      "2d78edfc715fb5052b8fa99d2fb2287832231ed38d98432f5afdd957a06ad11b0a58"
      "da42228f3106508cb04702ce79ffe526a5f5dee1e9d026ced1ee06eb921109120034"
      "556432b89f6565d10f4b9cf7248faeddbd94a317da93ab8d7c93524d0fa2d270398a"
      "cd02d36a43f6e8e460b4fbec62194b73469be9066968cbc10bea2acb1b98369a3842"
      "975b940fbc875476c1fa7400ee95733ac6eeddf8353e70d60c164d4b229bc361d7e3"
      "8a60453445190caf1dd3684c491efa3e0abee650c21c37734b6f8cb2ef851e7e8d98"
      "aa5d2399f1768a2cc81020b59371d16510bc1e38374922ef26159f629fd7bd7bc40b"
      "3ddb4ea2a1c7bc4da6504f83beca6e5163a12f72b26ed6fd219e5edb7d316f712311"
      "f57ee16ebdd2153e981a4890ab63440208b3dbbcf7f886ea217cb82608a1f6065592"
      "68d4f0d1ab16f4bc0dcef971d8d3218bfccab87f704de90c418ebbda7f088f4a5213"
      "0d0fd12bcbbbe6465b96ad8e111042560de47aad79bd1473c882bad2fce71b4c78a3"
      "b99e4f9b405e84fddd6815c6c4059cfb4b896880fb01e01fee33c9113a075c110782"
      "4b342545f984d90d0cd29e951dd482ec068a6c4fbc02f86ba5ca1bda0d847972287e"
      "6715cf5975d135c14bc8a9703b1aa6294c44a50979d0d789fa997a41f00899002a0f"
      "2928b6bf2b57dda37ea5d38d06e12734b88f566e5ac6d9be1f48afa815d7f2f0841d"
      "3177105b348379525fcc9a6a5fd14953ca3947ca43cde56eabf0be2867679a632c29"
      "1b8354fa0e4a0f9ba5ba2f2122c4e31d2001c0ea858b466dcc3c1c45cf8b19392073"
      "a45ed261f64878021d18f43520238d0205dee2cd3e6ef878d407f5f5cb3832204f4b"
      "80ee92174bf3f470c6e1800499d8d16f03c0d2946c460127a873dc46021ceeb84c34"
      "d136efdfdce8283daf3aeab90aa4f1256d5509838c96349cb88f1b817c94177e8927"
      "1f3c464b7dab45868a80f52740ec504334b87b1ffe7315d13e76ea8379873b89c652"
      "e7ac0984475231e71c67144a0c59924f96dc800d09b00a3a27f86f62f79317f7fc43"
      "9a5e235fce80908a60256543b5f3564a8205500b26e4a7d35bf60251ad8d52c8d44a"
      "6e3d0b111aaccbd813f1aa042b554b911bd3385e26be1af29cd0d3afa204e922a2ee"
      "039875df3fdb5c65636939a186304f9e3095bb82cba060b043e138f36ff275e00ca2"
      "7ec7a57a91853f19ee520d9a3a9e1ab97d57683e6091229180b6167d566982ef0865"
      "a70e487e27116d7ca227e7500765d4ca712cffd73dea70f138714221f308548c2fe9"
      "697a5a8204cb00dab455223b21dbda4ace383b4c2d5cb8a80b43a1cf80a85bcdbae4"
      "43e7cc65931b7d9c395a09d85aa04a0b896a1ababc3de81b258f22af403b0e45923c"
      "8ef7d2f68aa224aeb1261cfa4f645649c5f9597037eebfc066e517495d983e1f0461"
      "c651b4e425aca81ac92f7d624688b615d61b0dc20bacf56710803c549ebf9a558f65"
      "2ba4052026c5725e0f17a016eee0e070c1a857b8bd6d920397072b5cda654653a4ae"
      "18167007127ede81aea309b7a226293309449c3df4bb06bcfeab20b9fd0f7a682a55"
      "7269bfb1030c7d87fa7337fec56a7126e553a65869b94b8d41925d9fe4ca15de3b91"
      "6d497206bf80fbd657658c3cd59efe2d3e16f9886d48eb21ac931e9831fef5e1d8b0"
      "c28b0935378da0ab01455fb9e7e0305b638995f6dc7c29cde4293f871fd0318419a5"
      "fd289a1dbc78bff788fe84c5d85b0eabde25192665f1a9611dafc364dd2c76548e56"
      "c0c09f33ddcddacdb858e689ef60bc9024069c836cd9231a25c204a450c7a141ca2b"
      "6283b46f39aab490f368d5ecd8dbc2856ef94d48106550406fce98f9cac770369486"
      "9d9e9d9d56adadd6e5d7938503fed3e7f11f13ec55c87f9582e6b6b803f4359e4f1b"
      "1f0fd64137140da024169208957ec32301644b68fea7730fe73b03be378ab66d6ba0"
      "d9a95f44572e577ce0af4d5b169d0f5262e70a8056d0e9c9aa8c7e199b0853126de5"
      "75fcba6c7157789f46cd81870369218875e27ed132b80688035798300e01d621ce19"
      "a80a712a152d04eb4a1329eef9c2a76b3d8ef87aa5a4e5a0df0b5d86b5f6cf2a30a1"
      "4e0eea06fc7bbd9f31bb761e527b1a302a709e048db0829bd834cfc38ff0d8656eaf"
      "35dde2948b9001848b61ca80842be9eb2d821f4968e4a5ba17c407d0614e305b4867"
      "eb10f6fe0fde52c620f171ee0e8431d95c2fe6ebea9dd3edef4e38e1c06a447f238a"
      "aeb00aeb3a2deeaf11def8c70306e30f361000bbab84bd2b8d00f7489e11cef6ccf2"
      "2b2f73f5df1bd8ceaa96c72c2f73479ef034457bd48f180f8c25885f7317578934ad"
      "b4d88d0b9e0716f011c7ca6bdba21d9a2287bba225bdac9ff40456ecfe0c110dd2a1"
      "d02ed119a5219b5532def6e59da498dd52fd0450fc239de044a71e773c66435a1abb"
      "5a717ddb44bf0bfe38e606f3eebf545a6932428ac4e03891d6d124f97eb62b06e3ba"
      "4dd1c881a96de13e30a1dd4f609568cf3509c4aeb66df6ba0aa368ba6840f117e499"
      "d1a2db7f300623ed3fadde6513cd414a8767a153f8e514260e5fddc12f74cd7b2abe"
      "5a8a12268d3e0f0ce4aeb1cb02619795b7a9564f04bf7a66751f45b8c55581fdbb1d"
      "ef6bec2bc591361e6c5f655c154bebe62bda03d0cc13781351a63834817768044125"
      "204b2ccbe4a34ad8787913d19c51c5fe119692ad88701e41f410d2e551b4b4485cc6"
      "104ff8f3a96e60d441b981914b6f3a6e0a103874f5c8b5279c822380e1388f8c4e05"
      "1806df43d088919b6a96651a1fd18978d9f7b7303d81086fc802e80dbacddca3c67f"
      "75c31f9296a6a15671b828ffa2144d7d28aff2ef13981c08f29ceeeae61a4abf670a"
      "177bb66ee9eab7ba051fcea86c57279c74792c0db9d7be07b568a29be798e5cabe3f"
      "5922cb9bfe291d5cc733bedd48be1e0b42580ce82b2d192bc91cf834eed96e264049"
      "3f452eb90eebfedecbb6eef637fdfb36c58fbbf19f34d15c965e67f1bd19f8a86954"
      "190813ec1f32f9858ad282f3c3bb9e6599a60ff5a450bbee8a0fda246f1d98ec49f7"
      "1d3dddd8a3b894123279820f5e5eab5ee35be19ccdba7484205dac689e5af43f30de"
      "1eea0415c1d5844265ceb119755aab460fbb69450d1385e0917389dd2a8a254c9e11"
      "779fd80912f282c97cda530d1802588e37c57ebbc0fdd6b34afcd00008e3f2860c02"
      "9e87bd0c9cf9b7c875c78138e181c6b82180f1c2a324157dc1821a868d1fe05337df"
      "91051b0a712662f8174fd1363548e629e1a7ac2468ef99a4249640812f0cc2793c33"
      "0e7ae5d8e6f58ba598a75d378dd30fc43825e549382f0998cc63a91bd59562fc0194"
      "63e2ee0288ce8b972d02bfd6931c71e96f1438941f73a9fb0645e21bd830afa898cf"
      "adeae7092dc212ba3af1bfede863ac1868072674b962c28f9ffcc7c0b41eb3ce3f9b"
      "233bfeb7c73c1af34eb770f211e36d213d1327c5e115ec13741598df6924ecd5c49d"
      "92198e0690e5853163868077885525e2149476914ecd2da886cca521d9d4326fd076"
      "7d4760e02ebd300cd94f158439b3bce3e80daada688d0d82201e03704a0b7e817b82"
      "2ff42671ab1178c0fbef25872f62408a9918696ebfaa016ac83ab0ae6d7e564e4dab"
      "9cdf72c2be75e11a2e3bd421fab646c18bffe2b414db0ae47a2178b1fedb66099427"
      "7df443daba0335987650196c1459fec34300902fa7e093a319815ae9ca77a05e36cf"
      "c6ca1e921d38e5ce1d435f0780add12a7866d0e09039ee77696fa71b98b2ddf7cda6"
      "08aa3d711b36c0d7b7f00562173c3b1fff6d4b8bda89b59946a0702a1ec432d3369e"
      "03707faa4c8e40725e3a93b1b3250990aba031b44511c2427f69e82b75f5ebdf3936"
      "5ef9785786c9db0219b79f2520478d52490d3cd7f8c369f82ebd7401588c282ee868"
      "d6a578cc55fe5bbe83f32ea45b1da7169669275ab8de98ff73a74296359fcfba3a8b"
      "0fc1ba5c077096896e30077237231fb2573b4b564fb2f4eaab2b1c23da8f350ae14b"
      "bed8724bf9f0a99634fbfb703448d30d0e23df250781c655397703676e87a1c19498"
      "8c4c986c72fda703fb3a3e6147e35a50e82780b7299218452340cf7493fc24fedf14"
      "5ccdc6db06dd4d5bfd42cb9651b9530109d4678f110beb595c0d34e894bcd2fed27f"
      "0d5bd494852591ad791987f09f1484a4d5040529435f83c2ca21a450f450eb7641c8"
      "555a7ad97932d2bf415190e91ca48a9d2ad7c5f66198b40c33f2319aa0f877081061"
      "399259a141abe5b8501e3e4597960d65f6e1d3779cbc2f2962d86cfc3a791b8f7b81"
      "9fa9eac682de213ed1fe66ed39f7f5be6973d00ee63c9d6d7ebcd4a418b2ba25eb77"
      "90f8a9be0849c254e7123d05f8a95a277ab53929d5b270349b51fdd4b00cdb7f9084"
      "3da55e01544592f637c94a736de566ce1febb0f7ac8996b0b5f8d2681ead3b9a2070"
      "5c79793445fc22f993a1e80a5cef83bd897dc24974e9dd779d6f8a532f614a6886e4"
      "24eef1be0a2b93fffd2a0224f8efbc65306d275419a42395fed06402da279a57cf71"
      "27b6171548af2ec8c14000eca0ac27c16779a6f75600121b7461f4c9df3c10e33b64"
      "9f5b4cb450c9d18fdd199b1817f092f4709e33e3bcdcbb5ab10345568bcb4a753718"
      "7e851110074a356ac21e4bc7456b5c9039a0a8cc82655a5ad404d0a8aeca1f7c6432"
      "972230d605468c674b86e1ef0e84677733db6d44ade25808d3802ab1114b56e553bc"
      "8ae55f75fc61ce203ad84726396eb136932b66d0c948ee67929c1abfedefe6b7d679"
      "9743e0d6971f518315652d5ef0b780da21fbf4b99d543d1d122ceac03d3e08584d69"
      "2aedba5250387f71159efcf2120779c907bb358badcf9f72bf7817cf8f5936c19a51"
      "59cdecbabd6a7954dc46ed2dd50b58264f823b8dbfcb37e5a5fc8d418d9e144fee2a"
      "4bd1dec0ad923ab884f229e3aa247011f2c78f46c15ceacb0cd5b579d425ab947841"
      "1df734ed88962b8ba32abd9a4e7e02299736dc2c4c6303c80bb434a062461eb7b9fa"
      "7dced4e993427744488fa96d03cf019230ddafcc0d32a14c2808eff7dfb87a5088c0"
      "dd3a838394b696b33d8b5d9823923a58ad90a24582ea95485ea91296e22c646448a4"
      "baf64ab5b2f831aee378ad5e05444029f3b885160e2790ab96566f008dc845829219"
      "51e0ee071b51a5c1ab2b81d1e30e2b2511ab57d85413895fb33b6271f742b5d2a14c"
      "4ebbbdbcb1f8a7ba0e6efef4d95c1ac0d6e400a96e468e6ab948e73d8dae59e30ff3"
      "9742598d3c64cada3a37d590784368d3db26cdbaa3185f8085a8264b10ccbc43ec9f"
      "6eeb363f006f169f0aa4168acde5599e16b161b885761f0c9dcfd2b33fdbbe86f2a8"
      "8bef56fc46dfb075e01c84304ad1ea44aca68032de60acc3875e4b41630674ca07a9"
      "41d72ec4c212e885edd4a7ae77910059d344fd0545cdfb4170b2b735d575a56d743f"
      "c7e46dad742b4dfdd228cd30b2f49e86761f6c57f6d292a040444817b74d341127d5"
      "ca13ed730fc558f182b994a37565f0e05765df6004e7f288a54f311003c965b06dd4"
      "1de117f54d41a0482d3276fe3ed5535b228e95c12135b7d190b11c4f56fb77f1136e"
      "c0d76acf538abdd7d8fccfe0d69d23c46942283ec4c996b87e421d8219b3fa486d6d"
      "0fd83577b8e58f222d352865df77f25515eed1c8bdf7c7bf97eee9ac8e4b7ab35342"
      "cab4a96ad1be12e936a048e20c9c86a3281a32663f1552ce084cecf9cb1bcbfc8cff"
      "5a90042be4bf8eeeef16db1ba48f2c04998b9a4e896405f50d5e1ba8a4c490bcdf07"
      "187b90940bc36f973671d349aa772ffe6e708866f97ea7af8fb582e6031c12c999a4"
      "8fa092af5b5c18690a2577c58ac5447b20ab0413c6620b3928ece273a1d107e78b5e"
      "2f3a7489e1b871685814048f5dd9f03771ccca4fe9f8d717687a13c6b6a144441798"
      "5815f3a850fb0f6d36a1ae176b49a4a154fdc436821c5d6b1eca1012c46d5c6458fc"
      "a872a2ad3e12befaab636abd61bf440b5d3decbfead9ac3ea4a8f6342fa4dbb66340"
      "1836c67a126fe16fb35eee5104ee9d4b2da52d52e42d62ad866286a32305904e1f26"
      "7f3b0e71696b8eb1892a2bf0bbf0bc86abf6144d12c3f90a110d69766b14dcc06f8c"
      "20b9ef4bfa181390c2a78cc3b72c3687caaa7f3e4135dccb63cd2467d49af76784f6"
      "ace55bd2bc12bca438a4d99807e91d5d1571742922b099a46ebdea8f161720",
      ToHex(ZCashSerializer::SerializeRawTransaction(tx)));
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/command_line.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "brave/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

// IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST is excluded from Android build to
// save space. Ensure this test is not build on Android. If it will be required
// to run these tests on Android, include again
// IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST
static_assert(!BUILDFLAG(IS_ANDROID));

namespace brave_wallet {

namespace {

static base::NoDestructor<std::string> g_provider_solana_web3_script("");

constexpr char kFirstAccount[] = "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu";
constexpr char kSecondAccount[] =
    "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu";

// First byte = 0 is the length of signatures.
// Rest bytes are from the serialized message below.
// SolanaInstruction instruction(
//     mojom::kSolanaSystemProgramId,
//     {SolanaAccountMeta(kFirstAccount, true, true),
//      SolanaAccountMeta(kFirstAccount, false, true)},
//     {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
// SolanaMessage("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0,
//               kFirstAccount, {instruction});
constexpr char kUnsignedTxArrayStr[] =
    "0,1,0,1,2,108,100,57,137,161,117,30,158,157,136,81,70,62,51,111,138,48,"
    "102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,255,"
    "176,67,136,209,219,42,6,169,240,137,142,185,169,6,17,87,123,6,42,55,162,"
    "64,120,91,1,1,2,0,0,12,2,0,0,0,128,150,152,0,0,0,0,0";
constexpr char kEncodedUnsignedTxArrayStr[] =
    "QwE1cGTGKt9G9zyqwBzqPp711HGUJH15frCMMWp9ooU4DjCQpVSrFxeGxfqnMmw91nWkdFY42H"
    "Wnqqyw2fXYk4kspucEtan8vrRAUPRAr2ansqm52VZe8ocdZudeoXJqHYNPjuPYBawsYFKms5Wu"
    "NBVNFy4UTURd";

// Result of the above transaction signed by kFirstAccount.
constexpr char kSignedTxArrayStr[] =
    "1,39,140,195,60,14,241,115,164,59,230,251,59,231,246,11,104,246,137,211,"
    "101,131,22,147,172,87,182,89,213,67,79,6,233,245,66,112,55,246,89,97,111,"
    "7,99,99,42,32,15,205,69,189,151,25,172,15,166,171,238,153,135,118,192,135,"
    "93,168,10,1,0,1,2,108,100,57,137,161,117,30,158,157,136,81,70,62,51,111,"
    "138,48,102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,"
    "255,176,67,136,209,219,42,6,169,240,137,142,185,169,6,17,87,123,6,42,55,"
    "162,64,120,91,1,1,2,0,0,12,2,0,0,0,128,150,152,0,0,0,0,0";

// Base58 encoded signature of the above transaction.
constexpr char kEncodedSignature[] =
    "ns1aBL6AowxpiPzQL3ZeBK1RpCSLq1VfhqNw9KFSsytayARYdYrqrmbmhaizUTTkT4SXEnjnbV"
    "mPBrie3o9yuyB";

// First byte = 0 is the length of signatures.
// Rest bytes are from the serialized message below.
// SolanaInstruction instruction(
//     mojom::kSolanaSystemProgramId,
//     {SolanaAccountMeta(kFirstAccount, true, true),
//      SolanaAccountMeta(kSecondAccount, true, true)},
//     {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
// SolanaMessage("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0,
//               kFirstAccount, {instruction});
constexpr char kUnsignedTxArrayStr2[] =
    "0,2,0,1,3,108,100,57,137,161,117,30,158,157,136,81,70,62,51,111,138,48,"
    "102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,178,214,107,40,5,90,152,"
    "248,228,193,154,254,176,38,253,12,47,103,85,191,24,48,173,84,45,134,214,"
    "75,36,218,67,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,131,191,83,201,108,193,222,255,176,67,136,209,219,42,6,169,240,"
    "137,142,185,169,6,17,87,123,6,42,55,162,64,120,91,1,2,2,0,1,12,2,0,0,0,"
    "128,150,152,0,0,0,0,0";

// Result of the above transaction signed by kFirstAccount, with a second
// signature provided via tx.addSignature before calling APIs.
constexpr char kSignedTxArrayStr2[] =
    "2,221,78,151,240,171,39,49,7,200,99,111,63,70,103,35,101,246,89,202,209,"
    "188,206,90,156,43,178,250,220,181,255,34,188,67,52,209,40,243,166,212,225,"
    "120,226,216,254,3,83,80,173,38,191,91,160,51,211,23,3,49,77,220,62,58,179,"
    "207,8,31,184,95,37,231,236,178,185,129,150,198,140,61,252,203,157,22,107,"
    "9,191,165,232,35,10,178,246,230,39,243,235,134,74,27,43,195,122,143,80,"
    "180,51,217,107,224,154,137,48,99,56,163,98,175,238,202,28,149,75,73,167,"
    "209,91,209,125,53,15,2,0,1,3,108,100,57,137,161,117,30,158,157,136,81,70,"
    "62,51,111,138,48,102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,178,214,"
    "107,40,5,90,152,248,228,193,154,254,176,38,253,12,47,103,85,191,24,48,173,"
    "84,45,134,214,75,36,218,67,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,255,176,67,136,209,219,"
    "42,6,169,240,137,142,185,169,6,17,87,123,6,42,55,162,64,120,91,1,2,2,0,1,"
    "12,2,0,0,0,128,150,152,0,0,0,0,0";

// Encoded serialized message passed by web3.Transaction
constexpr char kEncodedSerializedMessage[] =
    "FDmjJVJ5XUQPik2xqs7NqP7VdMkDXNWLimqTR8C2KstRHZAdRUoCMQr7LXUjQ6dSer9jfWWfbN"
    "XzMToAWzoQLWvgduNCLxSVWVuiVZzqGPwC8mWT4SAu5NDCC5VTWcSNWj4Q9HSvgQitodttQiQR"
    "3yQvRZJurNzub3SBK3umEqULkVJPYZJRCmPbXQm9ebPEXGYQRKrjiAt7";

constexpr char kSecondAccountSignatureArray[] =
    "31,184,95,37,231,236,178,185,129,150,198,140,61,252,203,157,22,107,9,191,"
    "165,232,35,10,178,246,230,39,243,235,134,74,27,43,195,122,143,80,180,51,"
    "217,107,224,154,137,48,99,56,163,98,175,238,202,28,149,75,73,167,209,91,"
    "209,125,53,15";

constexpr uint8_t kSecondAccountSignature[] = {
    31,  184, 95,  37,  231, 236, 178, 185, 129, 150, 198, 140, 61,
    252, 203, 157, 22,  107, 9,   191, 165, 232, 35,  10,  178, 246,
    230, 39,  243, 235, 134, 74,  27,  43,  195, 122, 143, 80,  180,
    51,  217, 107, 224, 154, 137, 48,  99,  56,  163, 98,  175, 238,
    202, 28,  149, 75,  73,  167, 209, 91,  209, 125, 53,  15};

// First byte = 0 is the length of signatures.
// Rest bytes are from the serialized message below.
// SolanaInstruction instruction1(
//     mojom::kSolanaSystemProgramId,
//     {SolanaAccountMeta(kSecondAccount, true, true),
//     SolanaAccountMeta(kFirstAccount, false, true)},
//     {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
// SolanaInstruction instruction2(
//     mojom::kSolanaSystemProgramId,
//     {SolanaAccountMeta(kFirstAccount, true, true),
//     SolanaAccountMeta(kSecondAccount, false, true)},
//     {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
// SolanaMessage msg("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0,
//                   kSecondAccount, {instruction1, instruction2});
constexpr char kUnsignedTxArrayStr3[] =
    "0,2,0,1,3,178,214,107,40,5,90,152,248,228,193,154,254,176,38,253,12,47,"
    "103,85,191,24,48,173,84,45,134,214,75,36,218,67,194,108,100,57,137,161,"
    "117,30,158,157,136,81,70,62,51,111,138,48,102,91,148,103,82,143,30,248,0,"
    "4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,131,191,83,201,108,193,222,255,176,67,136,209,219,42,6,169,240,"
    "137,142,185,169,6,17,87,123,6,42,55,162,64,120,91,2,2,2,0,1,12,2,0,0,0,"
    "128,150,152,0,0,0,0,0,2,2,1,0,12,2,0,0,0,128,150,152,0,0,0,0,0";

// Signature of kUnsignedTxArrayStr3 signed by kSecondAccount.
constexpr char kSecondAccountSignatureArray2[] =
    "146,134,13,206,131,77,84,101,99,222,35,128,221,21,100,139,108,149,44,61,"
    "214,79,184,146,38,197,52,202,72,37,110,188,4,149,164,116,43,93,49,165,106,"
    "101,195,194,60,223,166,173,127,102,129,19,13,67,20,197,213,167,131,107,"
    "132,109,25,7";

// Result of the above transaction signed by kFirstAccount, with the first
// signature signed by kSecondAccount provided via tx.addSignature before
// calling APIs.
constexpr char kSignedTxArrayStr3[] =
    "2,146,134,13,206,131,77,84,101,99,222,35,128,221,21,100,139,108,149,44,61,"
    "214,79,184,146,38,197,52,202,72,37,110,188,4,149,164,116,43,93,49,165,106,"
    "101,195,194,60,223,166,173,127,102,129,19,13,67,20,197,213,167,131,107,"
    "132,109,25,7,64,80,2,141,31,0,136,77,254,119,123,202,223,237,54,78,90,33,"
    "83,40,134,168,122,131,27,159,219,82,66,35,195,239,14,17,2,110,95,103,216,"
    "30,207,59,87,230,134,153,195,56,245,125,171,2,194,244,233,61,15,58,239,17,"
    "221,71,171,12,2,0,1,3,178,214,107,40,5,90,152,248,228,193,154,254,176,38,"
    "253,12,47,103,85,191,24,48,173,84,45,134,214,75,36,218,67,194,108,100,57,"
    "137,161,117,30,158,157,136,81,70,62,51,111,138,48,102,91,148,103,82,143,"
    "30,248,0,4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,255,176,67,136,209,219,42,6,"
    "169,240,137,142,185,169,6,17,87,123,6,42,55,162,64,120,91,2,2,2,0,1,12,2,"
    "0,0,0,128,150,152,0,0,0,0,0,2,2,1,0,12,2,0,0,0,128,150,152,0,0,0,0,0";

// First byte is the number of required signature (1) and next 64 bytes are 0
// for an empty signature, this empty signature is provided otherwise the
// VersionedTransaction.deserialize() would fail.
// The rest bytes are serialized message created by logging
// transactionV0.message.serialize().join() from
// createTransferTransactionV0WithLookupTable in
// https://codesandbox.io/s/github/darkdh/solana-provider-test with fromPubkey
// and payerKey are changed to kFirstAccount and blockhash is set to
// 9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6.
constexpr char kUnsignedTxArrayStrV0[] =
    "1,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
    "128,1,0,1,2,108,100,57,137,161,117,30,158,157,136,81,70,62,51,111,138,48,"
    "102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,255,"
    "176,67,136,209,219,42,6,169,240,137,142,185,169,6,17,87,123,6,42,55,162,"
    "64,120,91,1,1,2,0,2,12,2,0,0,0,100,0,0,0,0,0,0,0,1,166,204,157,235,66,94,"
    "62,117,14,95,59,44,129,146,205,25,85,231,59,33,111,45,217,138,53,56,53,66,"
    "159,240,201,66,1,1,0";

// Result of the above transaction signed by kFirstAccount.
constexpr char kSignedTxArrayStrV0[] =
    "1,"
    "30,195,246,190,104,175,221,149,31,33,245,39,142,118,12,194,208,132,134,"
    "99,43,107,51,140,235,64,70,231,68,39,130,20,93,177,57,225,108,178,184,32,"
    "212,202,255,141,11,165,5,164,252,112,70,104,68,212,21,180,107,227,232,59,"
    "80,3,190,10,"
    "128,1,0,1,2,108,100,57,137,161,117,30,158,157,136,81,70,62,51,111,138,48,"
    "102,91,148,103,82,143,30,248,0,4,91,18,170,94,82,0,0,0,0,0,0,0,0,0,0,0,0,"
    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,131,191,83,201,108,193,222,255,"
    "176,67,136,209,219,42,6,169,240,137,142,185,169,6,17,87,123,6,42,55,162,"
    "64,120,91,1,1,2,0,2,12,2,0,0,0,100,0,0,0,0,0,0,0,1,166,204,157,235,66,94,"
    "62,117,14,95,59,44,129,146,205,25,85,231,59,33,111,45,217,138,53,56,53,66,"
    "159,240,201,66,1,1,0";

// signMessage
constexpr char kMessage[] = "bravey baby!";
constexpr char kEncodedMessage[] = "98,114,97,118,121,32,98,97,98,121,33";
constexpr char kExpectedSignature[] =
    "98,100,65,130,165,105,247,254,176,58,137,184,149,50,202,4,239,34,179,15,"
    "99,184,125,255,9,227,4,118,70,108,153,191,78,251,150,104,239,24,191,139,"
    "242,54,150,144,96,249,42,106,199,171,222,72,108,190,206,193,130,47,125,"
    "239,173,127,238,11";
constexpr char kExpectedEncodedSignature[] =
    "5KVxa2RGmhE2Ldfctr42MQCrAQrT2NTFcoCD65KUNRYki6CPctUzsnR2xm4sPLzeQvSdCS6Gib"
    "4ScfYJUJQrNE4C";

class TestTxServiceObserver : public mojom::TxServiceObserver {
 public:
  TestTxServiceObserver() = default;

  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {
    new_unapproved_txs_.push_back(std::move(tx));
    if (!run_loop_new_unapproved_) {
      return;
    }
    run_loop_new_unapproved_->Quit();
  }

  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx) override {}

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx) override {
    if (tx->tx_status == mojom::TransactionStatus::Rejected) {
      rejected_txs_.push_back(std::move(tx));
      if (run_loop_rejected_) {
        run_loop_rejected_->Quit();
      }
    }
  }

  void OnTxServiceReset() override {}

  void WaitForNewUnapprovedTx() {
    if (!new_unapproved_txs_.empty()) {
      return;
    }
    run_loop_new_unapproved_ = std::make_unique<base::RunLoop>();
    run_loop_new_unapproved_->Run();
  }

  void WaitForRejectedStatus() {
    if (!rejected_txs_.empty()) {
      return;
    }
    run_loop_rejected_ = std::make_unique<base::RunLoop>();
    run_loop_rejected_->Run();
  }

  mojo::PendingRemote<brave_wallet::mojom::TxServiceObserver> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

 private:
  mojo::Receiver<brave_wallet::mojom::TxServiceObserver> observer_receiver_{
      this};
  std::unique_ptr<base::RunLoop> run_loop_new_unapproved_;
  std::unique_ptr<base::RunLoop> run_loop_rejected_;
  std::vector<mojom::TransactionInfoPtr> new_unapproved_txs_;
  std::vector<mojom::TransactionInfoPtr> rejected_txs_;
};

bool WaitForWalletBubble(content::WebContents* web_contents) {
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  if (!tab_helper->IsShowingBubble()) {
    base::RunLoop run_loop;
    tab_helper->SetShowBubbleCallbackForTesting(run_loop.QuitClosure());
    run_loop.Run();
  }

  return tab_helper->IsShowingBubble();
}

void CloseWalletBubble(content::WebContents* web_contents) {
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  tab_helper->CloseBubble();
}

}  // namespace

class SolanaProviderTest : public InProcessBrowserTest {
 public:
  SolanaProviderTest()
      : https_server_for_files_(net::EmbeddedTestServer::TYPE_HTTPS),
        https_server_for_rpc_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~SolanaProviderTest() override = default;

  void SetUpOnMainThread() override {
    brave_wallet::SetDefaultSolanaWallet(
        browser()->profile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_for_files_.SetSSLConfig(
        net::EmbeddedTestServer::CERT_TEST_NAMES);
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII("brave-wallet");
    https_server_for_files_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_for_files()->Start());

    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
    json_rpc_service_ = brave_wallet_service_->json_rpc_service();
    keyring_service_ = brave_wallet_service_->keyring_service();
    AssetRatioServiceFactory::GetServiceForContext(browser()->profile())
        ->EnableDummyPricesForTesting();
    tx_service_ = brave_wallet_service_->tx_service();
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());

    StartRPCServer(base::BindRepeating(&SolanaProviderTest::HandleRequest,
                                       base::Unretained(this)));

    // load solana web3 script
    if (g_provider_solana_web3_script->empty()) {
      *g_provider_solana_web3_script =
          LoadDataResource(IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST);
    }
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("text/html");
    std::string request_path = request.GetURL().path();

    auto body = base::test::ParseJsonDict(request.content);
    auto* method = body.FindString("method");
    EXPECT_TRUE(method);
    if (*method == "isBlockhashValid") {
      std::string reply = R"({
      "jsonrpc": "2.0",
      "id": 1,
      "result": {
        "value": {valid}
      }
    })";
      base::ReplaceFirstSubstringAfterOffset(
          &reply, 0, "{valid}", mock_blockhash_is_valid_ ? "true" : "false");
      http_response->set_content(reply);
    } else if (*method == "getBlockHeight") {
      std::string reply = R"({ "jsonrpc": "2.0", "id": 1, "result": 1233 })";
      http_response->set_content(reply);
    } else if (*method == "getLatestBlockhash") {
      std::string reply = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "context": {
            "slot": 1069
          },
          "value": {
            "blockhash": "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N",
            "lastValidBlockHeight": 18446744073709551615
          }
        }
      })";
      http_response->set_content(reply);
    } else if (*method == "simulateTransaction") {
      std::string reply = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "context": {
            "apiVersion": "1.17.25",
            "slot": 259225005
          },
          "value": {
            "accounts": null,
            "err": null,
            "logs": [
              "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
              "Program log: Instruction: Transfer",
              "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY success"
            ],
            "returnData": null,
            "unitsConsumed": 69017
          }
        }
      })";
      http_response->set_content(reply);
    } else if (*method == "getSignatureStatuses") {
      std::string reply =
          R"({"jsonrpc":"2.0", "id":1, "result":"signature status not provided"})";
      http_response->set_content(reply);
    } else if (*method == "getFeeForMessage") {
      std::string reply =
          R"({"jsonrpc":"2.0", "id":1, "result":{"value":5000}})";
      http_response->set_content(reply);
    } else if (*method == "getRecentPrioritizationFees") {
      std::string reply = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "result": [
          {"prioritizationFee": 100, "slot": 293251906},
          {"prioritizationFee": 200, "slot": 293251906},
          {"prioritizationFee": 0, "slot": 293251805}
        ]
      })";
      http_response->set_content(reply);
    } else {
      std::string reply = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "result": "ns1aBL6AowxpiPzQL3ZeBK1RpCSLq1VfhqNw9KFSsytayARYdYrqrmbmhaizUTTkT4SXEnjnbVmPBrie3o9yuyB"
      })";
      http_response->set_content(reply);
    }
    return std::move(http_response);
  }

  void StartRPCServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_for_rpc()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_for_rpc()->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_for_rpc()->Start());

    // Update rpc url for kLocalhostChainId
    mojom::NetworkInfoPtr chain;
    json_rpc_service_->SetNetwork(mojom::kLocalhostChainId,
                                  mojom::CoinType::SOL, std::nullopt);
    base::RunLoop run_loop;
    json_rpc_service_->GetNetwork(
        mojom::CoinType::SOL, std::nullopt,
        base::BindLambdaForTesting([&](mojom::NetworkInfoPtr info) {
          chain = info.Clone();
          run_loop.Quit();
        }));
    run_loop.Run();
    base::RunLoop run_loop1;
    chain->rpc_endpoints =
        std::vector<GURL>({https_server_for_rpc()->base_url()});
    json_rpc_service_->AddChain(
        std::move(chain),
        base::BindLambdaForTesting([&](const std::string& chain_id,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
          ASSERT_EQ(chain_id, mojom::kLocalhostChainId);
          ASSERT_EQ(error, mojom::ProviderError::kSuccess);
          ASSERT_TRUE(error_message.empty());
          run_loop1.Quit();
        }));
    run_loop1.Run();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server_for_files() {
    return &https_server_for_files_;
  }
  net::EmbeddedTestServer* https_server_for_rpc() {
    return &https_server_for_rpc_;
  }

  std::unique_ptr<TestTxServiceObserver> CreateObserver() {
    std::unique_ptr<TestTxServiceObserver> obs =
        std::make_unique<TestTxServiceObserver>();
    tx_service_->AddObserver(obs->GetReceiver());
    return obs;
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void ReloadAndWaitForLoadStop(Browser* browser) {
    chrome::Reload(browser, WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents()));
  }

  void RestoreWallet() {
    ASSERT_TRUE(keyring_service_->RestoreWalletSync(
        kMnemonicScarePiece, kTestWalletPassword, false));

    EXPECT_EQ(kFirstAccount, GetAccountUtils().EnsureSolAccount(0)->address);
    EXPECT_EQ(kSecondAccount, GetAccountUtils().EnsureSolAccount(1)->address);
  }

  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service_); }

  void LockWallet() {
    keyring_service_->Lock();
    // Needed so KeyringServiceObserver::Locked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  mojom::AccountInfoPtr AddAccount(const std::string& name) {
    return keyring_service_->AddAccountSync(mojom::CoinType::SOL,
                                            mojom::kSolanaKeyringId, name);
  }

  void SetSelectedAccount(const mojom::AccountIdPtr& account_id) {
    EXPECT_TRUE(keyring_service_->SetSelectedAccountSync(account_id->Clone()));
  }

  void UserGrantPermission(bool granted,
                           const mojom::AccountInfoPtr& account_info) {
    if (granted) {
      permissions::BraveWalletPermissionContext::AcceptOrCancel(
          std::vector<std::string>{account_info->address},
          mojom::PermissionLifetimeOption::kForever, web_contents());
    } else {
      permissions::BraveWalletPermissionContext::Cancel(web_contents());
    }
    ASSERT_EQ(EvalJs(web_contents(), "getConnectedAccount()").ExtractString(),
              granted ? account_info->address : "");
  }

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo(
      const mojom::AccountIdPtr& account_id) {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    tx_service_->GetAllTransactionInfo(
        mojom::CoinType::SOL, mojom::kLocalhostChainId, account_id.Clone(),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::TransactionInfoPtr> v) {
              transaction_infos = std::move(v);
              run_loop.Quit();
            }));
    run_loop.Run();
    return transaction_infos;
  }

  void ApproveTransaction(const std::string& tx_meta_id) {
    base::RunLoop run_loop;
    tx_service_->ApproveTransaction(
        mojom::CoinType::SOL, mojom::kLocalhostChainId, tx_meta_id,
        base::BindLambdaForTesting([&](bool success,
                                       mojom::ProviderErrorUnionPtr error_union,
                                       const std::string& error_message) {
          EXPECT_TRUE(success);
          ASSERT_TRUE(error_union->is_solana_provider_error());
          EXPECT_EQ(error_union->get_solana_provider_error(),
                    mojom::SolanaProviderError::kSuccess);
          EXPECT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RejectTransaction(const std::string& tx_meta_id) {
    auto observer = CreateObserver();
    base::RunLoop run_loop;
    tx_service_->RejectTransaction(
        mojom::CoinType::SOL, mojom::kLocalhostChainId, tx_meta_id,
        base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          observer->WaitForRejectedStatus();
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RegisterSolAccountChanged() {
    ASSERT_TRUE(ExecJs(web_contents(), "registerAccountChanged()"));
  }

  void RegisterSolDisconnect() {
    ASSERT_TRUE(ExecJs(web_contents(), "registerDisconnect()"));
  }

  void CallSolanaConnect(const content::ToRenderFrameHost& execution_target,
                         bool is_expect_bubble = true) {
    ASSERT_TRUE(ExecJs(execution_target, "solanaConnect()"));
    if (is_expect_bubble) {
      WaitForWalletBubble(web_contents());
    }
    base::RunLoop().RunUntilIdle();
  }

  void CallSolanaDisconnect(
      const content::ToRenderFrameHost& execution_target) {
    ASSERT_TRUE(EvalJs(execution_target, "solanaDisconnect()").ExtractBool());
  }

  void CallSolanaSignMessage(const std::string& message,
                             const std::string& encoding) {
    CloseWalletBubble(web_contents());
    ASSERT_TRUE(ExecJs(web_contents(),
                       base::StringPrintf(R"(solanaSignMessage('%s', '%s'))",
                                          message.c_str(), encoding.c_str())));
  }

  void CallSolanaRequest(const std::string& json) {
    CloseWalletBubble(web_contents());
    ASSERT_TRUE(
        ExecJs(web_contents(),
               base::StringPrintf(R"(solanaRequest(%s))", json.c_str())));
  }

  std::string GetSignMessageResult() {
    return EvalJs(web_contents(), "getSignMessageResult()").ExtractString();
  }

  void CallSolanaSignAndSendTransaction(
      const std::string& unsigned_tx_array_string,
      const std::string& send_options_string = "{}",
      const std::string& pubkey = "",
      const std::string& signature_array_string = "",
      bool v0 = false) {
    CloseWalletBubble(web_contents());
    const std::string script =
        pubkey.empty()
            ? base::StringPrintf(
                  R"(%s solanaSignAndSendTransaction(%s, new Uint8Array([%s]),
                     %s))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_string.c_str(), send_options_string.c_str())
            : base::StringPrintf(
                  R"(%s solanaSignAndSendTransaction(%s, new Uint8Array([%s]),
                     %s, "%s", new Uint8Array([%s])))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_string.c_str(), send_options_string.c_str(),
                  pubkey.c_str(), signature_array_string.c_str());
    ASSERT_TRUE(ExecJs(web_contents(), script));
  }

  std::string GetSignAndSendTransactionResult() {
    return EvalJs(web_contents(), "getSignAndSendTransactionResult()")
        .ExtractString();
  }

  void CallSolanaSignTransaction(const std::string& unsigned_tx_array_string,
                                 const std::string& pubkey = "",
                                 const std::string& signature_array_string = "",
                                 bool v0 = false) {
    CloseWalletBubble(web_contents());
    const std::string script =
        pubkey.empty()
            ? base::StringPrintf(
                  R"(%s solanaSignTransaction(%s, new Uint8Array([%s])))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_string.c_str())
            : base::StringPrintf(
                  R"(%s solanaSignTransaction(%s, new Uint8Array([%s]), "%s",
                     new Uint8Array([%s])))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_string.c_str(), pubkey.c_str(),
                  signature_array_string.c_str());
    ASSERT_TRUE(ExecJs(web_contents(), script));
  }

  void CallSolanaSignAllTransactions(
      const std::string& unsigned_tx_array_str,
      const std::string& signed_tx_array_str,
      const std::string& pubkey = "",
      const std::string& signature_array_string = "",
      bool v0 = false) {
    CloseWalletBubble(web_contents());
    const std::string script =
        pubkey.empty()
            ? base::StringPrintf(
                  R"(%s solanaSignAllTransactions(%s, new Uint8Array([%s]),
                     new Uint8Array([%s])))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_str.c_str(), signed_tx_array_str.c_str())
            : base::StringPrintf(
                  R"(%s solanaSignAllTransactions(%s, new Uint8Array([%s]),
                     new Uint8Array([%s]), "%s", new Uint8Array([%s])))",
                  g_provider_solana_web3_script->c_str(), v0 ? "true" : "false",
                  unsigned_tx_array_str.c_str(), signed_tx_array_str.c_str(),
                  pubkey.c_str(), signature_array_string.c_str());
    ASSERT_TRUE(ExecJs(web_contents(), script));
  }

  std::string GetSignTransactionResult() {
    return EvalJs(web_contents(), "getSignTransactionResult()").ExtractString();
  }

  std::string GetSignAllTransactionsResult() {
    return EvalJs(web_contents(), "getSignAllTransactionsResult()")
        .ExtractString();
  }

  std::string GetRequestResult() {
    return EvalJs(web_contents(), "getRequestResult()").ExtractString();
  }

  std::string GetAccountChangedResult() {
    return EvalJs(web_contents(), "getAccountChangedResult()").ExtractString();
  }

  bool GetDisconnectEmitted() {
    return EvalJs(web_contents(), "getDisconnectEmitted()").ExtractBool();
  }

  bool IsSolanaConnected(const content::ToRenderFrameHost& execution_target) {
    return EvalJs(execution_target, "isSolanaConnected()").ExtractBool();
  }

  bool GetIsBraveWalletViaProxy() {
    return EvalJs(web_contents(), "getIsBraveWalletViaProxy()").ExtractBool();
  }

  void CallSolanaDisconnectViaProxy() {
    ASSERT_TRUE(
        EvalJs(web_contents(), "solanaDisconnectViaProxy()").ExtractBool());
  }

  void WaitForResultReady() {
    content::DOMMessageQueue message_queue;
    std::string message;
    EXPECT_TRUE(message_queue.WaitForMessage(&message));
    EXPECT_EQ("\"result ready\"", message);
  }

 protected:
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  bool mock_blockhash_is_valid_ = true;

 private:
  TestTxServiceObserver observer_;
  net::test_server::EmbeddedTestServer https_server_for_files_;
  net::test_server::EmbeddedTestServer https_server_for_rpc_;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectRequestInProgress) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  CallSolanaConnect(web_contents());
  const auto result = EvalJs(web_contents(), R"(
  (async function() {
    try {
      return await window.braveSolana.connect()
    } catch (err) {
      return err
    }
  })()
  )");
  ASSERT_TRUE(result.error.empty());
  ASSERT_TRUE(result.value.is_dict());
  EXPECT_EQ(*result.value.GetDict().FindInt("code"),
            static_cast<int>(mojom::SolanaProviderError::kResourceUnavailable));
  EXPECT_EQ(*result.value.GetDict().FindString("message"),
            l10n_util::GetStringUTF8(
                IDS_WALLET_REQUESTED_RESOURCE_NOT_AVAILABLE_ERROR));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusAndPermission) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  RegisterSolDisconnect();

  ASSERT_FALSE(IsSolanaConnected(web_contents()));
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));

  // Removing solana permission will disconnect and emit event.
  host_content_settings_map()->ClearSettingsForOneType(
      ContentSettingsType::BRAVE_SOLANA);
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
  EXPECT_TRUE(GetDisconnectEmitted());

  // Connect it again for following test cases
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  // disconnect() will set connected status to false.
  CallSolanaDisconnect(web_contents());
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
  EXPECT_TRUE(GetDisconnectEmitted());

  // disconnect won't be emitted when there is no connected account
  CallSolanaDisconnect(web_contents());
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
  EXPECT_FALSE(GetDisconnectEmitted());

  // Start testing revoking permission for an origin
  host_content_settings_map()->ClearSettingsForOneType(
      ContentSettingsType::BRAVE_SOLANA);
  // connect for a.test
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  ASSERT_TRUE(AddTabAtIndex(
      1, https_server_for_files()->GetURL("b.test", "/solana_provider.html"),
      ui::PAGE_TRANSITION_TYPED));
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 1);
  RegisterSolDisconnect();
  // connect for b.test
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  // Removing solana permission for a.test only
  host_content_settings_map()->ClearSettingsForOneTypeWithPredicate(
      ContentSettingsType::BRAVE_SOLANA, base::Time(), base::Time::Max(),
      base::BindLambdaForTesting(
          [&url, &account_0](const ContentSettingsPattern& primary_pattern,
                             const ContentSettingsPattern& secondary_pattern) {
            url::Origin new_origin;
            if (GetSubRequestOrigin(permissions::RequestType::kBraveSolana,
                                    url::Origin::Create(url),
                                    account_0->address, &new_origin) &&
                primary_pattern.Matches(new_origin.GetURL())) {
              return true;
            }
            return false;
          }));
  // b.test should still be connected
  EXPECT_TRUE(IsSolanaConnected(web_contents()));
  EXPECT_FALSE(GetDisconnectEmitted());

  // switch back to a.test and it should be disconnected
  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 0);
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
  EXPECT_TRUE(GetDisconnectEmitted());
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusWithDocumentChanged) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_FALSE(IsSolanaConnected(web_contents()));
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));

  // Reload will clear connected status.
  ReloadAndWaitForLoadStop(browser());
  EXPECT_FALSE(IsSolanaConnected(web_contents()));

  // Connect again and try navigate later.
  ASSERT_FALSE(IsSolanaConnected(web_contents()));
  CallSolanaConnect(web_contents(), false);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));

  // Navigate will clear connected status.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusInIframes) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url = https_server_for_files()->GetURL("a.test",
                                              "/iframe_solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  content::RenderFrameHost* main_frame = web_contents()->GetPrimaryMainFrame();

  ASSERT_FALSE(IsSolanaConnected(ChildFrameAt(main_frame, 0)));

  CallSolanaConnect(ChildFrameAt(main_frame, 0), true);
  permissions::BraveWalletPermissionContext::AcceptOrCancel(
      std::vector<std::string>{account_0->address},
      mojom::PermissionLifetimeOption::kForever, web_contents());
  // First iframe is now connected.
  EXPECT_TRUE(IsSolanaConnected(ChildFrameAt(main_frame, 0)));
  // Second iframe is still disconnected
  EXPECT_FALSE(IsSolanaConnected(ChildFrameAt(main_frame, 1)));

  CallSolanaConnect(ChildFrameAt(main_frame, 1), false);
  // Second iframe is now connected
  EXPECT_TRUE(IsSolanaConnected(ChildFrameAt(main_frame, 1)));

  // Disconnect first iframe won't affect second iframe's connected status
  CallSolanaDisconnect(ChildFrameAt(main_frame, 0));
  EXPECT_FALSE(IsSolanaConnected(ChildFrameAt(main_frame, 0)));
  EXPECT_TRUE(IsSolanaConnected(ChildFrameAt(main_frame, 1)));

  // Disabling this part of the test because:
  // 1. The first child frame is explicitly disconnected above so navigating
  // away makes no difference.
  // 2. BraveWalletProviderDelegateImpl::RenderFrameHostChanged doesn't get
  // triggered by the below navigation in the second iframe, so the connection
  // remains.
#if 0
  GURL new_iframe_url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  // navigate first iframe away won't affect second iframe's connected status
  EXPECT_TRUE(
      NavigateIframeToURL(web_contents(), "test-iframe-0", new_iframe_url));
  EXPECT_FALSE(IsSolanaConnected(ChildFrameAt(main_frame, 0)));
  EXPECT_TRUE(IsSolanaConnected(ChildFrameAt(main_frame, 1)));

  // navigate second iframe awau will clear its connected status
  EXPECT_TRUE(
      NavigateIframeToURL(web_contents(), "test-iframe-1", new_iframe_url));
  EXPECT_TRUE(IsSolanaConnected(ChildFrameAt(main_frame, 1)));
#endif
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusInMultiFrames) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_FALSE(IsSolanaConnected(web_contents()));
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  // First tab is now connected.
  EXPECT_TRUE(IsSolanaConnected(web_contents()));
  // Add same url at second tab
  ASSERT_TRUE(AddTabAtIndex(1, url, ui::PAGE_TRANSITION_TYPED));
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 1);
  // Connected status of second tab is separate from first tab.
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
  // Doing successful connect and disconnect shouldn't affect first tab.
  // Since a.test already has the permission so connect would success without
  // asking.
  CallSolanaConnect(web_contents(), false);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));
  CallSolanaDisconnect(web_contents());
  EXPECT_FALSE(IsSolanaConnected(web_contents()));

  // Swtich back to first tab and it should still be connected,
  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 0);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignMessage) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  size_t request_index = 0;
  CallSolanaSignMessage(kMessage, "utf8");
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user rejected request
  brave_wallet_service_->NotifySignMessageRequestProcessed(
      false, request_index++, nullptr, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignMessageResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  for (const std::string& encoding : {"utf8", "hex", "invalid", ""}) {
    CallSolanaSignMessage(kMessage, encoding);
    EXPECT_TRUE(WaitForWalletBubble(web_contents()));
    // user approved request
    brave_wallet_service_->NotifySignMessageRequestProcessed(
        true, request_index++, nullptr, std::nullopt);
    WaitForResultReady();
    EXPECT_EQ(GetSignMessageResult(), kExpectedSignature);
  }
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, GetPublicKey) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  constexpr char get_public_key_script[] =
      "window.braveSolana.publicKey ? window.braveSolana.publicKey.toString() "
      ": ''";

  // Will get null in disconnected state
  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script).ExtractString(), "");

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script).ExtractString(),
            account_0->address);

  LockWallet();
  // Publickey is still accessible when wallet is locked
  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script).ExtractString(),
            account_0->address);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignAndSendTransaction) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  auto account_1 = GetAccountUtils().EnsureSolAccount(1);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  auto observer = CreateObserver();
  CallSolanaSignAndSendTransaction(kUnsignedTxArrayStr);
  observer->WaitForNewUnapprovedTx();
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));

  auto infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(1UL, infos.size());
  EXPECT_EQ(account_0->account_id, infos[0]->from_account_id);
  EXPECT_EQ(account_0->address, infos[0]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[0]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[0]->tx_type);
  EXPECT_EQ(MakeOriginInfo(https_server_for_files()->GetOrigin("a.test")),
            infos[0]->origin_info);
  const std::string tx1_id = infos[0]->id;
  RejectTransaction(tx1_id);

  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(1UL, infos.size());
  EXPECT_EQ(account_0->account_id, infos[0]->from_account_id);
  EXPECT_EQ(account_0->address, infos[0]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Rejected, infos[0]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[0]->tx_type);
  EXPECT_TRUE(infos[0]->tx_hash.empty());

  WaitForResultReady();
  EXPECT_EQ(GetSignAndSendTransactionResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  const std::string send_options =
      R"({"maxRetries":1,"preflightCommitment":"confirmed","skipPreflight":true})";
  observer = CreateObserver();
  CallSolanaSignAndSendTransaction(kUnsignedTxArrayStr, send_options);
  observer->WaitForNewUnapprovedTx();
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));

  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(2UL, infos.size());
  size_t tx2_index = 0;
  for (size_t i = 0; i < infos.size(); i++) {
    if (infos[i]->id != tx1_id) {
      tx2_index = i;
      break;
    }
  }
  const std::string tx2_id = infos[tx2_index]->id;
  EXPECT_EQ(account_0->account_id, infos[tx2_index]->from_account_id);
  EXPECT_EQ(account_0->address, infos[tx2_index]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[tx2_index]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[tx2_index]->tx_type);
  EXPECT_EQ(MakeOriginInfo(https_server_for_files()->GetOrigin("a.test")),
            infos[tx2_index]->origin_info);

  ApproveTransaction(infos[tx2_index]->id);

  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(2UL, infos.size());
  EXPECT_EQ(account_0->account_id, infos[tx2_index]->from_account_id);
  EXPECT_EQ(account_0->address, infos[tx2_index]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Submitted, infos[tx2_index]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[tx2_index]->tx_type);
  EXPECT_EQ(infos[tx2_index]->tx_hash, kEncodedSignature);
  ASSERT_TRUE(infos[tx2_index]->tx_data_union->is_solana_tx_data());
  EXPECT_EQ(infos[tx2_index]->tx_data_union->get_solana_tx_data()->send_options,
            mojom::SolanaSendTransactionOptions::New(
                mojom::OptionalMaxRetries::New(1), "confirmed",
                mojom::OptionalSkipPreflight::New(true)));

  WaitForResultReady();
  EXPECT_EQ(GetSignAndSendTransactionResult(), kEncodedSignature);

  observer = CreateObserver();
  CallSolanaSignAndSendTransaction(kUnsignedTxArrayStr2, send_options,
                                   account_1->address,
                                   kSecondAccountSignatureArray);
  observer->WaitForNewUnapprovedTx();
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));

  // Test transaction.signatures.
  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(3UL, infos.size());
  size_t tx3_index = 0;
  for (size_t i = 0; i < infos.size(); i++) {
    if (infos[i]->id != tx1_id && infos[i]->id != tx2_id) {
      tx3_index = i;
      break;
    }
  }
  const std::string tx3_id = infos[tx3_index]->id;
  EXPECT_EQ(account_0->account_id, infos[tx3_index]->from_account_id);
  EXPECT_EQ(account_0->address, infos[tx3_index]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[tx3_index]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[tx3_index]->tx_type);
  EXPECT_EQ(MakeOriginInfo(https_server_for_files()->GetOrigin("a.test")),
            infos[tx3_index]->origin_info);

  ApproveTransaction(tx3_id);

  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(3UL, infos.size());
  EXPECT_EQ(tx3_id, infos[tx3_index]->id);
  EXPECT_EQ(account_0->account_id, infos[tx3_index]->from_account_id);
  EXPECT_EQ(account_0->address, infos[tx3_index]->from_address);
  EXPECT_EQ(mojom::TransactionStatus::Submitted, infos[tx3_index]->tx_status);
  EXPECT_EQ(mojom::TransactionType::SolanaDappSignAndSendTransaction,
            infos[tx3_index]->tx_type);
  // This comes from mock RPC response so it's still kEncodedSignature.
  EXPECT_EQ(infos[tx3_index]->tx_hash, kEncodedSignature);
  ASSERT_TRUE(infos[tx3_index]->tx_data_union->is_solana_tx_data());

  std::vector<mojom::SignaturePubkeyPairPtr> signatures;
  signatures.push_back(
      mojom::SignaturePubkeyPair::New(nullptr, account_0->address));
  signatures.push_back(mojom::SignaturePubkeyPair::New(
      mojom::SolanaSignature::New(
          std::vector<uint8_t>(std::begin(kSecondAccountSignature),
                               std::end(kSecondAccountSignature))),
      account_1->address));

  EXPECT_EQ(infos[tx3_index]
                ->tx_data_union->get_solana_tx_data()
                ->sign_transaction_param,
            mojom::SolanaSignTransactionParam::New(kEncodedSerializedMessage,
                                                   std::move(signatures)));

  WaitForResultReady();
  // This comes from mock RPC response so it's still kEncodedSignature.
  EXPECT_EQ(GetSignAndSendTransactionResult(), kEncodedSignature);

  // Test v0 transaction.
  observer = CreateObserver();
  CallSolanaSignAndSendTransaction(kUnsignedTxArrayStrV0, "{}", "", "", true);
  observer->WaitForNewUnapprovedTx();
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));

  infos = GetAllTransactionInfo(account_0->account_id);
  EXPECT_EQ(4UL, infos.size());
  size_t tx4_index = 0;
  for (size_t i = 0; i < infos.size(); i++) {
    if (infos[i]->id != tx1_id && infos[i]->id != tx2_id &&
        infos[i]->id != tx3_id) {
      tx4_index = i;
      break;
    }
  }
  ApproveTransaction(infos[tx4_index]->id);

  WaitForResultReady();
  // This comes from mock RPC response so it's still kEncodedSignature.
  EXPECT_EQ(GetSignAndSendTransactionResult(), kEncodedSignature);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, AccountChangedEventAndReload) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  auto account_1 = GetAccountUtils().EnsureSolAccount(1);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // connect account 1
  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);

  RegisterSolAccountChanged();
  // switch to disconnected account 2
  SetSelectedAccount(account_1->account_id);
  WaitForResultReady();
  EXPECT_EQ(GetAccountChangedResult(), "");
  // switch to connected account 1
  SetSelectedAccount(account_0->account_id);
  WaitForResultReady();
  EXPECT_EQ(GetAccountChangedResult(), account_0->address);

  ReloadAndWaitForLoadStop(browser());

  RegisterSolAccountChanged();
  // switch to disconnected account 2
  SetSelectedAccount(account_1->account_id);
  WaitForResultReady();
  EXPECT_EQ(GetAccountChangedResult(), "");
  // switch to disconnected account 1
  SetSelectedAccount(account_0->account_id);
  WaitForResultReady();
  EXPECT_EQ(GetAccountChangedResult(), "");
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectWithNonSelectedAccount) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  auto account_1 = GetAccountUtils().EnsureSolAccount(1);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect(web_contents());
  UserGrantPermission(false, account_1);
  auto selected_account = keyring_service_->GetSelectedSolanaDappAccount();
  ASSERT_TRUE(selected_account);
  // Reject connect request won't set selected account
  EXPECT_EQ(selected_account, account_0);
  EXPECT_FALSE(IsSolanaConnected(web_contents()));

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_1);
  selected_account = keyring_service_->GetSelectedSolanaDappAccount();
  ASSERT_TRUE(selected_account);
  // Connect successfuly will set selected acount automatically
  EXPECT_EQ(selected_account, account_1);
  EXPECT_TRUE(IsSolanaConnected(web_contents()));

  // Disconnect account 2 which has permission and select account 1 to test
  // permitted account filtering.
  CallSolanaDisconnect(web_contents());
  ASSERT_FALSE(IsSolanaConnected(web_contents()));
  SetSelectedAccount(account_0->account_id);

  // If permitted account 2 isn't filtered and we reject account 1,
  // RequestPermissions callback will contain account 2 in allowed accounts
  CallSolanaConnect(web_contents());
  UserGrantPermission(false, account_0);
  EXPECT_FALSE(IsSolanaConnected(web_contents()));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignTransaction) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  auto account_1 = GetAccountUtils().EnsureSolAccount(1);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  size_t request_index = 0;
  CallSolanaSignTransaction(kUnsignedTxArrayStr);

  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user rejected request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      false, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  CallSolanaSignTransaction(kUnsignedTxArrayStr);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(), kSignedTxArrayStr);

  CallSolanaSignTransaction(kUnsignedTxArrayStr2, account_1->address,
                            kSecondAccountSignatureArray);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(), kSignedTxArrayStr2);

  // Test the case where the transaction is already signed by the fee-payer,
  // and ask for the connected wallet account to co-sign the transaction.
  CallSolanaSignTransaction(kUnsignedTxArrayStr3, account_1->address,
                            kSecondAccountSignatureArray2);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(), kSignedTxArrayStr3);

  // Test v0 transaction.
  CallSolanaSignTransaction(kUnsignedTxArrayStrV0, "", "", true);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(), kSignedTxArrayStrV0);

  // Test blockhash is invalid.
  mock_blockhash_is_valid_ = false;
  CallSolanaSignTransaction(kUnsignedTxArrayStr);
  WaitForResultReady();
  EXPECT_EQ(GetSignTransactionResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_BLOCKHASH_ERROR));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignAllTransactions) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  auto account_1 = GetAccountUtils().EnsureSolAccount(1);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect(web_contents());
  UserGrantPermission(true, account_0);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  size_t request_index = 0;
  CallSolanaSignAllTransactions(kUnsignedTxArrayStr, kSignedTxArrayStr);

  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user rejected request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      false, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignAllTransactionsResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  CallSolanaSignAllTransactions(kUnsignedTxArrayStr, kSignedTxArrayStr);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignAllTransactionsResult(), "success");

  CallSolanaSignAllTransactions(kUnsignedTxArrayStr2, kSignedTxArrayStr2,
                                account_1->address,
                                kSecondAccountSignatureArray);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignAllTransactionsResult(), "success");

  // Test v0 transaction.
  CallSolanaSignAllTransactions(kUnsignedTxArrayStrV0, kSignedTxArrayStrV0, "",
                                "", true);
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  // user approved request
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, request_index++, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetSignAllTransactionsResult(), "success");

  // Test blockhash is invalid.
  mock_blockhash_is_valid_ = false;
  CallSolanaSignAllTransactions(kUnsignedTxArrayStr, kSignedTxArrayStr);
  WaitForResultReady();
  EXPECT_EQ(GetSignAllTransactionsResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_BLOCKHASH_ERROR));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Request) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // connect and accept
  CallSolanaRequest(R"({method: "connect"})");
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  permissions::BraveWalletPermissionContext::AcceptOrCancel(
      std::vector<std::string>{account_0->address},
      mojom::PermissionLifetimeOption::kForever, web_contents());
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), account_0->address);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  // disconnect
  CallSolanaRequest(R"({method: "disconnect"})");
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), "success");
  ASSERT_FALSE(IsSolanaConnected(web_contents()));

  // eagerly connect
  CallSolanaRequest(R"({method: "connect", params: { onlyIfTrusted: true }})");
  EXPECT_FALSE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), account_0->address);
  ASSERT_TRUE(IsSolanaConnected(web_contents()));

  // signMessage
  CallSolanaRequest(base::StringPrintf(R"(
    {method: "signMessage", params: { message: new Uint8Array([%s]) }})",
                                       kEncodedMessage));
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, 0, nullptr,
                                                           std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), kExpectedEncodedSignature);

  // signTransaction
  CallSolanaRequest(base::StringPrintf(R"(
    {method: "signTransaction", params: { message: '%s' }})",
                                       kEncodedUnsignedTxArrayStr));
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, 0, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), kEncodedSignature);

  // signAndSendTransaction
  auto observer = CreateObserver();
  const std::string send_options =
      R"({"maxRetries":1,"preflightCommitment":"confirmed","skipPreflight":true})";
  CallSolanaRequest(base::StringPrintf(R"(
    {method: "signAndSendTransaction", params: { message: '%s', options: %s }})",
                                       kEncodedUnsignedTxArrayStr,
                                       send_options.c_str()));
  observer->WaitForNewUnapprovedTx();
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  auto infos = GetAllTransactionInfo(account_0->account_id);
  ASSERT_EQ(infos.size(), 1u);
  ASSERT_TRUE(infos[0]->tx_data_union->is_solana_tx_data());
  EXPECT_EQ(infos[0]->tx_data_union->get_solana_tx_data()->send_options,
            mojom::SolanaSendTransactionOptions::New(
                mojom::OptionalMaxRetries::New(1), "confirmed",
                mojom::OptionalSkipPreflight::New(true)));
  ApproveTransaction(infos[0]->id);
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(), kEncodedSignature);

  // signAllTransactions
  CallSolanaRequest(base::StringPrintf(R"(
    {method: "signAllTransactions", params: { message: ['%s', '%s'] }})",
                                       kEncodedUnsignedTxArrayStr,
                                       kEncodedUnsignedTxArrayStr));
  EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  brave_wallet_service_->NotifySignSolTransactionsRequestProcessed(
      true, 1, {}, std::nullopt);
  WaitForResultReady();
  EXPECT_EQ(GetRequestResult(),
            base::StrCat({kEncodedSignature, ",", kEncodedSignature}));
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, NoCrashOnShortLivedIframes) {
  RestoreWallet();
  auto account_0 = GetAccountUtils().EnsureSolAccount(0);
  SetSelectedAccount(account_0->account_id);

  GURL url =
      https_server_for_files()->GetURL("a.test", "/short_lived_iframes.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ReloadAndWaitForLoadStop(browser());
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, CallViaProxy) {
  GURL url =
      https_server_for_files()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  EXPECT_TRUE(GetIsBraveWalletViaProxy());
  CallSolanaDisconnectViaProxy();
}

}  // namespace brave_wallet

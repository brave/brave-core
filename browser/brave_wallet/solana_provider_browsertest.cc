/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/l10n/l10n_util.h"

using brave_wallet::mojom::SolanaProviderError;

namespace {

// error returns from browser process
constexpr char kErrorMessage[] = "error from browser";
constexpr char kTestPublicKey[] =
    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
constexpr char kTestSignature[] =
    "As4N6cok5f7nhXp56Hdw8dWZpUnY8zjYKzBqK45CexE1qNPCqt6Y2gnZduGgqASDD1c6QULBRy"
    "pVa9BikoxWpGA";

const std::vector<uint8_t> kSerializedMessage = {
    1,   0,   1,   2,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159, 171,
    200, 40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,   58,  251,
    215, 62,  201, 172, 159, 197, 0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   65,  223, 160, 84,  229, 200, 41,
    124, 255, 227, 200, 207, 94,  13,  128, 218, 71,  139, 178, 169, 91,  44,
    201, 177, 125, 166, 36,  96,  136, 125, 3,   136, 1,   1,   2,   0,   0,
    12,  2,   0,   0,   0,   100, 0,   0,   0,   0,   0,   0,   0};

const std::vector<uint8_t> kSerializedTx = {
    1,   224, 52,  14,  175, 211, 221, 245, 217, 123, 232, 68,  232, 120, 145,
    131, 154, 133, 31,  130, 73,  190, 13,  227, 109, 83,  152, 160, 202, 226,
    134, 138, 141, 135, 187, 72,  153, 173, 159, 205, 222, 253, 26,  44,  34,
    18,  250, 176, 21,  84,  7,   142, 247, 65,  218, 40,  117, 145, 118, 52,
    75,  183, 98,  232, 10,  1,   0,   1,   2,   161, 51,  89,  91,  115, 210,
    217, 212, 76,  159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209,
    108, 143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   65,  223,
    160, 84,  229, 200, 41,  124, 255, 227, 200, 207, 94,  13,  128, 218, 71,
    139, 178, 169, 91,  44,  201, 177, 125, 166, 36,  96,  136, 125, 3,   136,
    1,   1,   2,   0,   0,   12,  2,   0,   0,   0,   100, 0,   0,   0,   0,
    0,   0,   0};

const std::vector<uint8_t> kSignedTx = {
    1,   231, 78,  150, 120, 219, 234, 88,  44,  144, 225, 53,  221, 88,  82,
    59,  51,  62,  211, 225, 231, 182, 123, 231, 229, 201, 48,  30,  137, 119,
    233, 102, 88,  31,  65,  88,  147, 197, 72,  166, 241, 126, 26,  59,  239,
    64,  196, 116, 28,  17,  124, 0,   123, 13,  28,  65,  242, 241, 226, 46,
    227, 55,  234, 251, 10,  1,   0,   1,   2,   161, 51,  89,  91,  115, 210,
    217, 212, 76,  159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209,
    108, 143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   62,  84,
    174, 253, 228, 77,  50,  164, 215, 178, 46,  88,  242, 49,  114, 246, 244,
    48,  9,   18,  36,  41,  160, 254, 174, 6,   207, 115, 11,  58,  220, 167,
    1,   1,   2,   0,   0,   12,  2,   0,   0,   0,   100, 0,   0,   0,   0,
    0,   0,   0};

const std::vector<uint8_t> kMessageToSign = {
    84,  111, 32,  97,  118, 111, 105, 100, 32,  100, 105, 103, 105, 116, 97,
    108, 32,  100, 111, 103, 110, 97,  112, 112, 101, 114, 115, 44,  32,  115,
    105, 103, 110, 32,  98,  101, 108, 111, 119, 32,  116, 111, 32,  97,  117,
    116, 104, 101, 110, 116, 105, 99,  97,  116, 101, 32,  119, 105, 116, 104,
    32,  67,  114, 121, 112, 116, 111, 67,  111, 114, 103, 105, 115, 46};

constexpr char OnAccountChangedScript[] =
    R"(async function disconnect() {await window.solana.disconnect()}
       window.solana.on('accountChanged', (result) => {
        if (result instanceof Object)
          window.domAutomationController.send(result.toString())
        else
          window.domAutomationController.send(result)
       })
       disconnect())";

std::string VectorToArrayString(const std::vector<uint8_t>& vec) {
  std::string result;
  for (size_t i = 0; i < vec.size(); ++i) {
    base::StrAppend(&result, {base::NumberToString(vec[i])});
    if (i != vec.size() - 1)
      base::StrAppend(&result, {", "});
  }
  return result;
}

std::string GetRequstObject(const std::string& method) {
  return base::StringPrintf(R"({method: "%s", params: {}})", method.c_str());
}

std::string ConnectScript(const std::string& args) {
  return base::StringPrintf(
      R"(async function connect() {
          try { const result = await window.solana.connect(%s)
            window.domAutomationController.send(result.publicKey.toString())
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } connect())",
      args.c_str());
}

std::string SignTransactionScript(const std::string& args) {
  const std::string signed_tx = VectorToArrayString(kSignedTx);
  return base::StringPrintf(
      R"(async function signTransaction() {
          try {
            const result = await window.solana.signTransaction%s
            if (result.serialize().join() === new Uint8Array([%s]).join())
              window.domAutomationController.send(true)
            else
              window.domAutomationController.send(false)
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } signTransaction())",
      args.c_str(), signed_tx.c_str());
}

std::string SignAllTransactionsScript(const std::string& args) {
  const std::string signed_tx = VectorToArrayString(kSignedTx);
  return base::StringPrintf(
      R"(async function signAllTransactions() {
          try {
            const result = await window.solana.signAllTransactions%s
            const isSameTx =
              (tx) => tx.serialize().join() === new Uint8Array([%s]).join()
            if (result.every(isSameTx))
              window.domAutomationController.send(true)
            else
              window.domAutomationController.send(false)
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } signAllTransactions())",
      args.c_str(), signed_tx.c_str());
}

std::string SignAndSendTransactionScript(const std::string& args) {
  const std::string expected_result = base::StringPrintf(
      R"({ publicKey: "%s", signature: "%s"})", kTestPublicKey, kTestSignature);
  return base::StringPrintf(
      R"(async function signAndSendTransaction() {
          try {
            const result = await window.solana.signAndSendTransaction%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              window.domAutomationController.send(true)
            else
              window.domAutomationController.send(false)
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } signAndSendTransaction())",
      args.c_str(), expected_result.c_str());
}

std::string SignMessageScript(const std::string& args) {
  std::vector<uint8_t> signature(brave_wallet::kSolanaSignatureSize);
  EXPECT_TRUE(brave_wallet::Base58Decode(std::string(kTestSignature),
                                         &signature, signature.size()));
  const std::string signature_str = VectorToArrayString(signature);
  const std::string expected_result = base::StringPrintf(
      R"({ publicKey: "%s", signature: new Uint8Array([%s])})", kTestPublicKey,
      signature_str.c_str());
  return base::StringPrintf(
      R"(async function signMessage() {
          try {
            const result = await window.solana.signMessage%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              window.domAutomationController.send(true)
            else
              window.domAutomationController.send(false)
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } signMessage())",
      args.c_str(), expected_result.c_str());
}

std ::string RequestScript(const std::string& args) {
  const std::string expected_result = base::StringPrintf(
      R"({ publicKey: "%s", signature: "%s"})", kTestPublicKey, kTestSignature);
  return base::StringPrintf(
      R"(async function request() {
          try {
            const result = await window.solana.request%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              window.domAutomationController.send(true)
            else if (result.publicKey)
              window.domAutomationController.send(result.publicKey.toString())
            else
              window.domAutomationController.send(false)
          } catch (err) {
            window.domAutomationController.send(err.message + (err.code ?? ""))
          }
        } request())",
      args.c_str(), expected_result.c_str());
}

class TestSolanaProvider final : public brave_wallet::mojom::SolanaProvider {
 public:
  TestSolanaProvider() = default;
  ~TestSolanaProvider() override = default;
  TestSolanaProvider(const TestSolanaProvider&) = delete;
  TestSolanaProvider& operator=(const TestSolanaProvider&) = delete;

  void Init(mojo::PendingRemote<brave_wallet::mojom::SolanaEventsListener>
                events_listener) override {
    if (!events_listener_.is_bound()) {
      events_listener_.Bind(std::move(events_listener));
    }
  }
  void Connect(absl::optional<base::Value> arg,
               ConnectCallback callback) override {
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              kTestPublicKey);
    } else {
      std::move(callback).Run(error_, error_message_, "");
      ClearError();
    }
  }
  void Disconnect() override {
    // Used to test onAccountChanged
    if (emit_empty_account_changed_)
      events_listener_->AccountChangedEvent(absl::nullopt);
    else
      events_listener_->AccountChangedEvent(kTestPublicKey);
  }
  void IsConnected(IsConnectedCallback callback) override {
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(true);
    } else {
      std::move(callback).Run(false);
      ClearError();
    }
  }
  void GetPublicKey(GetPublicKeyCallback callback) override {
    std::move(callback).Run(kTestPublicKey);
  }
  void SignTransaction(const std::string& encoded_serialized_msg,
                       SignTransactionCallback callback) override {
    EXPECT_EQ(encoded_serialized_msg,
              brave_wallet::Base58Encode(kSerializedMessage));
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(SolanaProviderError::kSuccess, "", kSignedTx);
    } else {
      std::move(callback).Run(error_, error_message_, std::vector<uint8_t>());
      ClearError();
    }
  }
  void SignAllTransactions(
      const std::vector<std::string>& encoded_serialized_msgs,
      SignAllTransactionsCallback callback) override {
    for (const auto& encoded_serialized_msg : encoded_serialized_msgs)
      EXPECT_EQ(encoded_serialized_msg,
                brave_wallet::Base58Encode(kSerializedMessage));
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              {kSignedTx, kSignedTx});
    } else {
      std::move(callback).Run(error_, error_message_,
                              std::vector<std::vector<uint8_t>>());
      ClearError();
    }
  }
  void SignAndSendTransaction(
      const std::string& encoded_serialized_msg,
      SignAndSendTransactionCallback callback) override {
    EXPECT_EQ(encoded_serialized_msg,
              brave_wallet::Base58Encode(kSerializedMessage));
    base::Value result(base::Value::Type::DICTIONARY);
    if (error_ == SolanaProviderError::kSuccess) {
      result.SetStringKey("publicKey", kTestPublicKey);
      result.SetStringKey("signature", kTestSignature);
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              std::move(result));
    } else {
      std::move(callback).Run(error_, error_message_, std::move(result));
      ClearError();
    }
  }
  void SignMessage(const std::string& encoded_msg,
                   const absl::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override {
    EXPECT_EQ(encoded_msg, brave_wallet::Base58Encode(kMessageToSign));
    base::Value result(base::Value::Type::DICTIONARY);
    if (error_ == SolanaProviderError::kSuccess) {
      result.SetStringKey("publicKey", kTestPublicKey);
      result.SetStringKey("signature", kTestSignature);
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              std::move(result));
    } else {
      std::move(callback).Run(error_, error_message_, std::move(result));
      ClearError();
    }
  }
  void Request(base::Value arg, RequestCallback callback) override {
    base::Value result(base::Value::Type::DICTIONARY);
    if (error_ == SolanaProviderError::kSuccess) {
      result.SetStringKey("publicKey", kTestPublicKey);
      result.SetStringKey("signature", kTestSignature);
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              std::move(result));
    } else {
      std::move(callback).Run(error_, error_message_, std::move(result));
      ClearError();
    }
  }

  void SetError(SolanaProviderError error, const std::string& error_message) {
    error_ = error;
    error_message_ = error_message;
  }

  void SetEmitEmptyAccountChanged(bool value) {
    emit_empty_account_changed_ = value;
  }

 private:
  void ClearError() {
    error_ = SolanaProviderError::kSuccess;
    error_message_.clear();
  }
  SolanaProviderError error_ = SolanaProviderError::kSuccess;
  std::string error_message_;
  bool emit_empty_account_changed_ = false;
  mojo::Remote<brave_wallet::mojom::SolanaEventsListener> events_listener_;
};

class TestBraveContentBrowserClient : public BraveContentBrowserClient {
 public:
  TestBraveContentBrowserClient() = default;
  ~TestBraveContentBrowserClient() override = default;
  TestBraveContentBrowserClient(const TestBraveContentBrowserClient&) = delete;
  TestBraveContentBrowserClient& operator=(
      const TestBraveContentBrowserClient&) = delete;

  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override {
    BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
        render_frame_host, map);
    // override binding for SolanaProvider
    map->Add<brave_wallet::mojom::SolanaProvider>(
        base::BindRepeating(&TestBraveContentBrowserClient::BindSolanaProvider,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  TestSolanaProvider* GetProvider(content::RenderFrameHost* frame_host) {
    if (!provider_map_.contains(frame_host))
      return nullptr;
    return static_cast<TestSolanaProvider*>(
        provider_map_.at(frame_host)->impl());
  }
  bool WaitForBinding(content::RenderFrameHost* render_frame_host,
                      base::OnceClosure callback) {
    if (IsBound(render_frame_host))
      return false;
    quit_on_binding_ = std::move(callback);
    return true;
  }
  bool IsBound(content::RenderFrameHost* frame_host) {
    return provider_map_.contains(frame_host);
  }

 private:
  void BindSolanaProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<brave_wallet::mojom::SolanaProvider> receiver) {
    auto provider = mojo::MakeSelfOwnedReceiver(
        std::make_unique<TestSolanaProvider>(), std::move(receiver));
    provider->set_connection_error_handler(
        base::BindOnce(&TestBraveContentBrowserClient::OnDisconnect,
                       weak_ptr_factory_.GetWeakPtr(), frame_host));
    provider_map_[frame_host] = provider;
    if (quit_on_binding_)
      std::move(quit_on_binding_).Run();
  }
  void OnDisconnect(content::RenderFrameHost* frame_host) {
    provider_map_.erase(frame_host);
  }

  base::OnceClosure quit_on_binding_;
  base::flat_map<
      content::RenderFrameHost*,
      mojo::SelfOwnedReceiverRef<brave_wallet::mojom::SolanaProvider>>
      provider_map_;
  base::WeakPtrFactory<TestBraveContentBrowserClient> weak_ptr_factory_{this};
};

}  // namespace

class SolanaProviderTest : public InProcessBrowserTest {
 public:
  SolanaProviderTest() {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSolanaFeature);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    content::SetBrowserClientForTesting(&test_content_browser_client_);
    host_resolver()->AddRule("*", "127.0.0.1");

    embedded_test_server()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());

    // This is intentional to trigger
    // TestBraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame
    NavigateToURLAndWaitForLoadStop(browser(), GURL("brave://settings"));

    GURL url = embedded_test_server()->GetURL("/empty.html");
    NavigateToURLAndWaitForLoadStop(browser(), url);

    ASSERT_TRUE(base::FeatureList::IsEnabled(
        brave_wallet::features::kNativeBraveWalletFeature));
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  content::WebContents* web_contents(Browser* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForLoadStop(Browser* browser, const GURL& url) {
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser, url)) << ":" << url;
    ASSERT_TRUE(WaitForLoadStop(web_contents(browser))) << ":" << url;
  }

 protected:
  TestBraveContentBrowserClient test_content_browser_client_;
  base::test::ScopedFeatureList feature_list_;

 private:
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

class SolanaProviderDisabledTest : public SolanaProviderTest {
 public:
  SolanaProviderDisabledTest() {
    feature_list_.Reset();
    feature_list_.InitAndDisableFeature(
        brave_wallet::features::kBraveWalletSolanaFeature);
  }
};

IN_PROC_BROWSER_TEST_F(SolanaProviderDisabledTest, SolanaObject) {
  auto result = EvalJs(web_contents(browser()),
                       "window.domAutomationController.send(!!window.solana)",
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Incognito) {
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  GURL url = embedded_test_server()->GetURL("/empty.html");
  NavigateToURLAndWaitForLoadStop(private_browser, url);

  auto result = EvalJs(web_contents(private_browser),
                       "window.domAutomationController.send(!!window.solana)",
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, IsPhantom) {
  auto result =
      EvalJs(web_contents(browser()),
             "window.domAutomationController.send(window.solana.isPhantom)",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Connect) {
  auto result = EvalJs(web_contents(browser()), ConnectScript(""),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  // allow extra parameters
  auto result2 = EvalJs(web_contents(browser()), ConnectScript("{}, 123"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result2.value);

  // non object args
  auto result3 = EvalJs(web_contents(browser()), ConnectScript("123"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result4 = EvalJs(web_contents(browser()), ConnectScript(""),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result4.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, OnConnect) {
  auto result =
      EvalJs(web_contents(browser()),
             R"(async function connect() {await window.solana.connect()}
              window.solana.on('connect',
                (key) => window.domAutomationController.send(key.toString()))
              connect())",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, IsConnected) {
  auto result =
      EvalJs(web_contents(browser()),
             "window.domAutomationController.send(window.solana.isConnected)",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  // just make TestSolanaProvider::IsConnected to return false
  provider->SetError(SolanaProviderError::kUserRejectedRequest, "");

  auto result2 =
      EvalJs(web_contents(browser()),
             "window.domAutomationController.send(window.solana.isConnected)",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(false), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, GetPublicKey) {
  auto result = EvalJs(
      web_contents(browser()),
      "window.domAutomationController.send(window.solana.publicKey.toString())",
      content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Disconnect) {
  auto result = EvalJs(web_contents(browser()),
                       R"(async function disconnect() {
                  const result = await window.solana.disconnect()
                  if (result == undefined)
                    window.domAutomationController.send(true)
                  else
                    window.domAutomationController.send(false)
                }
                disconnect())",
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  // OnDisconnect
  auto result2 =
      EvalJs(web_contents(browser()),
             R"(async function disconnect() {await window.solana.disconnect()}
                window.solana.on('disconnect', (arg) => {
                  if (!arg)
                    window.domAutomationController.send(true)
                  else
                    window.domAutomationController.send(false)
                })
                disconnect())",
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignTransaction) {
  const std::string serialized_tx_str = VectorToArrayString(kSerializedTx);
  const std::string tx =
      base::StrCat({"(window.solana.createTransaction(new Uint8Array([",
                    serialized_tx_str, "])))"});
  auto result = EvalJs(web_contents(browser()), SignTransactionScript(tx),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  // allow extra parameters
  const std::string tx2 = base::StrCat({"(", tx, ", {})"});
  auto result2 = EvalJs(web_contents(browser()), SignTransactionScript(tx2),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result3 = EvalJs(web_contents(browser()), SignTransactionScript("()"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not solanaWeb3.Transaction
  auto result4 =
      EvalJs(web_contents(browser()), SignTransactionScript("('123')"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result5 = EvalJs(web_contents(browser()), SignTransactionScript(tx),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result5.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignAllTransactions) {
  const std::string serialized_tx_str = VectorToArrayString(kSerializedTx);
  const std::string txs = base::StrCat(
      {"([window.solana.createTransaction(new Uint8Array([", serialized_tx_str,
       "])), window.solana.createTransaction(new Uint8Array([",
       serialized_tx_str, "]))])"});
  auto result = EvalJs(web_contents(browser()), SignAllTransactionsScript(txs),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  // allow extra parameters
  const std::string txs2 =
      base::StrCat({"([window.solana.createTransaction(new Uint8Array([",
                    serialized_tx_str, "]))], 1234)"});
  auto result2 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript(txs2),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result3 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("()"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not array
  auto result4 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("({})"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // not entirely solanaWeb3.Transaction[]
  const std::string txs3 =
      base::StrCat({"([window.solana.createTransaction(new Uint8Array([",
                    serialized_tx_str, "])), 1234])"});
  auto result5 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("({})"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);
  auto result6 = EvalJs(web_contents(browser()), SignAllTransactionsScript(txs),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result6.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignAndSendTransaction) {
  const std::string serialized_tx_str = VectorToArrayString(kSerializedTx);
  const std::string tx =
      base::StrCat({"(window.solana.createTransaction(new Uint8Array([",
                    serialized_tx_str, "])))"});
  auto result =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(tx),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  // allow extra parameters
  const std::string tx2 = base::StrCat({"(", tx, ", {})"});
  auto result2 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(tx2),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result3 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript("()"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not solanaWeb3.Transaction
  auto result4 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript("('123')"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result5 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(tx),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result5.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignMessage) {
  const std::string msg_str = VectorToArrayString(kMessageToSign);
  const std::string msg = base::StrCat({"(new Uint8Array([", msg_str, "]))"});
  auto result = EvalJs(web_contents(browser()), SignMessageScript(msg),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result.value);

  // with display
  const std::string msg2 =
      base::StrCat({"(new Uint8Array([", msg_str, "], \"utf8\"))"});
  auto result2 = EvalJs(web_contents(browser()), SignMessageScript(msg2),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // allow extra parameters
  const std::string msg3 =
      base::StrCat({"(new Uint8Array([", msg_str, "], \"utf8\", 123))"});
  auto result3 = EvalJs(web_contents(browser()), SignMessageScript(msg2),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result3.value);

  // not Uint8Array
  const std::string msg4 = base::StrCat({"([", msg_str, "])"});
  auto result4 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(msg4),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // no arg
  auto result5 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript("()"),
             content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result6 = EvalJs(web_contents(browser()), SignMessageScript(msg),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result6.value);
}

// Request test here won't be testing params object, renderer just convert the
// object to dictionary and pass it to browser and it is resposibility of
// browser process to extract the info
IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Request) {
  const std::string request =
      base::StrCat({"(", GetRequstObject("connect"), ")"});
  auto result = EvalJs(web_contents(browser()), RequestScript(request),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  const std::string request2 =
      base::StrCat({"(", GetRequstObject("signAndSendTransaction"), ")"});
  auto result2 = EvalJs(web_contents(browser()), RequestScript(request2),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // allow extra parameters
  const std::string request3 =
      base::StrCat({"(", GetRequstObject("signTransaction"), ", 123)"});
  auto result3 = EvalJs(web_contents(browser()), RequestScript(request3),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result4 = EvalJs(web_contents(browser()), RequestScript("()"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // object without method
  auto result5 = EvalJs(web_contents(browser()), RequestScript("({})"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result6 = EvalJs(web_contents(browser()), RequestScript(request),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result6.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, OnAccountChanged) {
  auto result = EvalJs(web_contents(browser()), OnAccountChangedScript,
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetMainFrame());
  ASSERT_TRUE(provider);

  provider->SetEmitEmptyAccountChanged(true);

  auto result2 = EvalJs(web_contents(browser()), OnAccountChangedScript,
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(), result2.value);
}

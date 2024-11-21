/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/renderer/resource_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"

// IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST is excluded from Android build to
// save space. Ensure this test is not build on Android. If it will be required
// to run these tests on Android, include again
// IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST
static_assert(!BUILDFLAG(IS_ANDROID));

using brave_wallet::mojom::SolanaProviderError;

namespace {

static base::NoDestructor<std::string> g_provider_solana_web3_script("");

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
    R"(async function disconnect() {await window.braveSolana.disconnect()}
       new Promise(resolve => {
        window.braveSolana.on('accountChanged', (result) => {
        if (result instanceof Object)
          resolve(result.toString());
        else
          resolve(result);
        })
        disconnect();
      });
    )";

constexpr char CheckSolanaProviderScript[] = "!!window.braveSolana";
constexpr char OverwriteScript[] = "window.solana = ['test'];window.solana[0]";

std::string VectorToArrayString(const std::vector<uint8_t>& vec) {
  std::string result;
  for (size_t i = 0; i < vec.size(); ++i) {
    base::StrAppend(&result, {base::NumberToString(vec[i])});
    if (i != vec.size() - 1) {
      base::StrAppend(&result, {", "});
    }
  }
  return result;
}

std::string GetRequstObject(std::string_view method) {
  return content::JsReplace(R"({method: $1, params: {}})", method);
}

std::string NonWriteableScriptMethod(std::string_view provider,
                                     std::string_view method) {
  return absl::StrFormat(
      R"(new Promise(resolve => {
          window.%s.%s = "brave"
          if (typeof window.%s.%s === "function")
            resolve(true);
          else
            resolve(false);
          });
        )",
      provider, method, provider, method);
}

std::string NonWriteableScriptProperty(std::string_view provider,
                                       std::string_view property) {
  return absl::StrFormat(
      R"(new Promise(resolve => {
          window.%s.%s = "brave"
          if (window.%s.%s === "brave")
            resolve(false)
          else
            resolve(true)
          });
        )",
      provider, property, provider, property);
}

std::string NonConfigurableScript(std::string_view provider) {
  return absl::StrFormat(
      R"(try {
         Object.defineProperty(window, '%s', {
           writable: true,
         });
       } catch (e) {}
       window.%s = 42;
       typeof window.%s === 'object'
        )",
      provider, provider, provider);
}

std::string ConnectScript(std::string_view args) {
  return absl::StrFormat(
      R"(async function connect() {
          try {
            const result = await window.braveSolana.connect(%s);
            return result.publicKey.toString();
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        connect();)",
      args);
}

std::string CreateTransactionScript(const std::vector<uint8_t>& serialized_tx) {
  const std::string serialized_tx_str = VectorToArrayString(serialized_tx);
  return absl::StrFormat(
      R"((function() {
          %s
          return solanaWeb3.Transaction.from(new Uint8Array([%s]))
         })())",
      *g_provider_solana_web3_script, serialized_tx_str);
}

std::string SignTransactionScript(std::string_view args) {
  const std::string signed_tx = VectorToArrayString(kSignedTx);
  return absl::StrFormat(
      R"(async function signTransaction() {
          try {
            const result = await window.braveSolana.signTransaction%s
            if (result.serialize().join() === new Uint8Array([%s]).join())
              return true;
            else
              return false;
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        signTransaction();)",
      args, signed_tx);
}

std::string SignAllTransactionsScript(std::string_view args) {
  const std::string signed_tx = VectorToArrayString(kSignedTx);
  return absl::StrFormat(
      R"(async function signAllTransactions() {
          try {
            const result = await window.braveSolana.signAllTransactions%s
            const isSameTx =
              (tx) => tx.serialize().join() === new Uint8Array([%s]).join()
            if (result.every(isSameTx))
              return true;
            else
              return false;
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        signAllTransactions();)",
      args, signed_tx);
}

std::string SignAndSendTransactionScript(std::string_view args) {
  const std::string expected_result = content::JsReplace(
      R"({ publicKey: $1, signature: $2})", kTestPublicKey, kTestSignature);
  return absl::StrFormat(
      R"(async function signAndSendTransaction() {
          try {
            const result = await window.braveSolana.signAndSendTransaction%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              return true;
            else
              return false;
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        signAndSendTransaction();)",
      args, expected_result);
}

std::string SignMessageScript(std::string_view args) {
  std::vector<uint8_t> signature(brave_wallet::kSolanaSignatureSize);
  EXPECT_TRUE(
      brave_wallet::Base58Decode(kTestSignature, &signature, signature.size()));
  const std::string signature_str = VectorToArrayString(signature);
  const std::string expected_result =
      absl::StrFormat(R"({ publicKey: "%s", signature: new Uint8Array([%s])})",
                      kTestPublicKey, signature_str);
  return absl::StrFormat(
      R"(async function signMessage() {
          try {
            const result = await window.braveSolana.signMessage%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              return true;
            else
              return false;
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        signMessage();)",
      args, expected_result);
}

std ::string RequestScript(std::string_view args) {
  const std::string expected_result = content::JsReplace(
      R"({ publicKey: $1, signature: $2})", kTestPublicKey, kTestSignature);
  return absl::StrFormat(
      R"(async function request() {
          try {
            const result = await window.braveSolana.request%s
            if (JSON.stringify(result) === JSON.stringify(%s))
              return true;
            else if (result.publicKey)
              return result.publicKey.toString();
            else
              return false;
          } catch (err) {
            return err.message + (err.code ?? "");
          }
        }
        request();)",
      args, expected_result);
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
  void Connect(std::optional<base::Value::Dict> arg,
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
    if (emit_empty_account_changed_) {
      events_listener_->AccountChangedEvent(std::nullopt);
    } else {
      events_listener_->AccountChangedEvent(kTestPublicKey);
    }
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
  void SignTransaction(brave_wallet::mojom::SolanaSignTransactionParamPtr param,
                       SignTransactionCallback callback) override {
    EXPECT_EQ(param->encoded_serialized_msg,
              brave_wallet::Base58Encode(kSerializedMessage));
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(
          SolanaProviderError::kSuccess, "", kSignedTx,
          brave_wallet::mojom::SolanaMessageVersion::kLegacy);
    } else {
      std::move(callback).Run(
          error_, error_message_, std::vector<uint8_t>(),
          brave_wallet::mojom::SolanaMessageVersion::kLegacy);
      ClearError();
    }
  }
  void SignAllTransactions(
      std::vector<brave_wallet::mojom::SolanaSignTransactionParamPtr> params,
      SignAllTransactionsCallback callback) override {
    for (const auto& param : params) {
      EXPECT_EQ(param->encoded_serialized_msg,
                brave_wallet::Base58Encode(kSerializedMessage));
    }
    if (error_ == SolanaProviderError::kSuccess) {
      std::move(callback).Run(
          SolanaProviderError::kSuccess, "", {kSignedTx, kSignedTx},
          {brave_wallet::mojom::SolanaMessageVersion::kLegacy,
           brave_wallet::mojom::SolanaMessageVersion::kLegacy});
    } else {
      std::move(callback).Run(
          error_, error_message_, std::vector<std::vector<uint8_t>>(),
          std::vector<brave_wallet::mojom::SolanaMessageVersion>());
      ClearError();
    }
  }
  void SignAndSendTransaction(
      brave_wallet::mojom::SolanaSignTransactionParamPtr param,
      std::optional<base::Value::Dict> send_options,
      SignAndSendTransactionCallback callback) override {
    EXPECT_EQ(param->encoded_serialized_msg,
              brave_wallet::Base58Encode(kSerializedMessage));

    auto expect_send_options = base::JSONReader::Read(
        R"({"maxRetries": 9007199254740991,
            "preflightCommitment": "confirmed",
            "skipPreflight": true})");
    ASSERT_TRUE(expect_send_options);
    EXPECT_EQ(send_options, send_options_);

    base::Value::Dict result;
    if (error_ == SolanaProviderError::kSuccess) {
      result.Set("publicKey", kTestPublicKey);
      result.Set("signature", kTestSignature);
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              std::move(result));
    } else {
      std::move(callback).Run(error_, error_message_, std::move(result));
      ClearError();
    }
  }
  void SignMessage(const std::vector<uint8_t>& blob_msg,
                   const std::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override {
    EXPECT_EQ(blob_msg, kMessageToSign);
    base::Value::Dict result;
    if (error_ == SolanaProviderError::kSuccess) {
      result.Set("publicKey", kTestPublicKey);
      result.Set("signature", kTestSignature);
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              std::move(result));
    } else {
      std::move(callback).Run(error_, error_message_, std::move(result));
      ClearError();
    }
  }
  void Request(base::Value::Dict arg, RequestCallback callback) override {
    base::Value::Dict result;
    if (error_ == SolanaProviderError::kSuccess) {
      result.Set("publicKey", kTestPublicKey);
      result.Set("signature", kTestSignature);
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

  void SetSendOptions(std::optional<base::Value::Dict> options) {
    send_options_ = std::move(options);
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
  std::optional<base::Value::Dict> send_options_;
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
    if (!provider_map_.contains(frame_host->GetGlobalId())) {
      return nullptr;
    }
    return static_cast<TestSolanaProvider*>(
        provider_map_.at(frame_host->GetGlobalId())->impl());
  }
  bool WaitForBinding(content::RenderFrameHost* render_frame_host,
                      base::OnceClosure callback) {
    if (IsBound(render_frame_host)) {
      return false;
    }
    quit_on_binding_ = std::move(callback);
    return true;
  }
  bool IsBound(content::RenderFrameHost* frame_host) {
    return provider_map_.contains(frame_host->GetGlobalId());
  }

 private:
  void BindSolanaProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<brave_wallet::mojom::SolanaProvider> receiver) {
    auto provider = mojo::MakeSelfOwnedReceiver(
        std::make_unique<TestSolanaProvider>(), std::move(receiver));
    provider->set_connection_error_handler(base::BindOnce(
        &TestBraveContentBrowserClient::OnDisconnect,
        weak_ptr_factory_.GetWeakPtr(), frame_host->GetGlobalId()));
    provider_map_[frame_host->GetGlobalId()] = provider;
    if (quit_on_binding_) {
      std::move(quit_on_binding_).Run();
    }
  }
  void OnDisconnect(content::GlobalRenderFrameHostId frame_host_id) {
    provider_map_.erase(frame_host_id);
  }

  base::OnceClosure quit_on_binding_;
  base::flat_map<
      content::GlobalRenderFrameHostId,
      mojo::SelfOwnedReceiverRef<brave_wallet::mojom::SolanaProvider>>
      provider_map_;
  base::WeakPtrFactory<TestBraveContentBrowserClient> weak_ptr_factory_{this};
};

}  // namespace

class SolanaProviderRendererTest : public InProcessBrowserTest {
 public:
  SolanaProviderRendererTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    brave_wallet::SetDefaultSolanaWallet(
        browser()->profile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
    content::SetBrowserClientForTesting(&test_content_browser_client_);
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());
    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());

    // This is intentional to trigger
    // TestBraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://settings")));

    GURL url = embedded_test_server()->GetURL("/empty.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

    ASSERT_TRUE(base::FeatureList::IsEnabled(
        brave_wallet::features::kNativeBraveWalletFeature));

    // load solana web3 script
    if (g_provider_solana_web3_script->empty()) {
      *g_provider_solana_web3_script = brave_wallet::LoadDataResource(
          IDR_BRAVE_WALLET_SOLANA_WEB3_JS_FOR_TEST);
    }
  }

  content::WebContents* web_contents(Browser* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void ReloadAndWaitForLoadStop(Browser* browser) {
    chrome::Reload(browser, WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents(browser)));
  }

  brave_wallet::KeyringService* GetKeyringService() {
    return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
               browser()->profile())
        ->keyring_service();
  }

 protected:
  net::EmbeddedTestServer https_server_;
  TestBraveContentBrowserClient test_content_browser_client_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, Incognito) {
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  GURL url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(private_browser, url));

  auto result =
      EvalJs(web_contents(private_browser), CheckSolanaProviderScript);
  EXPECT_EQ(base::Value(false), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, DefaultWallet) {
  auto result = EvalJs(web_contents(browser()), CheckSolanaProviderScript);

  EXPECT_EQ(base::Value(true), result.value);
  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);
  ReloadAndWaitForLoadStop(browser());
  auto result2 = EvalJs(web_contents(browser()), CheckSolanaProviderScript);
  EXPECT_EQ(base::Value(false), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, ExtensionOverwrite) {
  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  ReloadAndWaitForLoadStop(browser());
  // can't be overwritten
  EXPECT_EQ(content::EvalJs(web_contents(browser()), OverwriteScript).error,
            "");
  ASSERT_TRUE(
      content::EvalJs(web_contents(browser()), "window.solana.isPhantom")
          .ExtractBool());

  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop(browser());
  // overwritten
  EXPECT_EQ(
      content::EvalJs(web_contents(browser()), OverwriteScript).ExtractString(),
      "test");
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest,
                       AttachEvenIfNoWalletCreated) {
  GetKeyringService()->Reset(false);

  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop(browser());

  constexpr char kEvalIsBraveWallet[] = "window.solana.isBraveWallet";
  EXPECT_TRUE(content::EvalJs(web_contents(browser())->GetPrimaryMainFrame(),
                              kEvalIsBraveWallet)
                  .ExtractBool());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, AttachIfWalletCreated) {
  GetKeyringService()->CreateWallet("password", base::DoNothing());

  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension);
  ReloadAndWaitForLoadStop(browser());

  constexpr char kEvalIsBraveWallet[] = "window.solana.isBraveWallet";
  EXPECT_TRUE(content::EvalJs(web_contents(browser())->GetPrimaryMainFrame(),
                              kEvalIsBraveWallet)
                  .ExtractBool());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}
IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, NonWritable) {
  for (const std::string& provider : {"braveSolana", "solana"}) {
    // window.braveSolana.* and window.solana.* (methods)
    for (const std::string& method :
         {"on", "off", "emit", "removeListener", "removeAllListeners",
          "connect", "disconnect", "signAndSendTransaction", "signMessage",
          "request", "signTransaction", "signAllTransactions",
          "walletStandardInit"}) {
      SCOPED_TRACE(method);
      auto result = EvalJs(web_contents(browser()),
                           NonWriteableScriptMethod(provider, method));
      EXPECT_EQ(base::Value(true), result.value) << result.error;
    }
    // window.braveSolana.* and window.solana.* (properties)
    for (const std::string& property :
         {"isPhantom", "isBraveWallet", "isConnected", "publicKey"}) {
      SCOPED_TRACE(property);
      auto result = EvalJs(web_contents(browser()),
                           NonWriteableScriptProperty(provider, property));
      EXPECT_EQ(base::Value(true), result.value) << result.error;
    }
  }
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, IsPhantomAndIsBraveWallet) {
  ASSERT_TRUE(ExecJs(web_contents(browser()),
                     "window.braveSolana.isPhantom = 123; "
                     "window.braveSolana.isBraveWallet = 456"));
  // Both are non-writable
  auto result1 =
      EvalJs(web_contents(browser()), "window.braveSolana.isPhantom");
  EXPECT_EQ(base::Value(true), result1.value);
  auto result2 =
      EvalJs(web_contents(browser()), "window.braveSolana.isBraveWallet");
  EXPECT_EQ(base::Value(true), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, Connect) {
  for (const std::string& valid_case : {
           "",
           "{}, 123",    // allow extra parameters
           "undefined",  // allow optional params to be undefined
           "null",       // allow optional params to be null
           "undefined, 123",
           "null, 123",
       }) {
    SCOPED_TRACE(valid_case);
    auto result = EvalJs(web_contents(browser()), ConnectScript(valid_case));
    EXPECT_EQ(base::Value(kTestPublicKey), result.value);
  }

  // non object args
  auto result2 = EvalJs(web_contents(browser()), ConnectScript("123"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result2.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result3 = EvalJs(web_contents(browser()), ConnectScript(""));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result3.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, OnConnect) {
  auto result =
      EvalJs(web_contents(browser()),
             R"(async function connect() {await window.braveSolana.connect()}
                new Promise(resolve => {
                  window.braveSolana.on(
                    'connect', (key) => resolve(key.toString()));
                  connect();
                });
              )");
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, IsConnected) {
  auto result =
      EvalJs(web_contents(browser()), "window.braveSolana.isConnected");
  EXPECT_EQ(base::Value(true), result.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  // just make TestSolanaProvider::IsConnected to return false
  provider->SetError(SolanaProviderError::kUserRejectedRequest, "");

  auto result2 =
      EvalJs(web_contents(browser()), "window.braveSolana.isConnected");
  EXPECT_EQ(base::Value(false), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, GetPublicKey) {
  auto result = EvalJs(web_contents(browser()),
                       "window.braveSolana.publicKey.toString()");
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, Disconnect) {
  auto result = EvalJs(web_contents(browser()),
                       R"(async function disconnect() {
                  const result = await window.braveSolana.disconnect()
                  if (result == undefined)
                    return true;
                  else
                    return false;
                }
                disconnect();)");
  EXPECT_EQ(base::Value(true), result.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, SignTransaction) {
  const std::string tx =
      base::StrCat({"(", CreateTransactionScript(kSerializedTx), ")"});
  auto result = EvalJs(web_contents(browser()), SignTransactionScript(tx));
  EXPECT_EQ(base::Value(true), result.value);

  // allow extra parameters
  const std::string tx2 = base::StrCat({"(", tx, ", {})"});
  auto result2 = EvalJs(web_contents(browser()), SignTransactionScript(tx2));
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result3 = EvalJs(web_contents(browser()), SignTransactionScript("()"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not solanaWeb3.Transaction
  auto result4 =
      EvalJs(web_contents(browser()), SignTransactionScript("('123')"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result5 = EvalJs(web_contents(browser()), SignTransactionScript(tx));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result5.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, SignAllTransactions) {
  const std::string txs =
      base::StrCat({"([", CreateTransactionScript(kSerializedTx), ",",
                    CreateTransactionScript(kSerializedTx), "])"});
  auto result = EvalJs(web_contents(browser()), SignAllTransactionsScript(txs));
  EXPECT_EQ(base::Value(true), result.value);

  // allow extra parameters
  const std::string txs2 =
      base::StrCat({"([", CreateTransactionScript(kSerializedTx), "], 1234)"});
  auto result2 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript(txs2));
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result3 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("()"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not array
  auto result4 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("({})"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // not entirely solanaWeb3.Transaction[]
  const std::string txs3 =
      base::StrCat({"([", CreateTransactionScript(kSerializedTx), ", 1234])"});
  auto result5 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript("({})"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);
  auto result6 =
      EvalJs(web_contents(browser()), SignAllTransactionsScript(txs));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result6.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, SignAndSendTransaction) {
  const std::string send_options =
      R"({"maxRetries": 9007199254740991,
          "preflightCommitment": "confirmed",
          "skipPreflight": true})";
  const std::string tx_with_send_options = base::StrCat(
      {"(", CreateTransactionScript(kSerializedTx), ",", send_options, ")"});

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);
  provider->SetSendOptions(
      base::JSONReader::Read(send_options)->GetDict().Clone());

  auto send_options_result =
      EvalJs(web_contents(browser()),
             SignAndSendTransactionScript(tx_with_send_options));
  EXPECT_EQ(base::Value(true), send_options_result.value);

  provider->SetSendOptions(std::nullopt);
  const std::string tx =
      base::StrCat({"(", CreateTransactionScript(kSerializedTx), ")"});

  for (const std::string& valid_case :
       {tx, base::StrCat({"(", tx, ", undefined, {})"}),
        base::StrCat({"(", tx, ", null, {})"})}) {
    SCOPED_TRACE(valid_case);
    auto result = EvalJs(web_contents(browser()),
                         SignAndSendTransactionScript(valid_case));
    EXPECT_EQ(base::Value(true), result.value);
  }

  // allow extra parameters
  provider->SetSendOptions(base::Value::Dict());
  const std::string tx2 = base::StrCat({"(", tx, ", {}, {})"});
  auto result2 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(tx2));
  EXPECT_EQ(base::Value(true), result2.value);
  provider->SetSendOptions(std::nullopt);

  // no arg
  auto result3 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript("()"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  // not solanaWeb3.Transaction
  auto result4 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript("('123')"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result5 =
      EvalJs(web_contents(browser()), SignAndSendTransactionScript(tx));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result5.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, SignMessage) {
  const std::string msg_str = VectorToArrayString(kMessageToSign);
  const std::string msg = base::StrCat({"(new Uint8Array([", msg_str, "]))"});
  for (const std::string& valid_case :
       {msg,
        base::StrCat(
            {"(new Uint8Array([", msg_str, "], \"utf8\"))"}),  // with_display
        base::StrCat({"(new Uint8Array([", msg_str,
                      "], \"utf8\", 123))"}),  // allow extra parameters
        base::StrCat({"(new Uint8Array([", msg_str,
                      "], undefined))"}),  // with_display is undefined
        base::StrCat({"(new Uint8Array([", msg_str,
                      "], null))"}),  // with_display is null
        base::StrCat({"(new Uint8Array([", msg_str, "], undefined, 123))"}),
        base::StrCat({"(new Uint8Array([", msg_str, "], null, 123))"})}) {
    SCOPED_TRACE(valid_case);
    auto result =
        EvalJs(web_contents(browser()), SignMessageScript(valid_case));
    EXPECT_EQ(base::Value(true), result.value);
  }

  // not Uint8Array
  const std::string msg4 = base::StrCat({"([", msg_str, "])"});
  auto result4 = EvalJs(web_contents(browser()), SignMessageScript(msg4));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // no arg
  auto result5 = EvalJs(web_contents(browser()), SignMessageScript("()"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  // display is not string, use default utf8 encoding
  const std::string msg6 =
      base::StrCat({"(new Uint8Array([", msg_str, "], 12345))"});
  auto result6 = EvalJs(web_contents(browser()), SignMessageScript(msg6));
  EXPECT_EQ(base::Value(true), result6.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result7 = EvalJs(web_contents(browser()), SignMessageScript(msg));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result7.value);
}

// Request test here won't be testing params object, renderer just convert the
// object to dictionary and pass it to browser and it is resposibility of
// browser process to extract the info
IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, Request) {
  const std::string request =
      base::StrCat({"(", GetRequstObject("connect"), ")"});
  auto result = EvalJs(web_contents(browser()), RequestScript(request));
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  const std::string request2 =
      base::StrCat({"(", GetRequstObject("signAndSendTransaction"), ")"});
  auto result2 = EvalJs(web_contents(browser()), RequestScript(request2));
  EXPECT_EQ(base::Value(true), result2.value);

  // allow extra parameters
  const std::string request3 =
      base::StrCat({"(", GetRequstObject("signTransaction"), ", 123)"});
  auto result3 = EvalJs(web_contents(browser()), RequestScript(request3));
  EXPECT_EQ(base::Value(true), result2.value);

  // no arg
  auto result4 = EvalJs(web_contents(browser()), RequestScript("()"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result4.value);

  // object without method
  auto result5 = EvalJs(web_contents(browser()), RequestScript("({})"));
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result5.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result6 = EvalJs(web_contents(browser()), RequestScript(request));
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result6.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, OnAccountChanged) {
  auto result = EvalJs(web_contents(browser()), OnAccountChangedScript);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  TestSolanaProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ASSERT_TRUE(provider);

  provider->SetEmitEmptyAccountChanged(true);

  auto result2 = EvalJs(web_contents(browser()), OnAccountChangedScript);
  EXPECT_EQ(base::Value(), result2.value);
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, NonConfigurable) {
  brave_wallet::SetDefaultSolanaWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::BraveWallet);
  GURL url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(content::EvalJs(web_contents(browser()),
                              NonConfigurableScript("braveSolana"))
                  .ExtractBool());
  EXPECT_TRUE(
      content::EvalJs(web_contents(browser()), NonConfigurableScript("solana"))
          .ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, Iframe3P) {
  constexpr char kEvalSolanaUndefined[] =
      R"(typeof window.braveSolana === 'undefined')";

  GURL secure_top_url(https_server_.GetURL("a.com", "/iframe.html"));
  GURL insecure_top_url =
      embedded_test_server()->GetURL("a.com", "/iframe.html");
  GURL data_top_url = GURL(
      "data:text/html;,<html><body><iframe id='test'></iframe></body></html>");
  GURL iframe_url_1p(https_server_.GetURL("a.com", "/simple.html"));
  GURL iframe_url_3p(https_server_.GetURL("b.a.com", "/simple.html"));
  GURL data_simple_url = GURL("data:text/html;,<html><body></body></html>");

  const struct {
    std::string script;
    GURL top_url;
    GURL iframe_url;
  } solana_undefined_cases[] =
      {{// 3p iframe
        "true", secure_top_url, iframe_url_3p},
       {// 1st party iframe with allow="solana 'none'"
        R"(
        document.querySelector('iframe').setAttribute('allow', 'solana \'none\'');
        true
        )",
        secure_top_url, iframe_url_1p},
       {// 1st party iframe with sandbox="allow-scripts"
        R"(
        document.querySelector('iframe').removeAttribute('allow');
        document.querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
        true
        )",
        secure_top_url, iframe_url_1p},
       {// 3p iframe with sandbox="allow-scripts allow-same-origin"
        R"(
        document.querySelector('iframe').removeAttribute('allow');
        document.querySelector('iframe')
          .setAttribute('sandbox', 'allow-scripts allow-same-origin');
        true
        )",
        secure_top_url, iframe_url_3p},
       {// 3p iframe with allow="ethereum"
        R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe').setAttribute('allow', 'ethereum');
        true
        )",
        secure_top_url, iframe_url_3p},

       {// 3p iframe with allow="solana; ethereum" but insecure top level
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        insecure_top_url, iframe_url_3p},

       {// 3p iframe with allow="solana; ethereum" but insecure top level (data
        // URI)
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        data_top_url, iframe_url_3p},

       {// 3p iframe with allow="solana; ethereum" but insecure iframe
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        secure_top_url, data_simple_url},
       {// insecure top level and insecure iframe allow="solana; ethereum"
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'solana; ethereum');
      true
      )",
        data_top_url, data_simple_url}},
    solana_defined_cases[] = {
        {// 1st party iframe
         "true", secure_top_url, iframe_url_1p},
        {// 1st party iframe sandbox="allow-scripts allow-same-origin"
         R"(
      document.querySelector('iframe').removeAttribute('allow');
      document.querySelector('iframe')
          .setAttribute('sandbox', 'allow-scripts allow-same-origin');
      true
      )",
         secure_top_url, iframe_url_1p},
        {// 3p iframe with allow="solana"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow', 'solana');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3p iframe with allow="ethereum; solana"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow', 'ethereum; solana');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3rd party iframe with sandbox="allow-scripts" allow="solana"
         R"(
      document.querySelector('iframe').setAttribute('allow', 'solana');
      document.querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
      true
      )",
         secure_top_url, iframe_url_3p}};

  for (auto& c : solana_undefined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    EXPECT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    EXPECT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_TRUE(
        content::EvalJs(ChildFrameAt(main_frame, 0), kEvalSolanaUndefined)
            .ExtractBool());
  }
  for (auto& c : solana_defined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.top_url << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    EXPECT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    EXPECT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_FALSE(
        content::EvalJs(ChildFrameAt(main_frame, 0), kEvalSolanaUndefined)
            .ExtractBool());
  }
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest, SecureContextOnly) {
  // Secure context HTTPS server
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  constexpr char kEvalSolana[] = "typeof window.braveSolana !== 'undefined'";
  content::RenderFrameHost* main_frame =
      web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(content::EvalJs(main_frame, kEvalSolana).ExtractBool());

  // Insecure context
  url = embedded_test_server()->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_FALSE(content::EvalJs(main_frame, kEvalSolana).ExtractBool());

  // Secure context localhost HTTP
  url = embedded_test_server()->GetURL("localhost", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(content::EvalJs(main_frame, kEvalSolana).ExtractBool());

  // Secure context 127.0.0.1 HTTP
  url = embedded_test_server()->GetURL("localhost", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(content::EvalJs(main_frame, kEvalSolana).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SolanaProviderRendererTest,
                       SolanaWeb3PrototypePollution) {
  ASSERT_TRUE(ExecJs(web_contents(browser()), "Object.freeze = ()=>{}"));
  auto result = EvalJs(web_contents(browser()), ConnectScript(""));
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);
}

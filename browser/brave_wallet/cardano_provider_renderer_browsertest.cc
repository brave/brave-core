/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

namespace {

using testing::_;

constexpr char kCheckCardanoProviderScript[] =
    "!!window.cardano && !!window.cardano.brave";
constexpr char kOverwriteCardanoScript[] =
    "window.cardano = ['test']; window.cardano[0]";
constexpr char kOverwriteCardanoBraveScript[] =
    "window.cardano.brave = ['test']; window.cardano.brave[0]";
constexpr char kExtensionWallet[] = "window.cardano.somewallet = ['test'];";
constexpr char kCheckExtensionWallet[] =
    "!!window.cardano && !!window.cardano.somewallet";

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

std::string NonWriteableScriptApiMethod(std::string_view method) {
  return absl::StrFormat(
      R"(async function check() {
          let x = await window.cardano.brave.enable()
          x.%s = "brave"
          if (typeof x.%s === "function")
            return true;
          else
            return false;
          }
          check();
        )",
      method, method);
}

std::string EnableScript() {
  return
      R"(async function connect() {
          try {
            const result = await window.cardano.brave.enable();
            return true;
          } catch (err) {
            return false;
          }
        }
        connect();)";
}

std::string NonWriteableScriptProperty(const std::string& property) {
  return absl::StrFormat(
      R"(window.cardano.brave.%s = "brave";
         !(window.cardano.brave.%s === "brave");)",
      property, property);
}

class TestCardanoProvider : public brave_wallet::mojom::CardanoProvider {
 public:
  TestCardanoProvider() = default;
  ~TestCardanoProvider() override = default;

  MOCK_METHOD2(Enable,
               void(mojo::PendingReceiver<mojom::CardanoApi>, EnableCallback));
  MOCK_METHOD1(IsEnabled, void(IsEnabledCallback));

  void BindReceiver(
      mojo::PendingReceiver<brave_wallet::mojom::CardanoProvider> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  mojo::ReceiverSet<brave_wallet::mojom::CardanoProvider> receivers_;
};

class TestCardanoApi : public brave_wallet::mojom::CardanoApi {
 public:
  TestCardanoApi() = default;
  ~TestCardanoApi() override = default;

  MOCK_METHOD1(GetNetworkId, void(GetNetworkIdCallback));
  MOCK_METHOD1(GetUsedAddresses, void(GetUsedAddressesCallback callback));
  MOCK_METHOD1(GetUnusedAddresses, void(GetUnusedAddressesCallback callback));
  MOCK_METHOD1(GetChangeAddress, void(GetChangeAddressCallback callback));
  MOCK_METHOD1(GetRewardAddresses, void(GetRewardAddressesCallback callback));
  MOCK_METHOD1(GetBalance, void(GetBalanceCallback callback));
  MOCK_METHOD3(GetUtxos,
               void(const std::optional<std::string>& amount,
                    mojom::CardanoProviderPaginationPtr paginate,
                    GetUtxosCallback callback));
  MOCK_METHOD3(SignTx,
               void(const std::string& tx_cbor,
                    bool partial_sign,
                    SignTxCallback callback));
  MOCK_METHOD2(SubmitTx,
               void(const std::string& signed_tx_cbor,
                    SubmitTxCallback callback));
  MOCK_METHOD3(SignData,
               void(const std::string& address,
                    const std::string& payload_hex,
                    SignDataCallback callback));
  MOCK_METHOD2(GetCollateral,
               void(const std::string& amount, GetCollateralCallback callback));

  void BindReceiver(
      mojo::PendingReceiver<brave_wallet::mojom::CardanoApi> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  mojo::ReceiverSet<brave_wallet::mojom::CardanoApi> receivers_;
};

class TestBraveContentBrowserClient : public BraveContentBrowserClient {
 public:
  TestBraveContentBrowserClient() {
    provider_ = std::make_unique<TestCardanoProvider>();
  }

  ~TestBraveContentBrowserClient() override = default;
  TestBraveContentBrowserClient(const TestBraveContentBrowserClient&) = delete;
  TestBraveContentBrowserClient& operator=(
      const TestBraveContentBrowserClient&) = delete;

  void RegisterBrowserInterfaceBindersForFrame(
      content::RenderFrameHost* render_frame_host,
      mojo::BinderMapWithContext<content::RenderFrameHost*>* map) override {
    BraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame(
        render_frame_host, map);
    // override binding for CardanoProvider
    map->Add<brave_wallet::mojom::CardanoProvider>(
        base::BindRepeating(&TestBraveContentBrowserClient::BindCardanoProvider,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  TestCardanoProvider* GetProvider(content::RenderFrameHost* frame_host) {
    return provider_.get();
  }

 private:
  void BindCardanoProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<brave_wallet::mojom::CardanoProvider> receiver) {
    provider_->BindReceiver(std::move(receiver));
  }

  std::unique_ptr<TestCardanoProvider> provider_;
  base::WeakPtrFactory<TestBraveContentBrowserClient> weak_ptr_factory_{this};
};

}  // namespace

class CardanoProviderRendererTest : public InProcessBrowserTest {
 public:
  CardanoProviderRendererTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletCardanoFeature,
          {{"cardano_dapp_support", "true"}}}},
        {});
  }

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
    content::SetBrowserClientForTesting(&test_content_browser_client_);
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
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
  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

class CardanoProviderDisabledRendererTest : public CardanoProviderRendererTest {
 public:
  CardanoProviderDisabledRendererTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletCardanoFeature,
          {{"cardano_dapp_support", "false"}}}},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(CardanoProviderDisabledRendererTest,
                       NotAttached_FeatureDisabled) {
  ReloadAndWaitForLoadStop(browser());

  auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(false), result);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, Incognito) {
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  GURL url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(private_browser, url));

  auto result =
      EvalJs(web_contents(private_browser), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, ExtensionWallet) {
  ReloadAndWaitForLoadStop(browser());
  // Add a new wallet along with the brave one
  EXPECT_TRUE(
      content::EvalJs(web_contents(browser()), kExtensionWallet).is_ok());
  auto result = EvalJs(web_contents(browser()), kCheckExtensionWallet);
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, ExtensionOverwriteCardano) {
  ReloadAndWaitForLoadStop(browser());
  // can't be overwritten
  EXPECT_TRUE(content::EvalJs(web_contents(browser()), kOverwriteCardanoScript)
                  .is_ok());
  auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest,
                       ExtensionOverwriteCardanoBrave) {
  ReloadAndWaitForLoadStop(browser());
  // can't be overwritten
  EXPECT_TRUE(
      content::EvalJs(web_contents(browser()), kOverwriteCardanoBraveScript)
          .is_ok());
  {
    auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
    EXPECT_EQ(base::Value(true), result);
  }

  {
    auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
    EXPECT_EQ(base::Value(true), result);
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, Properties) {
  ReloadAndWaitForLoadStop(browser());

  {
    auto result = EvalJs(web_contents(browser()), "window.cardano.brave.name");
    EXPECT_EQ(base::Value("Brave"), result);
  }

  {
    auto result = EvalJs(web_contents(browser()),
                         "window.cardano.brave.supportedExtensions");
    EXPECT_EQ(base::Value::List(), result);
  }

  {
    auto result = EvalJs(web_contents(browser()), "window.cardano.brave.icon");
    EXPECT_TRUE(result.ExtractString().starts_with("data:image/png;base64,"));
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest,
                       AttachEvenIfNoWalletCreated) {
  GetKeyringService()->Reset(false);

  ReloadAndWaitForLoadStop(browser());

  auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(true), result);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, AttachIfWalletCreated) {
  GetKeyringService()->CreateWallet("password", base::DoNothing());

  ReloadAndWaitForLoadStop(browser());

  auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(true), result);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, NonWritableCardanoBrave) {
  {
    auto result = EvalJs(web_contents(browser()),
                         NonWriteableScriptMethod("cardano.brave", "enable"));
    EXPECT_EQ(base::Value(true), result) << result;
  }

  {
    auto result =
        EvalJs(web_contents(browser()),
               NonWriteableScriptMethod("cardano.brave", "isEnabled"));
    EXPECT_EQ(base::Value(true), result) << result;
  }

  // window.cardano.brave.* (properties)
  for (const std::string& property : {"name", "supportedExtensions", "icon"}) {
    SCOPED_TRACE(property);
    auto result =
        EvalJs(web_contents(browser()), NonWriteableScriptProperty(property));
    EXPECT_EQ(base::Value(true), result) << result;
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest,
                       NonWritableCardanoWalletApi) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(nullptr);
      });
  for (const std::string& method :
       {"getNetworkId", "getUsedAddresses", "getUnusedAddresses",
        "getChangeAddress", "getRewardAddresses", "getUtxos", "getBalance",
        "signTx", "signData", "submitTx", "getExtensions", "getCollateral"}) {
    SCOPED_TRACE(method);
    auto result =
        EvalJs(web_contents(browser()), NonWriteableScriptApiMethod(method));
    EXPECT_EQ(base::Value(true), result) << result;
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, EnableSuccess) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(nullptr);
      });

  auto result = EvalJs(web_contents(browser()), EnableScript());
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, EnableFail) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(
            mojom::CardanoProviderErrorBundle::New(-3, "Refused", nullptr));
      });

  auto result =
      content::EvalJs(web_contents(browser()), R"(async function connect() {
          try {
            return await window.cardano.brave.enable();
          } catch (err) {
            return err;
          }
        }
        connect();)");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-3));
  error_value.Set("info", "Refused");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, IsEnabled) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, IsEnabled(_))
      .WillByDefault([&](TestCardanoProvider::IsEnabledCallback callback) {
        std::move(callback).Run(true);
      });

  auto result = EvalJs(
      web_contents(browser()),
      "(async () => { return await window.cardano.brave.isEnabled()})()");
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, NotIsEnabled) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, IsEnabled(_))
      .WillByDefault([&](TestCardanoProvider::IsEnabledCallback callback) {
        std::move(callback).Run(false);
      });

  auto result = EvalJs(
      web_contents(browser()),
      "(async () => { return await window.cardano.brave.isEnabled()})()");
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetNetworkId) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetNetworkId(_))
      .WillByDefault([&](TestCardanoApi::GetNetworkIdCallback callback) {
        std::move(callback).Run(1, nullptr);
      });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).getNetworkId() })();");
  EXPECT_EQ(base::Value(1), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetNetworkId_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetNetworkId(_))
      .WillByDefault([&](TestCardanoApi::GetNetworkIdCallback callback) {
        std::move(callback).Run(
            0, mojom::CardanoProviderErrorBundle::New(-1, "Invalid", nullptr));
      });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).getNetworkId() } "
                       "catch(err){return err;}})();");
  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-1));
  error_value.Set("info", "Invalid");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUsedAddresses) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUsedAddresses(_))
      .WillByDefault([&](TestCardanoApi::GetUsedAddressesCallback callback) {
        std::vector<std::string> result{"1", "2"};
        std::move(callback).Run(result, nullptr);
      });
  auto result =
      EvalJs(web_contents(browser()),
             "(async () => { return await (await "
             "window.cardano.brave.enable()).getUsedAddresses() })();");
  base::Value::List list_value;
  list_value.Append(base::Value("1"));
  list_value.Append(base::Value("2"));

  EXPECT_EQ(list_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUsedAddresses_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUsedAddresses(_))
      .WillByDefault(
          [&](TestCardanoApi::GetUsedAddressesCallback callback) {
            std::move(callback).Run({}, mojom::CardanoProviderErrorBundle::New(
                                            -4, "Account change", nullptr));
          });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).getUsedAddresses() } "
                       "catch(err) {return err;}})();");
  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-4));
  error_value.Set("info", "Account change");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUnusedAddresses) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUnusedAddresses(_))
      .WillByDefault([&](TestCardanoApi::GetUnusedAddressesCallback callback) {
        std::vector<std::string> result{"1", "2"};
        std::move(callback).Run(result, nullptr);
      });
  auto result =
      EvalJs(web_contents(browser()),
             "(async () => { return await (await "
             "window.cardano.brave.enable()).getUnusedAddresses() })();");
  base::Value::List list_value;
  list_value.Append(base::Value("1"));
  list_value.Append(base::Value("2"));

  EXPECT_EQ(list_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUnusedAddresses_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUnusedAddresses(_))
      .WillByDefault(
          [&](TestCardanoApi::GetUnusedAddressesCallback callback) {
            std::move(callback).Run({}, mojom::CardanoProviderErrorBundle::New(
                                            -2, "Internal", nullptr));
          });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).getUnusedAddresses() } "
                       "catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-2));
  error_value.Set("info", "Internal");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetBalance) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetBalance(_))
      .WillByDefault([&](TestCardanoApi::GetBalanceCallback callback) {
        std::move(callback).Run("1", nullptr);
      });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).getBalance() })();");

  EXPECT_EQ(base::Value("1"), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetBalance_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetBalance(_))
      .WillByDefault(
          [&](TestCardanoApi::GetBalanceCallback callback) {
            std::move(callback).Run(std::nullopt,
                                    mojom::CardanoProviderErrorBundle::New(
                                        -2, "Internal", nullptr));
          });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).getBalance() } "
                       "catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-2));
  error_value.Set("info", "Internal");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetChangeAddress) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetChangeAddress(_))
      .WillByDefault([&](TestCardanoApi::GetChangeAddressCallback callback) {
        std::move(callback).Run("1", nullptr);
      });
  auto result =
      EvalJs(web_contents(browser()),
             "(async () => { return await (await "
             "window.cardano.brave.enable()).getChangeAddress() })();");

  EXPECT_EQ(base::Value("1"), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetChangeAddress_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetChangeAddress(_))
      .WillByDefault(
          [&](TestCardanoApi::GetChangeAddressCallback callback) {
            std::move(callback).Run(std::nullopt,
                                    mojom::CardanoProviderErrorBundle::New(
                                        -2, "Internal", nullptr));
          });
  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try{ return await (await "
                       "window.cardano.brave.enable()).getChangeAddress() } "
                       "catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-2));
  error_value.Set("info", "Internal");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetRewardAddresses) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetRewardAddresses(_))
      .WillByDefault([&](TestCardanoApi::GetRewardAddressesCallback callback) {
        std::vector<std::string> result{"1", "2"};
        std::move(callback).Run(result, nullptr);
      });

  auto result =
      EvalJs(web_contents(browser()),
             "(async () => { return await (await "
             "window.cardano.brave.enable()).getRewardAddresses() })();");

  base::Value::List list_value;
  list_value.Append(base::Value("1"));
  list_value.Append(base::Value("2"));

  EXPECT_EQ(list_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetRewardAddresses_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetRewardAddresses(_))
      .WillByDefault(
          [&](TestCardanoApi::GetRewardAddressesCallback callback) {
            std::move(callback).Run({}, mojom::CardanoProviderErrorBundle::New(
                                            -2, "Internal", nullptr));
          });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).getRewardAddresses() } "
                       "catch(err){return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(-2));
  error_value.Set("info", "Internal");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUtxos) {
  // Amount and pagination are defined.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_EQ("1", amount.value());
          EXPECT_EQ(2, paginate->page);
          EXPECT_EQ(3, paginate->limit);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).getUtxos(\"1\", {page: "
               "2, limit:3}) })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));
    EXPECT_EQ(list_value, result);
  }

  // Amount undefined, pagination not defined.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_FALSE(amount);
          EXPECT_FALSE(paginate);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).getUtxos(undefined) })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));
    EXPECT_EQ(list_value, result);
  }

  // Amount undefined with defined pagination.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_FALSE(amount);
          EXPECT_EQ(2, paginate->page);
          EXPECT_EQ(3, paginate->limit);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).getUtxos(undefined, {page: "
               "2, limit:3}) })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));
    EXPECT_EQ(list_value, result);
  }

  // Both undefined.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_FALSE(amount);
          EXPECT_FALSE(paginate);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result = EvalJs(
        web_contents(browser()),
        "(async () => { return await (await "
        "window.cardano.brave.enable()).getUtxos(undefined, undefined) })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));
    EXPECT_EQ(list_value, result);
  }

  // With amount, pagination not defined.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_EQ("1", amount);
          EXPECT_FALSE(paginate);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).getUtxos(\"1\") })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));

    EXPECT_EQ(list_value, result);
  }

  // With amount, pagination is undefined.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          EXPECT_EQ("1", amount);
          EXPECT_FALSE(paginate);
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    auto result = EvalJs(
        web_contents(browser()),
        "(async () => { return await (await "
        "window.cardano.brave.enable()).getUtxos(\"1\", undefined) })();");

    base::Value::List list_value;
    list_value.Append(base::Value("1"));
    list_value.Append(base::Value("2"));

    EXPECT_EQ(list_value, result);
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUtxos_NoArgs) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUtxos(_, _, _))
      .WillByDefault([&](const std::optional<std::string>& amount,
                         mojom::CardanoProviderPaginationPtr paginate,
                         TestCardanoApi::GetUtxosCallback callback) {
        EXPECT_FALSE(amount);
        EXPECT_FALSE(paginate);
        std::move(callback).Run(std::vector<std::string>({"1", "2"}), nullptr);
      });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).getUtxos() })();");

  base::Value::List list_value;
  list_value.Append(base::Value("1"));
  list_value.Append(base::Value("2"));

  EXPECT_EQ(list_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUtxos_WrongArguments) {
  // Wrong argument types.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    {
      auto result =
          EvalJs(web_contents(browser()),
                 "(async () => { return await (await "
                 "window.cardano.brave.enable()).getUtxos(1, 2) })();");
      EXPECT_FALSE(result.is_ok());
    }
  }

  // Wrong arguments amount.
  {
    TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
        web_contents(browser())->GetPrimaryMainFrame());
    auto cardano_api = std::make_unique<TestCardanoApi>();
    ON_CALL(*provider, Enable(_, _))
        .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                           TestCardanoProvider::EnableCallback callback) {
          cardano_api->BindReceiver(std::move(receiver));
          std::move(callback).Run(nullptr);
        });
    ON_CALL(*cardano_api, GetUtxos(_, _, _))
        .WillByDefault([&](const std::optional<std::string>& amount,
                           mojom::CardanoProviderPaginationPtr paginate,
                           TestCardanoApi::GetUtxosCallback callback) {
          std::move(callback).Run(std::vector<std::string>({"1", "2"}),
                                  nullptr);
        });

    {
      auto result = EvalJs(web_contents(browser()),
                           "(async () => { return await (await "
                           "window.cardano.brave.enable()).getUtxos(undefined, "
                           "undefined, undefined) })();");
      EXPECT_FALSE(result.is_ok());
    }
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUtxos_WrongPagination) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUtxos(_, _, _))
      .WillByDefault(
          [&](const std::optional<std::string>& amount,
              mojom::CardanoProviderPaginationPtr paginate,
              TestCardanoApi::GetUtxosCallback callback) {
            std::move(callback).Run(
                std::vector<std::string>(),
                mojom::CardanoProviderErrorBundle::New(
                    0, "error",
                    mojom::CardanoProviderPaginationErrorPayload::New(2)));
          });

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { try { return await (await "
                         "window.cardano.brave.enable()).getUtxos() } "
                         "catch(error) {return error} })();");
    base::Value::Dict dict_value;
    dict_value.Set("maxSize", base::Value(2));

    EXPECT_EQ(dict_value, result);
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetUtxos_NullResult) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetUtxos(_, _, _))
      .WillByDefault([&](const std::optional<std::string>& amount,
                         mojom::CardanoProviderPaginationPtr paginate,
                         TestCardanoApi::GetUtxosCallback callback) {
        std::move(callback).Run(std::nullopt, nullptr);
      });

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { try { return await (await "
                         "window.cardano.brave.enable()).getUtxos() } "
                         "catch(error) {return error} })();");
    EXPECT_EQ(base::Value(), result);
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignTx) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignTx(_, _, _))
      .WillByDefault([&](const std::string& tx, bool partial_sign,
                         TestCardanoApi::SignTxCallback callback) {
        EXPECT_EQ(partial_sign, true);
        EXPECT_EQ(tx, "tx");
        std::move(callback).Run("signed_tx", nullptr);
      });

  auto result =
      EvalJs(web_contents(browser()),
             "(async () => { return await (await "
             "window.cardano.brave.enable()).signTx(\"tx\", true) })();");

  EXPECT_EQ(base::Value("signed_tx"), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignTx_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignTx(_, _, _))
      .WillByDefault(
          [&](const std::string& tx, bool partial_sign,
              TestCardanoApi::SignTxCallback callback) {
            EXPECT_EQ(partial_sign, true);
            EXPECT_EQ(tx, "tx");
            std::move(callback).Run(std::nullopt,
                                    mojom::CardanoProviderErrorBundle::New(
                                        1, "Proof error", nullptr));
          });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).signTx(\"tx\", true) } "
                       "catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(1));
  error_value.Set("info", "Proof error");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignTx_PartialUndefined) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignTx(_, _, _))
      .WillByDefault([&](const std::string& tx, bool partial_sign,
                         TestCardanoApi::SignTxCallback callback) {
        EXPECT_EQ(partial_sign, false);
        EXPECT_EQ(tx, "tx");
        std::move(callback).Run("signed_tx", nullptr);
      });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).signTx(\"tx\") })();");

  EXPECT_EQ(base::Value("signed_tx"), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignTx_WrongArguments) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignTx(_, _, _))
      .WillByDefault([&](const std::string& tx, bool partial_sign,
                         TestCardanoApi::SignTxCallback callback) {
        EXPECT_EQ(partial_sign, true);
        EXPECT_EQ(tx, "tx");
        std::move(callback).Run("signed_tx", nullptr);
      });

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).signTx(\"tx\", 1) })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).signTx(\"tx\", \"\") })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).signTx(\"tx\", 1) })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { return await (await "
                         "window.cardano.brave.enable()).signTx(1) })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { return await (await "
                         "window.cardano.brave.enable()).signTx() })();");
    EXPECT_FALSE(result.is_ok());
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignData) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignData(_, _, _))
      .WillByDefault([&](const std::string& address, const std::string& data,
                         TestCardanoApi::SignDataCallback callback) {
        EXPECT_EQ("addr", address);
        EXPECT_EQ("data", data);
        base::Value::Dict signature_dict;
        signature_dict.Set("key", "key_value");
        signature_dict.Set("signature", "signature_value");
        std::move(callback).Run(std::move(signature_dict), nullptr);
      });

  auto result = EvalJs(
      web_contents(browser()),
      "(async () => { return await (await "
      "window.cardano.brave.enable()).signData(\"addr\", \"data\") })();");

  base::Value::Dict dict_value;
  dict_value.Set("key", base::Value("key_value"));
  dict_value.Set("signature", base::Value("signature_value"));

  EXPECT_EQ(dict_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignData_WrongArguments) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignData(_, _, _))
      .WillByDefault([&](const std::string& address, const std::string& data,
                         TestCardanoApi::SignDataCallback callback) {
        EXPECT_EQ("addr", address);
        EXPECT_EQ("data", data);
        base::Value::Dict signature_dict;
        signature_dict.Set("key", "key_value");
        signature_dict.Set("signature", "signature_value");
        std::move(callback).Run(std::move(signature_dict), nullptr);
      });

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).signData(\"addr\") })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { return await (await "
                         "window.cardano.brave.enable()).signData() })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).signData(\"\", 1) })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(
        web_contents(browser()),
        "(async () => { return await (await "
        "window.cardano.brave.enable()).signData(\"\", \"\", \"\") })();");
    EXPECT_FALSE(result.is_ok());
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SignData_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SignData(_, _, _))
      .WillByDefault(
          [&](const std::string& address, const std::string& data,
              TestCardanoApi::SignDataCallback callback) {
            EXPECT_EQ("addr", address);
            EXPECT_EQ("data", data);
            std::move(callback).Run(std::nullopt,
                                    mojom::CardanoProviderErrorBundle::New(
                                        2, "Data sign error", nullptr));
          });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).signData(\"addr\", "
                       "\"data\") } catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(2));
  error_value.Set("info", "Data sign error");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SubmitTx_Error) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SubmitTx(_, _))
      .WillByDefault(
          [&](const std::string& tx,
              TestCardanoApi::SubmitTxCallback callback) {
            std::move(callback).Run(
                std::nullopt,
                mojom::CardanoProviderErrorBundle::New(1, "Refused", nullptr));
          });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { try { return await (await "
                       "window.cardano.brave.enable()).submitTx(\"1\") } "
                       "catch(err) {return err}})();");

  base::Value::Dict error_value;
  error_value.Set("code", base::Value(1));
  error_value.Set("info", "Refused");

  EXPECT_EQ(error_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SubmitTx) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SubmitTx(_, _))
      .WillByDefault([&](const std::string& tx,
                         TestCardanoApi::SubmitTxCallback callback) {
        std::move(callback).Run("hash", nullptr);
      });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).submitTx(\"1\") })();");

  EXPECT_EQ(base::Value("hash"), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SubmitTx_WrongArguments) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, SubmitTx(_, _))
      .WillByDefault([&](const std::string& tx,
                         TestCardanoApi::SubmitTxCallback callback) {
        std::move(callback).Run("hash", nullptr);
      });

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { return await (await "
                         "window.cardano.brave.enable()).submitTx(1) })();");

    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(web_contents(browser()),
                         "(async () => { return await (await "
                         "window.cardano.brave.enable()).submitTx() })();");

    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).submitTx(\"1\", \"2\") })();");

    EXPECT_FALSE(result.is_ok());
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetExtensions) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(nullptr);
      });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).getExtensions()})();");
  EXPECT_EQ(base::Value::List(), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, GetCollateral) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetCollateral(_, _))
      .WillByDefault([&](const std::string& amount,
                         TestCardanoApi::GetCollateralCallback callback) {
        EXPECT_EQ("amount", amount);
        std::move(callback).Run(std::vector<std::string>({"1", "2"}), nullptr);
      });

  auto result = EvalJs(web_contents(browser()),
                       "(async () => { return await (await "
                       "window.cardano.brave.enable()).getCollateral({amount: "
                       "\"amount\"}) })();");

  base::Value::List list_value;
  list_value.Append(base::Value("1"));
  list_value.Append(base::Value("2"));
  EXPECT_EQ(list_value, result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest,
                       GetCollateral_WrongArguments) {
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  auto cardano_api = std::make_unique<TestCardanoApi>();
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        cardano_api->BindReceiver(std::move(receiver));
        std::move(callback).Run(nullptr);
      });
  ON_CALL(*cardano_api, GetCollateral(_, _))
      .WillByDefault([&](const std::string& amount,
                         TestCardanoApi::GetCollateralCallback callback) {
        EXPECT_EQ("amount", amount);
        std::move(callback).Run(std::vector<std::string>({"1", "2"}), nullptr);
      });

  {
    auto result =
        EvalJs(web_contents(browser()),
               "(async () => { return await (await "
               "window.cardano.brave.enable()).getCollateral(1) })();");
    EXPECT_FALSE(result.is_ok());
  }

  {
    auto result = EvalJs(
        web_contents(browser()),
        "(async () => { return await (await "
        "window.cardano.brave.enable()).getCollateral({amount: 1}) })();");
    EXPECT_FALSE(result.is_ok());
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, Iframe3P) {
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
  } cardano_undefined_cases[] =
      {{// 3p iframe
        "true", secure_top_url, iframe_url_3p},
       {// 1st party iframe with allow="cardano 'none'"
        R"(
        document.querySelector('iframe').setAttribute(
          'allow', 'cardano \'none\'');
        true
        )",
        secure_top_url, iframe_url_1p},
       {// 1st party iframe with sandbox="allow-scripts"
        R"(
        document.querySelector('iframe').removeAttribute('allow');
        document.querySelector('iframe').setAttribute(
          'sandbox', 'allow-scripts');
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

       {// 3p iframe with allow="cardano; ethereum" but insecure top level
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'cardano; ethereum');
      true
      )",
        insecure_top_url, iframe_url_3p},

       {// 3p iframe with allow="cardano; ethereum" but insecure top level (data
        // URI)
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'cardano; ethereum');
      true
      )",
        data_top_url, iframe_url_3p},

       {// 3p iframe with allow="cardano; ethereum" but insecure iframe
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'cardano; ethereum');
      true
      )",
        secure_top_url, data_simple_url},
       {// insecure top level and insecure iframe allow="cardano; ethereum"
        R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe')
          .setAttribute('allow', 'cardano; ethereum');
      true
      )",
        data_top_url, data_simple_url}},

    cardano_defined_cases[] = {
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
        {// 3p iframe with allow="cardano"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow', 'cardano');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3p iframe with allow="ethereum; cardano"
         R"(
      document.querySelector('iframe').removeAttribute('sandbox');
      document.querySelector('iframe').setAttribute('allow',
        'ethereum; cardano');
      true
      )",
         secure_top_url, iframe_url_3p},
        {// 3rd party iframe with sandbox="allow-scripts" allow="cardano"
         R"(
      document.querySelector('iframe').setAttribute('allow', 'cardano');
      document.querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
      true
      )",
         secure_top_url, iframe_url_3p}};

  for (auto& c : cardano_undefined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    EXPECT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    EXPECT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_FALSE(content::EvalJs(ChildFrameAt(main_frame, 0),
                                 kCheckCardanoProviderScript)
                     .ExtractBool());
  }
  for (auto& c : cardano_defined_cases) {
    SCOPED_TRACE(testing::Message() << c.script << c.top_url << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    EXPECT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    EXPECT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_TRUE(content::EvalJs(ChildFrameAt(main_frame, 0),
                                kCheckCardanoProviderScript)
                    .ExtractBool());
  }
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, SecureContextOnly) {
  // Secure context HTTPS server
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::RenderFrameHost* main_frame =
      web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckCardanoProviderScript).ExtractBool());

  // Insecure context
  url = embedded_test_server()->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_FALSE(
      content::EvalJs(main_frame, kCheckCardanoProviderScript).ExtractBool());

  // Secure context localhost HTTP
  url = embedded_test_server()->GetURL("localhost", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckCardanoProviderScript).ExtractBool());

  // Secure context 127.0.0.1 HTTP
  url = embedded_test_server()->GetURL("localhost", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckCardanoProviderScript).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest,
                       CardanoWeb3PrototypePollution) {
  ASSERT_TRUE(ExecJs(web_contents(browser()), "Object.freeze = ()=>{}"));
  TestCardanoProvider* provider = test_content_browser_client_.GetProvider(
      web_contents(browser())->GetPrimaryMainFrame());
  ON_CALL(*provider, Enable(_, _))
      .WillByDefault([&](mojo::PendingReceiver<mojom::CardanoApi> receiver,
                         TestCardanoProvider::EnableCallback callback) {
        std::move(callback).Run(nullptr);
      });
  auto result = EvalJs(web_contents(browser()), EnableScript());
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(CardanoProviderRendererTest, NotInstalled) {
  brave_wallet::SetDefaultCardanoWallet(
      browser()->profile()->GetPrefs(),
      brave_wallet::mojom::DefaultWallet::None);

  GURL url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto result = EvalJs(web_contents(browser()), kCheckCardanoProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

}  // namespace brave_wallet

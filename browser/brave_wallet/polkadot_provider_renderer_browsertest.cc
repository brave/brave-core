/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"  // IWYU pragma: keep
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_wallet {

namespace {

constexpr char kCheckPolkadotProviderScript[] =
    "!!window.injectedWeb3 && !!window.injectedWeb3['brave-wallet']";

class TestPolkadotApi : public mojom::PolkadotApi {
 public:
  TestPolkadotApi() = default;
  ~TestPolkadotApi() override = default;

  MOCK_METHOD2(GetAccounts, void(bool any_type, GetAccountsCallback callback));

  void BindReceiver(mojo::PendingReceiver<mojom::PolkadotApi> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  mojo::ReceiverSet<mojom::PolkadotApi> receivers_;
};

class TestPolkadotProvider : public mojom::PolkadotProvider {
 public:
  TestPolkadotProvider() = default;
  ~TestPolkadotProvider() override = default;

  MOCK_METHOD1(Enable, void(EnableCallback callback));

  void BindReceiver(mojo::PendingReceiver<mojom::PolkadotProvider> receiver) {
    receivers_.Add(this, std::move(receiver));
  }

 private:
  mojo::ReceiverSet<mojom::PolkadotProvider> receivers_;
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
    // Override the binding for PolkadotProvider so the renderer talks to
    // `provider_` instead of the real PolkadotProviderImpl.
    map->Add<mojom::PolkadotProvider>(base::BindRepeating(
        &TestBraveContentBrowserClient::BindPolkadotProvider,
        weak_ptr_factory_.GetWeakPtr()));
  }

  TestPolkadotProvider* provider() { return &provider_; }
  TestPolkadotApi* api() { return &api_; }

 private:
  void BindPolkadotProvider(
      content::RenderFrameHost* const frame_host,
      mojo::PendingReceiver<mojom::PolkadotProvider> receiver) {
    provider_.BindReceiver(std::move(receiver));
  }

  TestPolkadotProvider provider_;
  TestPolkadotApi api_;
  base::WeakPtrFactory<TestBraveContentBrowserClient> weak_ptr_factory_{this};
};

}  // namespace

class PolkadotProviderRendererTest : public InProcessBrowserTest {
 public:
  PolkadotProviderRendererTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // The provider is only injected once a wallet exists, so create one before
    // the navigation below.
    ASSERT_TRUE(GetKeyringService()->RestoreWalletSync(
        kMnemonicScarePiece, kTestWalletPassword, false));
    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));
  }

  content::WebContents* web_contents(Browser* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void ReloadAndWaitForLoadStop(Browser* browser) {
    chrome::Reload(browser, WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents(browser)));
  }

  KeyringService* GetKeyringService() {
    return BraveWalletServiceFactory::GetServiceForContext(
               browser()->GetProfile())
        ->keyring_service();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

class PolkadotProviderDisabledRendererTest
    : public PolkadotProviderRendererTest {
 public:
  PolkadotProviderDisabledRendererTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "false"}}}},
        {});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, AttachIfWalletCreated) {
  auto result =
      content::EvalJs(web_contents(browser()), kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, Version) {
  auto result = content::EvalJs(web_contents(browser()),
                                "window.injectedWeb3['brave-wallet'].version");
  EXPECT_EQ(base::Value("1.0.0"), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderDisabledRendererTest,
                       NotAttached_FeatureDisabled) {
  auto result =
      content::EvalJs(web_contents(browser()), kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest,
                       DoNotAttachIfNoWalletCreated) {
  GetKeyringService()->Reset(false);
  ReloadAndWaitForLoadStop(browser());

  auto result =
      content::EvalJs(web_contents(browser()), "!!window.injectedWeb3");
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, Incognito) {
  Browser* private_browser = CreateIncognitoBrowser(nullptr);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      private_browser, embedded_test_server()->GetURL("/empty.html")));

  auto result = content::EvalJs(web_contents(private_browser),
                                kCheckPolkadotProviderScript);
  EXPECT_EQ(base::Value(false), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, NonWritableEntry) {
  auto result =
      content::EvalJs(web_contents(browser()),
                      R"(window.injectedWeb3['brave-wallet'] = ['test'];
         window.injectedWeb3['brave-wallet'].version)");
  EXPECT_EQ(base::Value("1.0.0"), result);
}

// `window.injectedWeb3` is shared with every other injecting wallet, and
// @polkadot/extension-inject reassigns the property itself before adding its
// own key, from strict mode. Neither may be broken by our entry.
// See:
// https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-inject/src/bundle.ts#L20-L47
IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, OtherWalletCanInject) {
  auto result = content::EvalJs(web_contents(browser()),
                                R"('use strict';
         window.injectedWeb3 = window.injectedWeb3 || {};
         window.injectedWeb3['other-wallet'] = { version: '1' };
         !!window.injectedWeb3['brave-wallet'] &&
             !!window.injectedWeb3['other-wallet'])");
  EXPECT_EQ(base::Value(true), result);
}

// Exercises the renderer side of `enable()` against a mocked
// mojom::PolkadotProvider, so the shape of the resolved `Injected` object is
// covered without granting a real permission.
class PolkadotProviderEnableRendererTest : public PolkadotProviderRendererTest {
 public:
  void SetUpOnMainThread() override {
    content::SetBrowserClientForTesting(&test_content_browser_client_);
    PolkadotProviderRendererTest::SetUpOnMainThread();
    // Intentional: makes the browser re-run
    // RegisterBrowserInterfaceBindersForFrame for the frame under test.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://settings")));
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));
  }

  TestPolkadotProvider* provider() {
    return test_content_browser_client_.provider();
  }
  TestPolkadotApi* api() { return test_content_browser_client_.api(); }

  // Makes the mocked provider grant the permission, handing the renderer a
  // remote bound to `api()`.
  void ExpectEnableGranted() {
    EXPECT_CALL(*provider(), Enable(testing::_))
        .WillRepeatedly([&](mojom::PolkadotProvider::EnableCallback callback) {
          mojo::PendingRemote<mojom::PolkadotApi> remote;
          api()->BindReceiver(remote.InitWithNewPipeAndPassReceiver());
          std::move(callback).Run(std::move(remote), nullptr);
        });
  }

 private:
  TestBraveContentBrowserClient test_content_browser_client_;
};

IN_PROC_BROWSER_TEST_F(PolkadotProviderEnableRendererTest, EnableShape) {
  ExpectEnableGranted();

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         const injected = await window.injectedWeb3['brave-wallet'].enable(
             'test dapp');
         return typeof injected.accounts.get === 'function' &&
                typeof injected.accounts.subscribe === 'function' &&
                typeof injected.signer.signPayload === 'function' &&
                typeof injected.signer.signRaw === 'function';
       })())");
  EXPECT_EQ(base::Value(true), result);
}

// `connect()` is the newer hook and, unlike `enable()`, carries the extension
// name and version on the object it resolves to.
IN_PROC_BROWSER_TEST_F(PolkadotProviderEnableRendererTest, ConnectHasName) {
  ExpectEnableGranted();

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         const ext = await window.injectedWeb3['brave-wallet'].connect(
             'test dapp');
         return [ext.name, ext.version].join('/');
       })())");
  EXPECT_EQ(base::Value("Brave Wallet/1.0.0"), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderEnableRendererTest, GetAccounts) {
  ExpectEnableGranted();

  EXPECT_CALL(*api(), GetAccounts(false, testing::_))
      .WillOnce([](bool any_type,
                   mojom::PolkadotApi::GetAccountsCallback callback) {
        std::vector<mojom::PolkadotInjectedAccountPtr> accounts;
        accounts.push_back(mojom::PolkadotInjectedAccount::New(
            "5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY",
            /*genesis_hash=*/std::nullopt, "Account 1", "sr25519"));
        std::move(callback).Run(std::move(accounts), nullptr);
      });

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         const injected = await window.injectedWeb3['brave-wallet'].enable(
             'test dapp');
         const accounts = await injected.accounts.get();
         return JSON.stringify(accounts);
       })())");
  EXPECT_EQ(
      base::Value(
          R"([{"address":"5GrwvaEF5zXb26Fz9rcQpDWS57CtERHpNehXCPcNoHGKutQY",)"
          R"("genesisHash":null,"name":"Account 1","type":"sr25519"}])"),
      result);
}

// A rejected request must reject the promise with the message the browser
// supplied, since that is all a dapp has to show the user.
IN_PROC_BROWSER_TEST_F(PolkadotProviderEnableRendererTest, EnableRejected) {
  EXPECT_CALL(*provider(), Enable(testing::_))
      .WillRepeatedly([](mojom::PolkadotProvider::EnableCallback callback) {
        std::move(callback).Run(
            mojo::NullRemote(),
            mojom::PolkadotProviderErrorBundle::New(
                mojom::PolkadotProviderError::kUnknown, "no thanks"));
      });

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         try {
           await window.injectedWeb3['brave-wallet'].enable('test dapp');
           return 'resolved';
         } catch (err) {
           return err.message;
         }
       })())");
  EXPECT_EQ(base::Value("no thanks"), result);
}

// Signing isn't implemented yet, but the methods must exist and reject rather
// than be absent, so dapps fail on the call instead of on property access.
IN_PROC_BROWSER_TEST_F(PolkadotProviderEnableRendererTest, SignerRejects) {
  ExpectEnableGranted();

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         const injected = await window.injectedWeb3['brave-wallet'].enable(
             'test dapp');
         try {
           await injected.signer.signRaw({});
           return 'resolved';
         } catch (err) {
           return err.message;
         }
       })())");
  EXPECT_EQ(base::Value("Signing is not implemented by this extension yet"),
            result);
}

}  // namespace brave_wallet

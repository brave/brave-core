/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"  // IWYU pragma: keep
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

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
  PolkadotProviderRendererTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
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

    // The provider is only injected once a wallet exists, so create one before
    // the call to NavigateToURL.
    ASSERT_TRUE(GetKeyringService()->RestoreWalletSync(
        kMnemonicScarePiece, kTestWalletPassword, false));

    content::SetBrowserClientForTesting(&test_content_browser_client_);

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(https_server_.Start());

    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());

    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));

    // Intentional: makes the browser re-run
    // RegisterBrowserInterfaceBindersForFrame for the frame under test.
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL("brave://settings")));
    ASSERT_TRUE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/empty.html")));
  }

  content::WebContents* web_contents(BrowserWindowInterface* browser) const {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  void ReloadAndWaitForLoadStop(BrowserWindowInterface* browser) {
    chrome::Reload(browser, WindowOpenDisposition::CURRENT_TAB);
    ASSERT_TRUE(content::WaitForLoadStop(web_contents(browser)));
  }

  KeyringService* GetKeyringService() {
    return BraveWalletServiceFactory::GetServiceForContext(
               browser()->GetProfile())
        ->keyring_service();
  }

  TestPolkadotProvider* provider() {
    return test_content_browser_client_.provider();
  }

  TestPolkadotApi* api() { return test_content_browser_client_.api(); }

  void ExpectEnableGranted() {
    EXPECT_CALL(*provider(), Enable(testing::_))
        .WillOnce([&](mojom::PolkadotProvider::EnableCallback callback) {
          mojo::PendingRemote<mojom::PolkadotApi> remote;
          api()->BindReceiver(remote.InitWithNewPipeAndPassReceiver());
          std::move(callback).Run(std::move(remote), nullptr);
        });
  }

 protected:
  net::EmbeddedTestServer https_server_;
  TestBraveContentBrowserClient test_content_browser_client_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
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
  auto* private_browser = CreateIncognitoBrowser(nullptr);
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
// own key.
// See:
// https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-inject/src/bundle.ts#L20-L47
IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, OtherWalletCanInject) {
  auto result = content::EvalJs(web_contents(browser()),
                                R"(
    (function() {
      'use strict';
      window.injectedWeb3 = window.injectedWeb3 || {};
      window.injectedWeb3['other-wallet'] = { version: '1' };
    })();

    !!window.injectedWeb3['brave-wallet'] &&
        !!window.injectedWeb3['other-wallet']
)");
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, RegistryNotReplaceable) {
  // We only permit self-assignment how the polkadot/extension repo does it:
  // https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-inject/src/bundle.ts#L25
  // This is because basically all the dApps just use this package and it
  // prevents any malcious script from overriding what we've set.

  auto result = content::EvalJs(web_contents(browser()),
                                R"(
    (function() {
      'use strict';
      const registry = window.injectedWeb3;

      let threw = false;
      try {
        window.injectedWeb3 = { 'brave-wallet': { version: 'evil' } };
      } catch (e) {
        threw = true;
      }

      return threw && window.injectedWeb3 === registry &&
          window.injectedWeb3['brave-wallet'].version === '1.0.0';
    })()
)");
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest,
                       RegistryNotDeletableOrRedefinable) {
  // The accessor is non-configurable, otherwise a script could delete or
  // redefine the property to get around the setter.

  auto result = content::EvalJs(web_contents(browser()),
                                R"(
    (function() {
      'use strict';
      let deleteThrew = false;
      try {
        delete window.injectedWeb3;
      } catch (e) {
        deleteThrew = true;
      }

      let defineThrew = false;
      try {
        Object.defineProperty(window, 'injectedWeb3', { value: {} });
      } catch (e) {
        defineThrew = true;
      }

      return deleteThrew && defineThrew &&
          !!window.injectedWeb3['brave-wallet'];
    })()
)");
  EXPECT_EQ(base::Value(true), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, Iframe3P) {
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
  } polkadot_undefined_cases[] =
      {{// 3p iframe
        "true", secure_top_url, iframe_url_3p},
       {// 1st party iframe with allow="polkadot 'none'"
        R"(
        document.querySelector('iframe').setAttribute(
          'allow', 'polkadot \'none\'');
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
       {// 3p iframe with allow="polkadot; ethereum" but insecure top level
        R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe')
            .setAttribute('allow', 'polkadot; ethereum');
        true
        )",
        insecure_top_url, iframe_url_3p},

       {// 3p iframe with allow="polkadot; ethereum" but insecure top level
        // (data URI)
        R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe')
            .setAttribute('allow', 'polkadot; ethereum');
        true
        )",
        data_top_url, iframe_url_3p},

       {// 3p iframe with allow="polkadot; ethereum" but insecure iframe
        R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe')
            .setAttribute('allow', 'polkadot; ethereum');
        true
        )",
        secure_top_url, data_simple_url},
       {// insecure top level and insecure iframe allow="polkadot; ethereum"
        R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe')
            .setAttribute('allow', 'polkadot; ethereum');
        true
        )",
        data_top_url, data_simple_url}},
    polkadot_defined_cases[] = {
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
        {// 3p iframe with allow="polkadot"
         R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe').setAttribute('allow', 'polkadot');
        true
        )",
         secure_top_url, iframe_url_3p},
        {// 3p iframe with allow="ethereum; polkadot"
         R"(
        document.querySelector('iframe').removeAttribute('sandbox');
        document.querySelector('iframe').setAttribute('allow',
          'ethereum; polkadot');
        true
        )",
         secure_top_url, iframe_url_3p},
        {// 3rd party iframe with sandbox="allow-scripts" allow="polkadot"
         R"(
        document.querySelector('iframe').setAttribute('allow', 'polkadot');
        document
          .querySelector('iframe').setAttribute('sandbox', 'allow-scripts');
        true
        )",
         secure_top_url, iframe_url_3p}};

  for (auto& c : polkadot_undefined_cases) {
    SCOPED_TRACE(testing::Message()
                 << "script: " << c.script << ", top_url: " << c.top_url
                 << ", iframe_url: " << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    ASSERT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    ASSERT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_FALSE(content::EvalJs(ChildFrameAt(main_frame, 0),
                                 kCheckPolkadotProviderScript)
                     .ExtractBool());
  }
  for (auto& c : polkadot_defined_cases) {
    SCOPED_TRACE(testing::Message()
                 << "script: " << c.script << ", top_url: " << c.top_url
                 << ", iframe_url: " << c.iframe_url);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), c.top_url));
    content::RenderFrameHost* main_frame =
        web_contents(browser())->GetPrimaryMainFrame();
    ASSERT_TRUE(content::EvalJs(main_frame, c.script).ExtractBool());
    ASSERT_TRUE(
        NavigateIframeToURL(web_contents(browser()), "test", c.iframe_url));
    EXPECT_TRUE(content::EvalJs(ChildFrameAt(main_frame, 0),
                                kCheckPolkadotProviderScript)
                    .ExtractBool());
  }
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, SecureContextOnly) {
  // Secure context HTTPS server.
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::RenderFrameHost* main_frame =
      web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckPolkadotProviderScript).ExtractBool());

  // Insecure context.
  url = embedded_test_server()->GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_FALSE(
      content::EvalJs(main_frame, kCheckPolkadotProviderScript).ExtractBool());

  // Secure context localhost HTTP.
  url = embedded_test_server()->GetURL("localhost", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckPolkadotProviderScript).ExtractBool());

  // Secure context 127.0.0.1 HTTP.
  url = embedded_test_server()->GetURL("127.0.0.1", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  main_frame = web_contents(browser())->GetPrimaryMainFrame();
  EXPECT_TRUE(
      content::EvalJs(main_frame, kCheckPolkadotProviderScript).ExtractBool());
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, EnableShape) {
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

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, ConnectHasName) {
  // `connect()` is the newer hook and, unlike `enable()`, carries the extension
  // name and version on the object it resolves to.
  ExpectEnableGranted();

  auto result = content::EvalJs(web_contents(browser()),
                                R"((async () => {
         const { name,version } = await window.injectedWeb3['brave-wallet']
          .connect('test dapp');
         return [name, version].join('/');
       })())");
  EXPECT_EQ(base::Value("Brave Wallet/1.0.0"), result);
}

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, GetAccounts) {
  ExpectEnableGranted();

  EXPECT_CALL(*api(), GetAccounts(false, testing::_))
      .WillOnce(
          [](bool any_type, mojom::PolkadotApi::GetAccountsCallback callback) {
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

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, EnableRejected) {
  // A rejected request must reject the promise with the message the browser
  // supplied, since that is all a dapp has to show the user.

  EXPECT_CALL(*provider(), Enable(testing::_))
      .WillOnce([](mojom::PolkadotProvider::EnableCallback callback) {
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

IN_PROC_BROWSER_TEST_F(PolkadotProviderRendererTest, SignerRejects) {
  // Signing isn't implemented yet, but the methods must exist and reject rather
  // than be absent, so dapps fail on the call instead of on property access.

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

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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

constexpr char kTestPublicKey[] =
    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";

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

class TestSolanaProvider final : public brave_wallet::mojom::SolanaProvider {
 public:
  TestSolanaProvider() = default;
  ~TestSolanaProvider() override = default;
  TestSolanaProvider(const TestSolanaProvider&) = delete;
  TestSolanaProvider& operator=(const TestSolanaProvider&) = delete;

  void Init(mojo::PendingRemote<brave_wallet::mojom::SolanaEventsListener>
                events_listener) override {}
  void Connect(absl::optional<base::Value> arg,
               ConnectCallback callback) override {
    if (error_ == SolanaProviderError::kSuccess)
      std::move(callback).Run(SolanaProviderError::kSuccess, "",
                              kTestPublicKey);
    else {
      std::move(callback).Run(error_, error_message_, "");
    }
  }
  void Disconnect() override {}
  void IsConnected(IsConnectedCallback callback) override {}
  void GetPublicKey(GetPublicKeyCallback callback) override {}
  void SignTransaction(const std::string& encoded_serialized_msg,
                       SignTransactionCallback callback) override {}
  void SignAllTransactions(
      const std::vector<std::string>& encoded_serialized_msgs,
      SignAllTransactionsCallback callback) override {}
  void SignAndSendTransaction(
      const std::string& encoded_serialized_msg,
      SignAndSendTransactionCallback callback) override {}
  void SignMessage(const std::string& encoded_msg,
                   const absl::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override {}
  void Request(base::Value arg, RequestCallback callback) override {}

  void SetError(SolanaProviderError error, const std::string& error_message) {
    error_ = error;
    error_message_ = error_message;
  }

 private:
  void ClearError() {
    error_ = SolanaProviderError::kSuccess;
    error_message_.clear();
  }
  SolanaProviderError error_ = SolanaProviderError::kSuccess;
  std::string error_message_;
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
  SolanaProviderTest() {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    content::SetBrowserClientForTesting(&test_content_browser_client_);
    host_resolver()->AddRule("*", "127.0.0.1");

    embedded_test_server()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);

    ASSERT_TRUE(test_server_handle_ =
                    embedded_test_server()->StartAndReturnHandle());

    // This is intentional to trigger
    // TestBraveContentBrowserClient::RegisterBrowserInterfaceBindersForFrame
    GURL url = GURL("brave://settings");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    ASSERT_TRUE(WaitForLoadStop(web_contents()));
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // HTTPS server only serves a valid cert for localhost, so this is needed
    // to load pages from other hosts without an error.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  content::WebContents* web_contents() const {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  TestBraveContentBrowserClient test_content_browser_client_;

 private:
  net::test_server::EmbeddedTestServerHandle test_server_handle_;
};

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, Connect) {
  GURL url = embedded_test_server()->GetURL("/empty.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  ASSERT_TRUE(WaitForLoadStop(web_contents()));

  auto result = EvalJs(web_contents(), ConnectScript(""),
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result.value);

  // allow extra parameters
  auto result2 = EvalJs(web_contents(), ConnectScript("{}, 123"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(base::Value(kTestPublicKey), result2.value);

  // non object args
  auto result3 = EvalJs(web_contents(), ConnectScript("123"),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  EXPECT_EQ(
      base::Value(l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS)),
      result3.value);

  TestSolanaProvider* provider =
      test_content_browser_client_.GetProvider(web_contents()->GetMainFrame());
  ASSERT_TRUE(provider);

  // error returns from browser process
  constexpr char kErrorMessage[] = "error from browser";
  provider->SetError(SolanaProviderError::kUserRejectedRequest, kErrorMessage);

  auto result4 = EvalJs(web_contents(), ConnectScript(""),
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY);
  // check error message + error code
  EXPECT_EQ(base::Value(kErrorMessage +
                        base::NumberToString(static_cast<int>(
                            SolanaProviderError::kUserRejectedRequest))),
            result4.value);
}

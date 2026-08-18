// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_handler.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/test/permission_request_observer.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

constexpr char kTestEthAccount[] = "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A";
constexpr char kTestSolAccount[] =
    "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";

bool HasEthereumPermission(content::BrowserContext* context,
                           const url::Origin& origin,
                           const std::string& account) {
  bool has_permission = false;
  return permissions::BraveWalletPermissionContext::HasPermission(
             blink::PermissionType::BRAVE_ETHEREUM, context, origin, account,
             &has_permission) &&
         has_permission;
}

}  // namespace

// The panel names one origin and every control on it must act on that same
// origin. These tests pin that down while a cross-origin subframe holds the
// browser's frame focus.
class WalletPanelHandlerBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory(
        GetChromeTestDataDir());
    ASSERT_TRUE(embedded_test_server()->Start());

    main_frame_url_ = embedded_test_server()->GetURL("a.test", "/iframe.html");
    subframe_url_ = embedded_test_server()->GetURL("b.test", "/title1.html");
  }

  void TearDownOnMainThread() override {
    subframe_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

  content::RenderFrameHost* subframe() { return subframe_; }

  // Loads a.test embedding a cross-origin b.test iframe and moves the browser's
  // frame focus into that iframe, which is the precondition the attack relies
  // on. Call via ASSERT_NO_FATAL_FAILURE.
  void LoadPageAndFocusSubframe() {
    subframe_ = nullptr;
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), main_frame_url_));
    ASSERT_TRUE(
        content::NavigateIframeToURL(web_contents(), "test", subframe_url_));

    subframe_ = content::ChildFrameAt(main_frame(), 0);
    ASSERT_TRUE(subframe_);
    ASSERT_EQ(url::Origin::Create(subframe_url_),
              subframe_->GetLastCommittedOrigin());
    ASSERT_NE(main_frame()->GetLastCommittedOrigin(),
              subframe_->GetLastCommittedOrigin());

    // Focusing an element inside the subframe makes it the browser's focused
    // frame, via RenderFrameHostImpl::DidFocusFrame. That is a renderer to
    // browser message, so wait for the browser side state to catch up.
    ASSERT_TRUE(content::ExecJs(subframe_, R"(
        const input = document.createElement('input');
        document.body.appendChild(input);
        input.focus();
    )"));
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return web_contents()->GetFocusedFrame() == subframe_.get(); }));
  }

  // The origin the panel puts on screen for this tab.
  std::string GetDisplayedOriginSpec() {
    return brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
               browser()->GetProfile())
        ->GetActiveOriginSync()
        ->origin_spec;
  }

  // RequestPermission, ConnectToSite and IsSolanaAccountConnected never touch
  // the WebUI controller, so the panel does not need to be instantiated to
  // drive them.
  std::unique_ptr<WalletPanelHandler> CreateHandler() {
    return std::make_unique<WalletPanelHandler>(
        handler_remote_.BindNewPipeAndPassReceiver(),
        /*webui_controller=*/nullptr, web_contents());
  }

 protected:
  GURL main_frame_url_;
  GURL subframe_url_;

 private:
  raw_ptr<content::RenderFrameHost> subframe_ = nullptr;
  mojo::Remote<brave_wallet::mojom::PanelHandler> handler_remote_;
};

// Connecting from the panel must grant to the origin the panel displays, even
// though a cross-origin subframe holds the frame focus.
IN_PROC_BROWSER_TEST_F(
    WalletPanelHandlerBrowserTest,
    RequestPermission_GrantsToDisplayedOriginWhileSubframeIsFocused) {
  ASSERT_NO_FATAL_FAILURE(LoadPageAndFocusSubframe());

  const url::Origin displayed_origin = url::Origin::Create(main_frame_url_);
  const url::Origin focused_origin = url::Origin::Create(subframe_url_);
  ASSERT_EQ(displayed_origin.Serialize(), GetDisplayedOriginSpec());

  auto handler = CreateHandler();

  permissions::PermissionRequestObserver prompt_observer(web_contents());
  base::test::TestFuture<bool> granted;
  handler->RequestPermission(
      brave_wallet::MakeAccountId(brave_wallet::mojom::CoinType::ETH,
                                  brave_wallet::mojom::KeyringId::kDefault,
                                  brave_wallet::mojom::AccountKind::kDerived,
                                  kTestEthAccount),
      granted.GetCallback());
  prompt_observer.Wait();
  ASSERT_TRUE(prompt_observer.request_shown());

  handler->ConnectToSite(
      {kTestEthAccount},
      brave_wallet::mojom::PermissionLifetimeOption::kForever);
  EXPECT_TRUE(granted.Get());

  auto* context = browser()->GetProfile();
  EXPECT_TRUE(HasEthereumPermission(context, displayed_origin, kTestEthAccount))
      << "the panel named " << displayed_origin << " but did not grant to it";
  EXPECT_FALSE(HasEthereumPermission(context, focused_origin, kTestEthAccount))
      << "granted to focused subframe " << focused_origin
      << ", an origin the panel never named";
}

// The connected indicator must describe the origin the panel displays, not
// whichever frame holds the focus.
IN_PROC_BROWSER_TEST_F(WalletPanelHandlerBrowserTest,
                       IsSolanaAccountConnected_ReportsMainFrameNotSubframe) {
  ASSERT_NO_FATAL_FAILURE(LoadPageAndFocusSubframe());
  ASSERT_EQ(url::Origin::Create(main_frame_url_).Serialize(),
            GetDisplayedOriginSpec());

  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents());
  ASSERT_TRUE(tab_helper);
  auto handler = CreateHandler();

  auto is_connected = [&] {
    base::test::TestFuture<bool> future;
    handler->IsSolanaAccountConnected(kTestSolAccount, future.GetCallback());
    return future.Get();
  };

  EXPECT_FALSE(is_connected());

  // A connection held by the focused subframe is not a connection of the top
  // level site the panel names.
  tab_helper->AddSolanaConnectedAccount(subframe()->GetGlobalId(),
                                        kTestSolAccount);
  EXPECT_FALSE(is_connected());

  // A main frame connection is still reported.
  tab_helper->AddSolanaConnectedAccount(main_frame()->GetGlobalId(),
                                        kTestSolAccount);
  EXPECT_TRUE(is_connected());
}

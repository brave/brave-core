// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/hid/hid_chooser_context.h"
#include "chrome/browser/hid/hid_chooser_context_factory.h"
#include "chrome/browser/ui/chooser_bubble_testapi.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "services/device/public/cpp/test/fake_hid_manager.h"

using ::testing::HasSubstr;

namespace brave_wallet {

namespace {

constexpr char kUserGestureError[] =
    "Must be handling a user gesture to show a permission request.";

}

class LedgerUIBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_->AddDefaultHandlers(GetChromeTestDataDir());
    ASSERT_TRUE(https_server_->Start());

    mojo::PendingRemote<device::mojom::HidManager> hid_manager;
    hid_manager_.Bind(hid_manager.InitWithNewPipeAndPassReceiver());

    base::test::TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>>
        future_devices;
    HidChooserContextFactory::GetForProfile(browser()->profile())
        ->SetHidManagerForTesting(std::move(hid_manager),
                                  future_devices.GetCallback());
    ASSERT_TRUE(future_devices.Wait());

    hid_manager_.CreateAndAddDevice("ledger_id", 0x2c97, 0x4015, "Ledger",
                                    "001",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
  }

  content::RenderFrameHost* AppendSubframe(content::RenderFrameHost* frame,
                                           const GURL& url) {
    EXPECT_TRUE(content::ExecJs(frame,
                                content::JsReplace(R"js(
        new Promise(resolve => {
          const iframe = document.createElement('iframe');
          iframe.onload = resolve;
          iframe.allow = 'hid';
          iframe.src = $1;
          document.body.appendChild(iframe);
        })
      )js",
                                                   url),
                                content::EXECUTE_SCRIPT_NO_USER_GESTURE));
    return content::ChildFrameAt(frame, 0);
  }

  // Matches Ledger's device request:
  // https://github.com/LedgerHQ/ledger-live/blob/641a32b87cf83a38fb93dc65322ad4e2f6d3e8b5/libs/ledgerjs/packages/hw-transport-webhid/src/TransportWebHID.ts#L29-L35
  content::EvalJsResult ReturnLedgerDevice(content::RenderFrameHost* frame,
                                           int options) {
    return content::EvalJs(frame,
                           R"js(
          (async () => {
            const devices = await navigator.hid.requestDevice({
              filters: [ {vendorId : 0x2c97} ]
            });
            return devices.map(device => device.vendorId);
          })()
          )js",
                           options);
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 protected:
  device::FakeHidManager hid_manager_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_ =
      std::make_unique<net::EmbeddedTestServer>(
          net::EmbeddedTestServer::TYPE_HTTPS);
};

IN_PROC_BROWSER_TEST_F(LedgerUIBrowserTest,
                       ReturnLedgerDeviceWithoutUserGesture) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);

  // `requestDevice` in a ledger subframe returns ledger device even without
  // user gesture.
  content::RenderFrameHost* ledger_rfh =
      AppendSubframe(wallet_rfh, GURL(kUntrustedLedgerURL));
  auto result =
      ReturnLedgerDevice(ledger_rfh, content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_FALSE(ledger_rfh->HasTransientUserActivation());
  EXPECT_TRUE(result.is_ok());
  EXPECT_EQ(result.ExtractList(), base::ListValue().Append(0x2c97));

  // Request device in a wallet frame without user gesture fails.
  result =
      ReturnLedgerDevice(wallet_rfh, content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_FALSE(wallet_rfh->HasTransientUserActivation());
  EXPECT_THAT(result,
              content::EvalJsResult::ErrorIs(HasSubstr(kUserGestureError)));

  // Request device in a new tab frame without user gesture fails.
  auto* new_tab_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab"));
  result =
      ReturnLedgerDevice(new_tab_rfh, content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_FALSE(new_tab_rfh->HasTransientUserActivation());
  EXPECT_THAT(result,
              content::EvalJsResult::ErrorIs(HasSubstr(kUserGestureError)));

  // Request device in a https frame without user gesture fails.
  auto* https_rfh = ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("/empty.html"));
  result =
      ReturnLedgerDevice(https_rfh, content::EXECUTE_SCRIPT_NO_USER_GESTURE);
  EXPECT_FALSE(https_rfh->HasTransientUserActivation());
  EXPECT_THAT(result,
              content::EvalJsResult::ErrorIs(HasSubstr(kUserGestureError)));

  // Request device in a https frame but with a user gesture should still work
  // and device chooser bubble should be shown.
  auto waiter = test::ChooserBubbleUiWaiter::Create();
  result = ReturnLedgerDevice(https_rfh,
                              content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES);
  EXPECT_TRUE(https_rfh->HasTransientUserActivation());
  waiter->WaitForChange();
  EXPECT_TRUE(waiter->has_shown());
}

}  // namespace brave_wallet

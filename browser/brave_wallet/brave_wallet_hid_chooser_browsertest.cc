/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/hid/hid_chooser_context.h"
#include "chrome/browser/hid/hid_chooser_context_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chooser_bubble_testapi.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/device/public/cpp/test/fake_hid_manager.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using ::base::test::DictionaryHasValues;

namespace {

constexpr uint16_t kLedgerVendorId = 0x2c97;
constexpr uint16_t kOtherVendorId = 0x1234;
constexpr uint16_t kProductId = 0x4015;

}  // namespace

class BraveWalletHidChooserBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletHidChooserBrowserTest() = default;
  ~BraveWalletHidChooserBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    embedded_test_server()->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());

    mojo::PendingRemote<device::mojom::HidManager> hid_manager;
    hid_manager_.Bind(hid_manager.InitWithNewPipeAndPassReceiver());

    base::test::TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>>
        future_devices;
    HidChooserContextFactory::GetForProfile(browser()->profile())
        ->SetHidManagerForTesting(std::move(hid_manager),
                                  future_devices.GetCallback());
    ASSERT_TRUE(future_devices.Wait());

    hid_manager_.CreateAndAddDevice("ledger_id", kLedgerVendorId, kProductId,
                                    "Ledger", "001",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
    hid_manager_.CreateAndAddDevice("some_device_id", kOtherVendorId,
                                    kProductId, "Some Device", "123",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
  }

  base::DictValue ExpectedDevice() {
    return base::DictValue()
        .Set("vendorId", kLedgerVendorId)
        .Set("productName", "Ledger");
  }

  void RemoveAllDevices() {
    hid_manager_.GetDevices(base::BindLambdaForTesting(
        [this](std::vector<device::mojom::HidDeviceInfoPtr> devices) {
          for (auto& device : devices) {
            hid_manager_.RemoveDevice(device->guid);
          }
        }));
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  static base::ListValue MakeVendorFilter(uint16_t vendor_id) {
    return base::ListValue().Append(
        base::DictValue().Set("vendorId", static_cast<int>(vendor_id)));
  }

  static base::ListValue MakeLedgerFilter() {
    return MakeVendorFilter(kLedgerVendorId);
  }

  // Calls navigator.hid.requestDevice({filters, exclusionFilters}) and returns
  // the found device or a error string.
  content::EvalJsResult RequestDevice(
      content::RenderFrameHost* frame,
      base::ListValue filters = MakeLedgerFilter()) {
    return content::EvalJs(frame, content::JsReplace(
                                      R"js(
              (async () => {
                const devices = await navigator.hid.requestDevice({
                  filters: $1
                });
                if (devices.length !== 1) return "No Devices"
                const {vendorId, productName} = devices[0];
                return {vendorId, productName};
              })()
              )js",
                                      base::Value(std::move(filters))));
  }

  // Calls navigator.hid.requestDevice({filters, exclusionFilters}) without
  // awaiting result. Expects chromium's chooser bubble to be shown.
  void RequestDeviceAndExpectChromiumChooserBubble(
      content::RenderFrameHost* frame,
      base::ListValue filters = MakeLedgerFilter()) {
    auto waiter = test::ChooserBubbleUiWaiter::Create();

    ASSERT_TRUE(content::ExecJs(
        frame,
        content::JsReplace(R"js(navigator.hid.requestDevice({filters: $1}))js",
                           base::Value(std::move(filters))),
        content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));

    waiter->WaitForChange();
    EXPECT_TRUE(waiter->has_shown());
  }

  content::RenderFrameHost* AppendSubframe(const GURL& url) {
    EXPECT_TRUE(content::ExecJs(web_contents(), content::JsReplace(R"js(
        new Promise(resolve => {
          const iframe = document.createElement('iframe');
          iframe.onload = resolve;
          iframe.allow = 'hid';
          iframe.src = $1;
          document.body.appendChild(iframe);
        })
      )js",
                                                                   url)));
    return content::ChildFrameAt(web_contents()->GetPrimaryMainFrame(), 0);
  }

 private:
  device::FakeHidManager hid_manager_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest, LedgerSubframe) {
  auto waiter = test::ChooserBubbleUiWaiter::Create();

  content::RenderFrameHost* ledger_subframe = nullptr;

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
  ledger_subframe = AppendSubframe(GURL(kUntrustedLedgerURL));
  ASSERT_TRUE(ledger_subframe);
  EXPECT_THAT(RequestDevice(ledger_subframe, MakeVendorFilter(kLedgerVendorId))
                  .ExtractDict(),
              DictionaryHasValues(ExpectedDevice()));

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletPanelURL)));
  ledger_subframe = AppendSubframe(GURL(kUntrustedLedgerURL));
  ASSERT_TRUE(ledger_subframe);
  EXPECT_THAT(RequestDevice(ledger_subframe, MakeVendorFilter(kLedgerVendorId))
                  .ExtractDict(),
              DictionaryHasValues(ExpectedDevice()));

  EXPECT_FALSE(waiter->has_shown());
}

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest, NoDevices) {
  RemoveAllDevices();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
  auto* ledger_subframe = AppendSubframe(GURL(kUntrustedLedgerURL));

  EXPECT_EQ(RequestDevice(ledger_subframe).ExtractString(), "No Devices");
}

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest,
                       WalletMainFrameShowsChooserBubble) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
  RequestDeviceAndExpectChromiumChooserBubble(
      web_contents()->GetPrimaryMainFrame());

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletPanelURL)));
  RequestDeviceAndExpectChromiumChooserBubble(
      web_contents()->GetPrimaryMainFrame());
}

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest,
                       NonWalletOriginShowsChooserBubble) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL("/simple.html")));

  RequestDeviceAndExpectChromiumChooserBubble(
      web_contents()->GetPrimaryMainFrame());
}

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest,
                       TrezorSubframeShowsChooserBubble) {
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletPanelURL)));
  auto* trezor_subframe = AppendSubframe(GURL(kUntrustedTrezorURL));
  ASSERT_TRUE(trezor_subframe);
  RequestDeviceAndExpectChromiumChooserBubble(trezor_subframe);
}

IN_PROC_BROWSER_TEST_F(BraveWalletHidChooserBrowserTest, InvalidFilter) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
  auto* ledger_subframe = AppendSubframe(GURL(kUntrustedLedgerURL));

  EXPECT_THAT(RequestDevice(ledger_subframe, {}).ExtractString(), "No Devices");
  EXPECT_THAT(RequestDevice(ledger_subframe, MakeVendorFilter(kOtherVendorId))
                  .ExtractString(),
              "No Devices");
}

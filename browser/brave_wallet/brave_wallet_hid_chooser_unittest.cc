/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_hid_chooser.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "chrome/browser/hid/hid_chooser_context.h"
#include "chrome/browser/hid/hid_chooser_context_factory.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/hid_chooser.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/device/public/cpp/test/fake_hid_manager.h"
#include "services/device/public/mojom/hid.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/hid/hid.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

using base::test::TestFuture;

namespace {

constexpr uint16_t kLedgerVendorId = 0x2c97;
constexpr uint16_t kOtherVendorId = 0x1234;
constexpr uint16_t kProductId = 0x4015;

std::vector<blink::mojom::HidDeviceFilterPtr> MakeLedgerVendorOnlyFilters() {
  std::vector<blink::mojom::HidDeviceFilterPtr> filters;
  filters.push_back(blink::mojom::HidDeviceFilter::New(
      blink::mojom::DeviceIdFilter::NewVendor(kLedgerVendorId), nullptr));
  return filters;
}

}  // namespace

class BraveWalletHidChooserTest : public ChromeRenderViewHostTestHarness {
 public:
  BraveWalletHidChooserTest() = default;
  BraveWalletHidChooserTest(const BraveWalletHidChooserTest&) = delete;
  BraveWalletHidChooserTest& operator=(const BraveWalletHidChooserTest&) =
      delete;
  ~BraveWalletHidChooserTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    mojo::PendingRemote<device::mojom::HidManager> hid_manager;
    hid_manager_.Bind(hid_manager.InitWithNewPipeAndPassReceiver());

    TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future_devices;
    HidChooserContextFactory::GetForProfile(profile())->SetHidManagerForTesting(
        std::move(hid_manager), future_devices.GetCallback());
    ASSERT_TRUE(future_devices.Wait());

    AddNonLedgerDevice();
    AddLedgerDevice();
    AddAnotherLedgerDevice();

    content::WebContentsTester::For(web_contents())
        ->NavigateAndCommit(GURL("chrome://wallet/"));
    subframe_rfh_ =
        content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
    SetSubFrameOrigin("chrome-untrusted://ledger-bridge/");
  }

  void TearDown() override {
    subframe_rfh_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::RenderFrameHost* subframe_rfh() { return subframe_rfh_; }

  void AddLedgerDevice() {
    hid_manager_.CreateAndAddDevice("ledger", kLedgerVendorId, kProductId,
                                    "Ledger", "002",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
  }

  void AddAnotherLedgerDevice() {
    hid_manager_.CreateAndAddDevice("another_ledger", kLedgerVendorId,
                                    kProductId, "Another Ledger", "003",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
  }

  void AddNonLedgerDevice() {
    hid_manager_.CreateAndAddDevice("not_ledger", kOtherVendorId, kProductId,
                                    "Not Ledger", "001",
                                    device::mojom::HidBusType::kHIDBusTypeUSB);
  }

  void RemoveAllDevices() {
    hid_manager().GetDevices(base::BindLambdaForTesting(
        [this](std::vector<device::mojom::HidDeviceInfoPtr> devices) {
          for (auto& device : devices) {
            hid_manager().RemoveDevice(device->guid);
          }
        }));
  }

  void SetMainFrameOrigin(const std::string& url) {
    content::OverrideLastCommittedOrigin(main_rfh(),
                                         url::Origin::Create(GURL(url)));
  }

  void SetSubFrameOrigin(const std::string& url) {
    content::OverrideLastCommittedOrigin(subframe_rfh(),
                                         url::Origin::Create(GURL(url)));
  }

  std::unique_ptr<BraveWalletHidChooser> CreateChooser(
      content::RenderFrameHost* rfh,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
      content::HidChooser::Callback callback) {
    return std::make_unique<BraveWalletHidChooser>(rfh, std::move(filters),
                                                   std::move(exclusion_filters),
                                                   std::move(callback));
  }

  device::FakeHidManager& hid_manager() { return hid_manager_; }

 private:
  device::FakeHidManager hid_manager_;
  raw_ptr<content::RenderFrameHost> subframe_rfh_ = nullptr;
};

TEST_F(BraveWalletHidChooserTest, IsLedgerSubframeOfBraveWallet_NullRfh) {
  EXPECT_FALSE(BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(nullptr));
}

TEST_F(BraveWalletHidChooserTest, TestDefaults) {
  EXPECT_EQ(main_rfh()->GetLastCommittedOrigin(),
            url::Origin::Create(GURL("chrome://wallet/")));
  EXPECT_EQ(subframe_rfh()->GetLastCommittedOrigin(),
            url::Origin::Create(GURL("chrome-untrusted://ledger-bridge/")));
  EXPECT_TRUE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(subframe_rfh()));
}

TEST_F(BraveWalletHidChooserTest, IsLedgerSubframeOfBraveWallet_HttpsOrigin) {
  SetMainFrameOrigin("https://brave.com/");
  EXPECT_FALSE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(subframe_rfh()));
}

TEST_F(BraveWalletHidChooserTest, IsLedgerSubframeOfBraveWallet_Mainframe) {
  EXPECT_FALSE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(main_rfh()));
}

TEST_F(BraveWalletHidChooserTest, IsLedgerSubframeOfBraveWallet_WalletOrigins) {
  SetMainFrameOrigin("chrome://wallet/");
  EXPECT_TRUE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(subframe_rfh()));

  SetMainFrameOrigin("chrome://wallet-panel.top-chrome/");
  EXPECT_TRUE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(subframe_rfh()));
}

TEST_F(BraveWalletHidChooserTest,
       IsLedgerSubframeOfBraveWallet_OtherChromeUIHost) {
  SetMainFrameOrigin("chrome://settings/");
  EXPECT_FALSE(
      BraveWalletHidChooser::IsLedgerSubframeOfBraveWallet(subframe_rfh()));
}

TEST_F(BraveWalletHidChooserTest, EmptyFiltersCancels) {
  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser = CreateChooser(subframe_rfh(), {}, {}, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, OtherVendorFilterCancels) {
  std::vector<blink::mojom::HidDeviceFilterPtr> filters;
  filters.push_back(blink::mojom::HidDeviceFilter::New(
      blink::mojom::DeviceIdFilter::NewVendor(kOtherVendorId), nullptr));

  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser = CreateChooser(subframe_rfh(), std::move(filters), {},
                               future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, MultipleFiltersCancels) {
  auto filters = MakeLedgerVendorOnlyFilters();
  filters.push_back(blink::mojom::HidDeviceFilter::New(
      blink::mojom::DeviceIdFilter::NewVendor(kOtherVendorId), nullptr));

  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser = CreateChooser(subframe_rfh(), std::move(filters), {},
                               future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, ExclusionFiltersCancels) {
  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser =
      CreateChooser(subframe_rfh(), MakeLedgerVendorOnlyFilters(),
                    MakeLedgerVendorOnlyFilters(), future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, VendorAndProductFilterCancels) {
  std::vector<blink::mojom::HidDeviceFilterPtr> filters;
  filters.push_back(blink::mojom::HidDeviceFilter::New(
      blink::mojom::DeviceIdFilter::NewVendorAndProduct(
          blink::mojom::VendorAndProduct::New(kLedgerVendorId, kProductId)),
      nullptr));

  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser = CreateChooser(subframe_rfh(), std::move(filters), {},
                               future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, ValidLedgerFilterNoDevicesCancels) {
  RemoveAllDevices();

  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;
  auto chooser = CreateChooser(subframe_rfh(), MakeLedgerVendorOnlyFilters(),
                               {}, future.GetCallback());
  EXPECT_TRUE(future.Get().empty());
}

TEST_F(BraveWalletHidChooserTest, ValidLedgerFilterSelectsFirstDevice) {
  TestFuture<std::vector<device::mojom::HidDeviceInfoPtr>> future;

  auto chooser = CreateChooser(subframe_rfh(), MakeLedgerVendorOnlyFilters(),
                               {}, future.GetCallback());

  const auto& result = future.Get();
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0]->vendor_id, kLedgerVendorId);
  // Devices come in arbitrary order, so we expect either of the two Ledger
  // devices to be selected.
  EXPECT_THAT(result[0]->physical_device_id,
              testing::AnyOf("ledger", "another_ledger"));
}

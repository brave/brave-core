/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "base/path_service.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/chooser_bubble_testapi.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/usb/chrome_usb_delegate.h"
#include "chrome/browser/usb/usb_chooser_context_factory.h"
#include "chrome/browser/usb/usb_chooser_controller.h"
#include "chrome/browser/usb/web_usb_chooser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/permissions/permission_request.h"
#include "content/public/browser/usb_chooser.h"
#include "content/public/common/content_client.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "services/device/public/cpp/test/fake_usb_device_manager.h"
#include "services/device/public/mojom/usb_device.mojom.h"
#include "services/device/public/mojom/usb_enumeration_options.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/mojom/usb/web_usb_service.mojom.h"

using brave_shields::ControlType;

namespace {

constexpr char kTestDeviceSerialNumber[] = "123456";
constexpr char kGetDevicesScript[] = R"((async () => {
        let devices = await navigator.usb.getDevices();
        return devices.map(device => device.serialNumber);
      })())";
constexpr char kRequestDeviceScript[] = R"((async () => {
        let device =
            await navigator.usb.requestDevice({ filters: [{ vendorId: 0 }] });
        return device.serialNumber;
      })())";

class FakeChooserView : public permissions::ChooserController::View {
 public:
  explicit FakeChooserView(
      std::unique_ptr<permissions::ChooserController> controller)
      : controller_(std::move(controller)) {
    controller_->set_view(this);
  }

  FakeChooserView(const FakeChooserView&) = delete;
  FakeChooserView& operator=(const FakeChooserView&) = delete;

  ~FakeChooserView() override { controller_->set_view(nullptr); }

  void OnOptionsInitialized() override {
    if (controller_->NumOptions()) {
      controller_->Select({0});
    } else {
      controller_->Cancel();
    }
    delete this;
  }

  void OnOptionAdded(size_t index) override { NOTREACHED(); }
  void OnOptionRemoved(size_t index) override { NOTREACHED(); }
  void OnOptionUpdated(size_t index) override { NOTREACHED(); }
  void OnAdapterEnabledChanged(bool enabled) override { NOTREACHED(); }
  void OnRefreshStateChanged(bool refreshing) override { NOTREACHED(); }

 private:
  std::unique_ptr<permissions::ChooserController> controller_;
};

class FakeUsbChooser : public WebUsbChooser {
 public:
  FakeUsbChooser() = default;
  FakeUsbChooser(const FakeUsbChooser&) = delete;
  FakeUsbChooser& operator=(const FakeUsbChooser&) = delete;
  ~FakeUsbChooser() override = default;

  void ShowChooser(content::RenderFrameHost* frame,
                   std::unique_ptr<UsbChooserController> controller) override {
    // Device list initialization in UsbChooserController may complete before
    // having a valid view in which case OnOptionsInitialized() has no chance to
    // be triggered, so select the first option directly if options are ready.
    if (controller->NumOptions()) {
      controller->Select({0});
    } else {
      new FakeChooserView(std::move(controller));
    }
  }
};

class TestUsbDelegate : public ChromeUsbDelegate {
 public:
  TestUsbDelegate() = default;
  TestUsbDelegate(const TestUsbDelegate&) = delete;
  TestUsbDelegate& operator=(const TestUsbDelegate&) = delete;
  ~TestUsbDelegate() override = default;

  std::unique_ptr<content::UsbChooser> RunChooser(
      content::RenderFrameHost& frame,
      blink::mojom::WebUsbRequestDeviceOptionsPtr options,
      blink::mojom::WebUsbService::GetPermissionCallback callback) override {
    if (use_fake_chooser_) {
      auto chooser = std::make_unique<FakeUsbChooser>();
      chooser->ShowChooser(
          &frame, std::make_unique<UsbChooserController>(
                      &frame, std::move(options), std::move(callback)));
      return chooser;
    } else {
      return ChromeUsbDelegate::RunChooser(frame, std::move(options),
                                           std::move(callback));
    }
  }

  void UseFakeChooser() { use_fake_chooser_ = true; }

 private:
  bool use_fake_chooser_ = false;
};

class TestContentBrowserClient : public BraveContentBrowserClient {
 public:
  TestContentBrowserClient()
      : usb_delegate_(std::make_unique<TestUsbDelegate>()) {}
  TestContentBrowserClient(const TestContentBrowserClient&) = delete;
  TestContentBrowserClient& operator=(const TestContentBrowserClient&) = delete;
  ~TestContentBrowserClient() override = default;

  // ChromeContentBrowserClient:
  content::UsbDelegate* GetUsbDelegate() override {
    return usb_delegate_.get();
  }

  TestUsbDelegate& delegate() { return *usb_delegate_; }

  void ResetUsbDelegate() { usb_delegate_.reset(); }

 private:
  std::unique_ptr<TestUsbDelegate> usb_delegate_;
};

class BraveNavigatorUsbFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorUsbFarblingBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        webcompat::features::kBraveWebcompatExceptionsService);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    browser_content_client_ = std::make_unique<TestContentBrowserClient>();
    content::SetBrowserClientForTesting(browser_content_client_.get());

    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    content::SetupCrossSiteRedirector(https_server_.get());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_->Start());

    // Connect with the FakeUsbDeviceManager.
    mojo::PendingRemote<device::mojom::UsbDeviceManager> device_manager;
    device_manager_.AddReceiver(
        device_manager.InitWithNewPipeAndPassReceiver());
    UsbChooserContextFactory::GetForProfile(browser()->profile())
        ->SetDeviceManagerForTesting(std::move(device_manager));
  }

  void TearDownOnMainThread() override {
    browser_content_client_->ResetUsbDelegate();
  }

  void TearDown() override { browser_content_client_.reset(); }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server()->GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server()->GetURL(domain, "/"));
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void AddFakeDevice(const std::string& serial_number) {
    DCHECK(!fake_device_info_);
    fake_device_info_ = device_manager_.CreateAndAddDevice(
        0, 0, "Test Manufacturer", "Test Device", serial_number);
  }

  void RemoveFakeDevice() {
    DCHECK(fake_device_info_);
    device_manager_.RemoveDevice(fake_device_info_->guid);
    fake_device_info_ = nullptr;
  }

  void UseFakeChooser() {
    browser_content_client_->delegate().UseFakeChooser();
  }

  UsbChooserContext* GetChooserContext() {
    return UsbChooserContextFactory::GetForProfile(browser()->profile());
  }

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<TestContentBrowserClient> browser_content_client_;
  device::FakeUsbDeviceManager device_manager_;
  device::mojom::UsbDeviceInfoPtr fake_device_info_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveNavigatorUsbFarblingBrowserTest,
                       FarbleSerialNumber) {
  // Insert a fake USB device.
  AddFakeDevice(kTestDeviceSerialNumber);

  // Navigate with farbling off.
  std::string domain_b = "b.com";
  GURL url_b = https_server()->GetURL(domain_b, "/simple.html");
  // Farbling level: off
  AllowFingerprinting(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));

  // Call getDevices with no device permissions. This should return an empty
  // list.
  EXPECT_EQ(content::ListValueOf(), EvalJs(web_contents(), kGetDevicesScript));

  // Request permission to access a USB device. The fake chooser view will
  // automatically select the item representing the fake device we created and
  // grant permission.
  UseFakeChooser();

  // Request the device configuration and check its serial number. This
  // should be the actual serial number we assigned when we created the device.
  EXPECT_EQ(kTestDeviceSerialNumber,
            EvalJs(web_contents(), kRequestDeviceScript));

  // Call getDevices again. Our fake device should be included, still with
  // the actual serial number we assigned when we created the device.
  EXPECT_EQ(content::ListValueOf(kTestDeviceSerialNumber),
            EvalJs(web_contents(), kGetDevicesScript));

  // Reload with farbling at default.
  SetFingerprintingDefault(domain_b);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));

  // Call getDevices again. The fake device is still included, but now its
  // serial number is farbled.
  EXPECT_EQ(content::ListValueOf("dt9mTRQnb057d1a0"),
            EvalJs(web_contents(), kGetDevicesScript));

  // Do it all again, but on a different domain.
  std::string domain_z = "z.com";
  GURL url_z = https_server()->GetURL(domain_z, "/simple.html");
  SetFingerprintingDefault(domain_z);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
  EXPECT_EQ("Qv2Eh368mTRQv26G", EvalJs(web_contents(), kRequestDeviceScript));

  // Reload once more with farbling at default but enable a webcompat exception.
  SetFingerprintingDefault(domain_b);
  brave_shields::SetWebcompatEnabled(
      content_settings(),
      ContentSettingsType::BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER, true,
      GURL(url_b), nullptr);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));

  // Call getDevices again. The fake device is still included, but now its
  // serial number is not farbled.
  EXPECT_EQ(content::ListValueOf(kTestDeviceSerialNumber),
            EvalJs(web_contents(), kGetDevicesScript));
}

}  // namespace

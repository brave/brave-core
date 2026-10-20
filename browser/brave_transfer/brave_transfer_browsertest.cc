// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_transfer/brave_transfer_service_factory.h"
#include "brave/browser/brave_transfer/mojo_byte_stream.h"
#include "brave/browser/brave_transfer/tcp_connector.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/brave_transfer_button.h"
#include "brave/components/brave_transfer/browser/brave_transfer_service.h"
#include "brave/components/brave_transfer/browser/directory_file_sink.h"
#include "brave/components/brave_transfer/browser/test/fakes.h"
#include "brave/components/brave_transfer/core/features.h"
#include "brave/components/brave_transfer/core/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/views/test/button_test_api.h"
#include "url/gurl.h"

namespace brave_transfer {

namespace {

constexpr char kMacGuid[] = "mac-guid";
constexpr char kPhoneGuid[] = "phone-guid";

ChainKeys KeysFromByte(uint8_t value) {
  return *ChainKeys::FromSeed(std::vector<uint8_t>(32, value));
}

// A service with real sockets and file saving, and fake Sync and discovery.
struct LoopbackDevice {
  LoopbackDevice(const std::string& guid,
                 const std::string& name,
                 const base::FilePath& downloads,
                 os_crypt_async::OSCryptAsync* os_crypt_async,
                 Profile* profile) {
    BraveTransferService::RegisterProfilePrefs(prefs.registry());
    prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
    auto trust_source = std::make_unique<FakeTrustSource>();
    trust = trust_source.get();
    trust->SetLocalDevice(guid, name);
    trust->SetChainKeys(KeysFromByte(5));
    auto discovery_provider = std::make_unique<FakeDiscoveryProvider>();
    discovery = discovery_provider.get();
    service = std::make_unique<BraveTransferService>(
        &prefs, base::FilePath(), os_crypt_async, std::move(trust_source),
        std::move(discovery_provider),
        std::make_unique<TcpConnector>(base::BindRepeating(
            [](Profile* profile) {
              return profile->GetDefaultStoragePartition()->GetNetworkContext();
            },
            base::Unretained(profile))),
        std::make_unique<DirectoryFileSink>(
            base::BindRepeating(
                [](base::FilePath downloads) {
                  DirectoryFileSink::Directories directories;
                  directories.fallback = downloads;
                  return directories;
                },
                downloads),
            DirectoryFileSink::FinalizeCallback()),
        std::make_unique<FakeServiceDelegate>(downloads, &counts));
  }

  ~LoopbackDevice() {
    service->Shutdown();
    trust = nullptr;
    discovery = nullptr;
    service.reset();
  }

  TestingPrefServiceSimple prefs;
  FakeServiceDelegate::Counts counts;
  raw_ptr<FakeTrustSource> trust = nullptr;
  raw_ptr<FakeDiscoveryProvider> discovery = nullptr;
  std::unique_ptr<BraveTransferService> service;
};

}  // namespace

class BraveTransferBrowserTest : public InProcessBrowserTest {
 public:
  BraveTransferBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveTransfer);
  }

 protected:
  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  // Listens on loopback and hands accepted connections to `device`.
  uint16_t Listen(LoopbackDevice& device) {
    base::test::TestFuture<int32_t, const std::optional<net::IPEndPoint>&>
        listening;
    auto options = network::mojom::TCPServerSocketOptions::New();
    options->backlog = 5;
    browser()
        ->GetProfile()
        ->GetDefaultStoragePartition()
        ->GetNetworkContext()
        ->CreateTCPServerSocket(
            net::IPEndPoint(net::IPAddress::IPv4Localhost(), 0),
            std::move(options),
            net::MutableNetworkTrafficAnnotationTag(
                TRAFFIC_ANNOTATION_FOR_TESTS),
            server_socket_.BindNewPipeAndPassReceiver(),
            listening.GetCallback());
    EXPECT_EQ(listening.Get<0>(), net::OK);
    Accept(device);
    return listening.Get<1>()->port();
  }

  void Accept(LoopbackDevice& device) {
    server_socket_->Accept(
        mojo::NullRemote(),
        base::BindLambdaForTesting(
            [this, &device](
                int32_t result, const std::optional<net::IPEndPoint>&,
                mojo::PendingRemote<network::mojom::TCPConnectedSocket> socket,
                mojo::ScopedDataPipeConsumerHandle receive_stream,
                mojo::ScopedDataPipeProducerHandle send_stream) {
              if (result != net::OK) {
                return;
              }
              device.service->AcceptIncoming(std::make_unique<MojoByteStream>(
                  std::move(socket),
                  mojo::PendingReceiver<network::mojom::SocketObserver>(),
                  std::move(receive_stream), std::move(send_stream)));
              Accept(device);
            }));
  }

  base::test::ScopedFeatureList feature_list_;
  mojo::Remote<network::mojom::TCPServerSocket> server_socket_;
};

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, PageLoadsAndBindsHandler) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveTransferURL)));
  EXPECT_TRUE(web_contents()->GetWebUI());
  EXPECT_TRUE(
      BraveTransferServiceFactory::GetForProfile(browser()->GetProfile()));
  EXPECT_TRUE(base::test::RunUntil([&] {
    return content::EvalJs(web_contents(),
                           "!!document.getElementById('root')?.firstChild")
        .ExtractBool();
  }));
}

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, NotAvailableInIncognito) {
  BrowserWindowInterface* incognito = CreateIncognitoBrowser();
  EXPECT_FALSE(
      BraveTransferServiceFactory::GetForProfile(incognito->GetProfile()));
  std::ignore =
      ui_test_utils::NavigateToURL(incognito, GURL(kBraveTransferURL));
  EXPECT_FALSE(
      incognito->GetTabStripModel()->GetActiveWebContents()->GetWebUI());
}

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, PolicyDisablesPage) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kEnabled, false);
  std::ignore =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveTransferURL));
  EXPECT_FALSE(web_contents()->GetWebUI());
}

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, ToolbarButton) {
  auto* toolbar = static_cast<BraveToolbarView*>(
      BrowserView::GetBrowserViewForBrowser(browser())->toolbar());
  BraveTransferButton* button = toolbar->brave_transfer_button();
  ASSERT_TRUE(button);
  EXPECT_TRUE(button->GetVisible());

  PrefService* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetBoolean(prefs::kShowToolbarButton, false);
  EXPECT_FALSE(button->GetVisible());
  prefs->SetBoolean(prefs::kShowToolbarButton, true);
  EXPECT_TRUE(button->GetVisible());
  prefs->SetBoolean(prefs::kEnabled, false);
  EXPECT_FALSE(button->GetVisible());
  prefs->SetBoolean(prefs::kEnabled, true);
  EXPECT_TRUE(button->GetVisible());

  const int tab_count = browser()->tab_strip_model()->count();
  views::test::ButtonTestApi(button).NotifyDefaultMouseClick();
  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count + 1);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents()));
  EXPECT_EQ(web_contents()->GetLastCommittedURL().host(), kBraveTransferHost);

  views::test::ButtonTestApi(button).NotifyDefaultMouseClick();
  EXPECT_EQ(browser()->tab_strip_model()->count(), tab_count + 1);
}

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, NoToolbarButtonInIncognito) {
  BrowserWindowInterface* incognito = CreateIncognitoBrowser();
  auto* toolbar = static_cast<BraveToolbarView*>(
      BrowserView::GetBrowserViewForBrowser(incognito)->toolbar());
  EXPECT_FALSE(toolbar->brave_transfer_button());
}

IN_PROC_BROWSER_TEST_F(BraveTransferBrowserTest, TransfersOverLoopback) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  const base::FilePath phone_downloads = temp_dir.GetPath().AppendASCII("p");
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async =
      os_crypt_async::GetTestOSCryptAsyncForTesting(
          /*is_sync_for_unittests=*/true);

  auto mac = std::make_unique<LoopbackDevice>(
      kMacGuid, "MacBook", temp_dir.GetPath().AppendASCII("m"),
      os_crypt_async.get(), browser()->GetProfile());
  auto phone = std::make_unique<LoopbackDevice>(
      kPhoneGuid, "iPhone", phone_downloads, os_crypt_async.get(),
      browser()->GetProfile());
  mac->trust->SetChainDevices(
      {{kPhoneGuid, "iPhone", mojom::DeviceType::kPhone}});
  phone->trust->SetChainDevices(
      {{kMacGuid, "MacBook", mojom::DeviceType::kLaptop}});
  ASSERT_TRUE(base::test::RunUntil([&] {
    return mac->service->has_static_key() && phone->service->has_static_key();
  }));

  const uint16_t port = Listen(*phone);
  mac->service->AddPage();
  mac->discovery->AddCandidate("iphone._brave-xfer._tcp.local",
                               {{"127.0.0.1", port}});
  ASSERT_TRUE(base::test::RunUntil([&] {
    std::vector<mojom::DevicePtr> devices = mac->service->GetDevices();
    return devices.size() == 1 && devices[0]->is_connected;
  }));

  const std::vector<uint8_t> photo(3 * 1024 * 1024, 7);
  std::vector<mojom::FileDataPtr> files;
  files.push_back(mojom::FileData::New("photo.jpg", "image/jpeg", photo));
  EXPECT_EQ(mac->service->SendFiles(kPhoneGuid, std::move(files)),
            mojom::SendResult::kSuccess);
  EXPECT_EQ(mac->service->SendText(kPhoneGuid, "https://brave.com/"),
            mojom::SendResult::kSuccess);

  ASSERT_TRUE(
      base::test::RunUntil([&] { return phone->counts.items_received >= 2; }));
  std::optional<int64_t> size =
      base::GetFileSize(phone_downloads.AppendASCII("photo.jpg"));
  ASSERT_TRUE(size);
  EXPECT_EQ(*size, static_cast<int64_t>(photo.size()));

  base::test::TestFuture<std::vector<mojom::InboxItemPtr>, bool> inbox;
  phone->service->GetInbox(0, 10, inbox.GetCallback());
  std::vector<mojom::ItemKind> kinds;
  for (const mojom::InboxItemPtr& item : inbox.Get<0>()) {
    kinds.push_back(item->kind);
  }
  EXPECT_THAT(kinds, testing::UnorderedElementsAre(mojom::ItemKind::kLink,
                                                   mojom::ItemKind::kImage));

  mac->service->RemovePage();
  mac.reset();
  phone.reset();
}

}  // namespace brave_transfer

// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/brave_transfer_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_transfer/browser/directory_file_sink.h"
#include "brave/components/brave_transfer/browser/test/fake_byte_stream.h"
#include "brave/components/brave_transfer/browser/test/fakes.h"
#include "brave/components/brave_transfer/browser/test/run_until.h"
#include "brave/components/brave_transfer/core/pref_names.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

namespace {

constexpr char kMacGuid[] = "mac-guid";
constexpr char kPhoneGuid[] = "phone-guid";

ChainKeys KeysFromByte(uint8_t value) {
  std::vector<uint8_t> seed(32, value);
  return *ChainKeys::FromSeed(seed);
}

class TestObserver : public BraveTransferService::Observer {
 public:
  void OnOfferReceived(const mojom::IncomingOffer& offer) override {
    offers.push_back(offer.Clone());
  }
  void OnOfferRemoved(const std::string& offer_id) override {
    removed_offers.push_back(offer_id);
  }
  void OnTransferUpdated(const mojom::Transfer& transfer) override {
    last_transfer = transfer.Clone();
  }
  void OnInboxChanged() override { ++inbox_changes; }
  void OnSettingsChanged() override { ++settings_changes; }

  std::vector<mojom::IncomingOfferPtr> offers;
  std::vector<std::string> removed_offers;
  mojom::TransferPtr last_transfer;
  int inbox_changes = 0;
  int settings_changes = 0;
};

// One simulated device: a service with fakes for everything platform
// specific.
struct TestDevice {
  TestDevice(const std::string& guid,
             const std::string& name,
             const base::FilePath& downloads,
             os_crypt_async::OSCryptAsync* os_crypt_async,
             FakeConnector::Handler connect_handler) {
    BraveTransferService::RegisterProfilePrefs(prefs.registry());
    auto trust_source = std::make_unique<FakeTrustSource>();
    trust = trust_source.get();
    trust->SetLocalDevice(guid, name);
    auto discovery_provider = std::make_unique<FakeDiscoveryProvider>();
    discovery = discovery_provider.get();
    auto connector_impl =
        std::make_unique<FakeConnector>(std::move(connect_handler));
    connector = connector_impl.get();
    service = std::make_unique<BraveTransferService>(
        &prefs, base::FilePath(), os_crypt_async, std::move(trust_source),
        std::move(discovery_provider), std::move(connector_impl),
        std::make_unique<DirectoryFileSink>(
            base::BindLambdaForTesting([this, downloads] {
              DirectoryFileSink::Directories directories;
              directories.preferred = prefs.GetFilePath(prefs::kSaveDirectory);
              directories.fallback = downloads;
              return directories;
            }),
            DirectoryFileSink::FinalizeCallback()),
        std::make_unique<FakeServiceDelegate>(downloads, &counts));
    service->AddObserver(&observer);
  }

  ~TestDevice() {
    service->RemoveObserver(&observer);
    service->Shutdown();
    trust = nullptr;
    discovery = nullptr;
    connector = nullptr;
    service.reset();
  }

  std::vector<mojom::InboxItemPtr> Inbox() {
    base::test::TestFuture<std::vector<mojom::InboxItemPtr>, bool> future;
    service->GetInbox(0, 100, future.GetCallback());
    return std::get<0>(future.Take());
  }

  TestingPrefServiceSimple prefs;
  FakeServiceDelegate::Counts counts;
  TestObserver observer;
  raw_ptr<FakeTrustSource> trust = nullptr;
  raw_ptr<FakeDiscoveryProvider> discovery = nullptr;
  raw_ptr<FakeConnector> connector = nullptr;
  std::unique_ptr<BraveTransferService> service;
};

}  // namespace

class BraveTransferServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    mac_downloads_ = temp_dir_.GetPath().AppendASCII("mac");
    phone_downloads_ = temp_dir_.GetPath().AppendASCII("phone");
    os_crypt_async_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);

    phone_ = std::make_unique<TestDevice>(
        kPhoneGuid, "iPhone", phone_downloads_, os_crypt_async_.get(),
        base::BindRepeating(
            [](const std::vector<PeerEndpoint>&)
                -> std::unique_ptr<ByteStream> { return nullptr; }));
    mac_ = std::make_unique<TestDevice>(
        kMacGuid, "MacBook", mac_downloads_, os_crypt_async_.get(),
        base::BindLambdaForTesting(
            [this](const std::vector<PeerEndpoint>& endpoints)
                -> std::unique_ptr<ByteStream> {
              if (!phone_listening_) {
                return nullptr;
              }
              auto [mac_end, phone_end] = FakeByteStream::CreatePair();
              phone_->service->AcceptIncoming(std::move(phone_end));
              return std::move(mac_end);
            }));

    const ChainKeys keys = KeysFromByte(9);
    for (TestDevice* device : {mac_.get(), phone_.get()}) {
      device->trust->SetChainKeys(keys);
    }
    mac_->trust->SetChainDevices(
        {{kPhoneGuid, "iPhone", mojom::DeviceType::kPhone}});
    phone_->trust->SetChainDevices(
        {{kMacGuid, "MacBook", mojom::DeviceType::kLaptop}});
    ASSERT_TRUE(mac_->service->has_static_key());
    ASSERT_TRUE(phone_->service->has_static_key());
  }

  void TearDown() override {
    mac_.reset();
    phone_.reset();
  }

  bool MacSeesPhone() {
    for (const mojom::DevicePtr& device : mac_->service->GetDevices()) {
      if (device->id == kPhoneGuid) {
        return device->is_connected;
      }
    }
    return false;
  }

  // Opens brave://transfer on the Mac and lets it find the phone.
  void ConnectDevices() {
    mac_->service->AddPage();
    ASSERT_TRUE(mac_->discovery->IsStarted());
    mac_->discovery->AddCandidate("iphone._brave-xfer._tcp.local",
                                  {{"192.168.1.20", 52000}});
    ASSERT_TRUE(RunUntilWithMockTime(task_environment_,
                                     [&] { return MacSeesPhone(); }));
  }

  mojom::TransferPtr WaitForMacTransfer(mojom::TransferState state) {
    EXPECT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
      return mac_->observer.last_transfer &&
             mac_->observer.last_transfer->state == state;
    }));
    return mac_->observer.last_transfer.Clone();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  base::FilePath mac_downloads_;
  base::FilePath phone_downloads_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  bool phone_listening_ = true;
  std::unique_ptr<TestDevice> phone_;
  std::unique_ptr<TestDevice> mac_;
};

TEST_F(BraveTransferServiceTest, DefaultSettings) {
  mojom::SettingsPtr settings = mac_->service->GetSettings();
  EXPECT_EQ(settings->receive_mode, mojom::ReceiveMode::kTrustedDevices);
  EXPECT_FALSE(settings->auto_accept_trusted);
  EXPECT_TRUE(settings->auto_trust_sync_chain);
  EXPECT_TRUE(settings->notify_on_receive);
  EXPECT_TRUE(settings->save_directory.empty());
  EXPECT_EQ(settings->default_save_directory, mac_downloads_.AsUTF8Unsafe());
  EXPECT_TRUE(mac_->service->IsEnabled());
}

TEST_F(BraveTransferServiceTest, StaticKeyIsStoredEncrypted) {
  const std::string stored = mac_->prefs.GetString(prefs::kStaticKey);
  ASSERT_FALSE(stored.empty());
  std::optional<std::vector<uint8_t>> decoded = base::Base64Decode(stored);
  ASSERT_TRUE(decoded);
  EXPECT_NE(decoded->size(), kKeySize);
}

TEST_F(BraveTransferServiceTest, DiscoveryOnlyWhilePageOpen) {
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
  mac_->service->AddPage();
  mac_->service->AddPage();
  EXPECT_TRUE(mac_->service->IsDiscoveryActive());
  EXPECT_TRUE(mac_->service->GetServiceState()->discovery_active);
  mac_->service->RemovePage();
  EXPECT_TRUE(mac_->service->IsDiscoveryActive());
  mac_->service->RemovePage();
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
}

TEST_F(BraveTransferServiceTest, NoDiscoveryWithoutSync) {
  mac_->trust->SetSyncActive(false);
  mac_->service->AddPage();
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
  EXPECT_FALSE(mac_->service->GetServiceState()->sync_enabled);
  EXPECT_TRUE(mac_->service->GetDevices().empty());

  mac_->trust->SetSyncActive(true);
  EXPECT_TRUE(mac_->service->IsDiscoveryActive());
  mac_->service->RemovePage();
}

TEST_F(BraveTransferServiceTest, ConnectsAndPinsKeys) {
  ConnectDevices();
  std::vector<mojom::DevicePtr> devices = mac_->service->GetDevices();
  ASSERT_EQ(devices.size(), 1u);
  EXPECT_EQ(devices[0]->name, "iPhone");
  EXPECT_EQ(devices[0]->type, mojom::DeviceType::kPhone);
  EXPECT_EQ(devices[0]->source, mojom::TrustSource::kSyncChain);
  EXPECT_EQ(devices[0]->policy, mojom::DevicePolicy::kDefault);
  EXPECT_FALSE(devices[0]->key_fingerprint.empty());
  EXPECT_GT(devices[0]->last_seen_time, 0);

  const base::DictValue* pinned =
      mac_->prefs.GetDict(prefs::kTrustedDevices).FindDict(kPhoneGuid);
  ASSERT_TRUE(pinned);
  EXPECT_TRUE(pinned->FindString("key"));
  EXPECT_TRUE(phone_->prefs.GetDict(prefs::kTrustedDevices).FindDict(kMacGuid));
}

TEST_F(BraveTransferServiceTest, PeerOutsideChainIsRejected) {
  phone_->trust->SetChainDevices({});
  mac_->service->AddPage();
  mac_->discovery->AddCandidate("iphone", {{"192.168.1.20", 52000}});
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return mac_->connector->connect_count() >= 1 &&
           phone_->prefs.GetDict(prefs::kTrustedDevices).empty();
  }));
  task_environment_.FastForwardBy(base::Seconds(2));
  EXPECT_FALSE(MacSeesPhone());
  EXPECT_TRUE(mac_->prefs.GetDict(prefs::kTrustedDevices).empty());
  EXPECT_TRUE(phone_->prefs.GetDict(prefs::kTrustedDevices).empty());
}

TEST_F(BraveTransferServiceTest, KeyMismatchIsRejected) {
  {
    ScopedDictPrefUpdate update(&mac_->prefs, prefs::kTrustedDevices);
    update->EnsureDict(kPhoneGuid)
        ->Set("key", base::Base64Encode(std::vector<uint8_t>(32, 1)));
  }
  mac_->service->AddPage();
  mac_->discovery->AddCandidate("iphone", {{"192.168.1.20", 52000}});
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return mac_->counts.key_mismatches > 0; }));
  EXPECT_FALSE(MacSeesPhone());
}

TEST_F(BraveTransferServiceTest, RetriesFailedDials) {
  phone_listening_ = false;
  mac_->service->AddPage();
  mac_->discovery->AddCandidate("iphone", {{"192.168.1.20", 52000}});
  task_environment_.RunUntilIdle();
  EXPECT_EQ(mac_->connector->connect_count(), 1);
  phone_listening_ = true;
  task_environment_.FastForwardBy(BraveTransferService::kMinRetryDelay);
  ASSERT_TRUE(
      RunUntilWithMockTime(task_environment_, [&] { return MacSeesPhone(); }));
  EXPECT_EQ(mac_->connector->connect_count(), 2);
}

TEST_F(BraveTransferServiceTest, SendTextAskThenAccept) {
  ConnectDevices();
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, "hello iPhone"),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !phone_->observer.offers.empty(); }));
  EXPECT_EQ(phone_->counts.offers_pending, 1);
  std::vector<mojom::IncomingOfferPtr> pending =
      phone_->service->GetPendingOffers();
  ASSERT_EQ(pending.size(), 1u);
  EXPECT_EQ(pending[0]->device_id, kMacGuid);
  EXPECT_EQ(pending[0]->device_name, "MacBook");
  ASSERT_EQ(pending[0]->items.size(), 1u);
  EXPECT_EQ(pending[0]->items[0]->kind, mojom::ItemKind::kText);
  EXPECT_EQ(pending[0]->items[0]->text, "hello iPhone");
  EXPECT_EQ(mac_->observer.last_transfer->state,
            mojom::TransferState::kWaitingForResponse);

  EXPECT_TRUE(phone_->service->RespondToOffer(pending[0]->id, true));
  EXPECT_FALSE(phone_->service->RespondToOffer(pending[0]->id, true));
  WaitForMacTransfer(mojom::TransferState::kCompleted);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->observer.inbox_changes > 0; }));

  std::vector<mojom::InboxItemPtr> inbox = phone_->Inbox();
  ASSERT_EQ(inbox.size(), 1u);
  EXPECT_EQ(inbox[0]->kind, mojom::ItemKind::kText);
  EXPECT_EQ(inbox[0]->text, "hello iPhone");
  EXPECT_EQ(inbox[0]->device_name, "MacBook");
  EXPECT_EQ(phone_->counts.items_received, 1);
  EXPECT_TRUE(phone_->service->GetPendingOffers().empty());
}

TEST_F(BraveTransferServiceTest, LinkIsSentAsLink) {
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, "  https://brave.com/  "),
            mojom::SendResult::kSuccess);
  WaitForMacTransfer(mojom::TransferState::kCompleted);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->observer.inbox_changes > 0; }));
  std::vector<mojom::InboxItemPtr> inbox = phone_->Inbox();
  ASSERT_EQ(inbox.size(), 1u);
  EXPECT_EQ(inbox[0]->kind, mojom::ItemKind::kLink);
  EXPECT_EQ(inbox[0]->text, "https://brave.com/");
  EXPECT_EQ(phone_->counts.offers_pending, 0);
}

TEST_F(BraveTransferServiceTest, AutoAcceptedFilesAreSaved) {
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  std::vector<mojom::FileDataPtr> files;
  files.push_back(mojom::FileData::New("photo.jpg", "image/jpeg",
                                       std::vector<uint8_t>(150000, 3)));
  files.push_back(
      mojom::FileData::New("../notes.txt", "text/plain",
                           std::vector<uint8_t>{'n', 'o', 't', 'e', 's'}));
  files.push_back(
      mojom::FileData::New("empty.bin", "", std::vector<uint8_t>()));
  EXPECT_EQ(mac_->service->SendFiles(kPhoneGuid, std::move(files)),
            mojom::SendResult::kSuccess);
  mojom::TransferPtr sent =
      WaitForMacTransfer(mojom::TransferState::kCompleted);
  EXPECT_EQ(sent->bytes_total, 150005u);
  EXPECT_EQ(sent->bytes_transferred, sent->bytes_total);

  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->Inbox().size() == 3; }));
  std::vector<mojom::InboxItemPtr> inbox = phone_->Inbox();
  for (const mojom::InboxItemPtr& item : inbox) {
    EXPECT_FALSE(item->file_missing);
    EXPECT_FALSE(item->saved_to_fallback);
  }
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(phone_downloads_.AppendASCII("_notes.txt"),
                                     &contents));
  EXPECT_EQ(contents, "notes");
  EXPECT_EQ(base::GetFileSize(phone_downloads_.AppendASCII("photo.jpg")),
            150000);
  EXPECT_TRUE(base::PathExists(phone_downloads_.AppendASCII("empty.bin")));
  EXPECT_FALSE(base::PathExists(temp_dir_.GetPath().AppendASCII("notes.txt")));

  base::test::TestFuture<std::optional<InboxEntry>> entry;
  phone_->service->GetInboxEntry(inbox[0]->id, entry.GetCallback());
  ASSERT_TRUE(entry.Get());
  EXPECT_TRUE(base::PathExists(entry.Get()->file_path));
}

TEST_F(BraveTransferServiceTest, LongTextIsSavedAsFile) {
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  const std::string medium(20000, 'm');
  const std::string large(BraveTransferService::kMaxInboxTextBytes + 1, 'l');
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, medium),
            mojom::SendResult::kSuccess);
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, large),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->Inbox().size() == 2; }));
  std::vector<mojom::InboxItemPtr> inbox = phone_->Inbox();
  bool saw_medium = false;
  bool saw_large = false;
  for (const mojom::InboxItemPtr& item : inbox) {
    if (item->kind == mojom::ItemKind::kText) {
      EXPECT_EQ(item->text, medium);
      saw_medium = true;
    } else {
      EXPECT_EQ(item->kind, mojom::ItemKind::kFile);
      EXPECT_EQ(item->file_name, "text.txt");
      EXPECT_EQ(item->size, large.size());
      saw_large = true;
    }
  }
  EXPECT_TRUE(saw_medium);
  EXPECT_TRUE(saw_large);
}

TEST_F(BraveTransferServiceTest, DeclineAndPolicies) {
  ConnectDevices();
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "one"),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !phone_->observer.offers.empty(); }));
  EXPECT_TRUE(
      phone_->service->RespondToOffer(phone_->observer.offers[0]->id, false));
  WaitForMacTransfer(mojom::TransferState::kDeclined);

  EXPECT_TRUE(
      phone_->service->SetDevicePolicy(kMacGuid, mojom::DevicePolicy::kBlock));
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "two"),
            mojom::SendResult::kSuccess);
  WaitForMacTransfer(mojom::TransferState::kDeclined);
  EXPECT_EQ(phone_->observer.offers.size(), 1u);

  EXPECT_TRUE(phone_->service->SetDevicePolicy(
      kMacGuid, mojom::DevicePolicy::kAlwaysAccept));
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "three"),
            mojom::SendResult::kSuccess);
  WaitForMacTransfer(mojom::TransferState::kCompleted);

  phone_->prefs.SetInteger(prefs::kReceiveMode,
                           static_cast<int>(mojom::ReceiveMode::kOff));
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "four"),
            mojom::SendResult::kSuccess);
  WaitForMacTransfer(mojom::TransferState::kDeclined);
  EXPECT_EQ(phone_->observer.offers.size(), 1u);
}

TEST_F(BraveTransferServiceTest, OfferExpires) {
  ConnectDevices();
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "anyone?"),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !phone_->observer.offers.empty(); }));
  task_environment_.FastForwardBy(kOfferTimeout + base::Seconds(2));
  WaitForMacTransfer(mojom::TransferState::kExpired);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return !phone_->observer.removed_offers.empty();
  }));
  EXPECT_TRUE(phone_->service->GetPendingOffers().empty());
}

TEST_F(BraveTransferServiceTest, SendErrors) {
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, "hi"),
            mojom::SendResult::kDeviceUnavailable);
  ConnectDevices();
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, ""),
            mojom::SendResult::kFailed);
  EXPECT_EQ(mac_->service->SendText("unknown", "hi"),
            mojom::SendResult::kDeviceUnavailable);
  EXPECT_EQ(mac_->service->SendFiles(kPhoneGuid, {}),
            mojom::SendResult::kFailed);
  std::vector<mojom::FileDataPtr> files;
  files.push_back(mojom::FileData::New(
      "huge.bin", "",
      std::vector<uint8_t>(BraveTransferService::kMaxSendBytes + 1)));
  EXPECT_EQ(mac_->service->SendFiles(kPhoneGuid, std::move(files)),
            mojom::SendResult::kTooLarge);
}

TEST_F(BraveTransferServiceTest, QueuedSendsRunInOrder) {
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  for (const char* text : {"first", "second", "third"}) {
    ASSERT_EQ(mac_->service->SendText(kPhoneGuid, text),
              mojom::SendResult::kSuccess);
  }
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->Inbox().size() == 3; }));
  std::vector<mojom::InboxItemPtr> inbox = phone_->Inbox();
  EXPECT_EQ(inbox[0]->text, "third");
  EXPECT_EQ(inbox[2]->text, "first");
  for (const mojom::TransferPtr& transfer : mac_->service->GetTransfers()) {
    EXPECT_EQ(transfer->state, mojom::TransferState::kCompleted);
    EXPECT_EQ(transfer->direction, mojom::TransferDirection::kSend);
  }
}

TEST_F(BraveTransferServiceTest, CancelOutgoing) {
  ConnectDevices();
  ASSERT_EQ(mac_->service->SendText(kPhoneGuid, "wait"),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return !phone_->observer.offers.empty(); }));
  mac_->service->CancelTransfer(mac_->observer.last_transfer->id);
  WaitForMacTransfer(mojom::TransferState::kCancelled);
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return phone_->service->GetPendingOffers().empty();
  }));
}

TEST_F(BraveTransferServiceTest, SyncOffClosesConnections) {
  ConnectDevices();
  mac_->trust->SetSyncActive(false);
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    return phone_->service->GetDevices()[0]->is_connected == false;
  }));
  EXPECT_TRUE(mac_->service->GetDevices().empty());
}

TEST_F(BraveTransferServiceTest, DevicesLeavingChainDropPins) {
  ConnectDevices();
  mac_->trust->SetChainDevices({});
  EXPECT_FALSE(
      mac_->prefs.GetDict(prefs::kTrustedDevices).FindDict(kPhoneGuid));
  ASSERT_TRUE(RunUntilWithMockTime(task_environment_, [&] {
    for (const mojom::DevicePtr& device : phone_->service->GetDevices()) {
      if (device->is_connected) {
        return false;
      }
    }
    return true;
  }));
}

TEST_F(BraveTransferServiceTest, RemoveDevice) {
  ConnectDevices();
  EXPECT_TRUE(mac_->service->RemoveDevice(kPhoneGuid));
  EXPECT_TRUE(mac_->service->GetDevices().empty());
  const base::DictValue* entry =
      mac_->prefs.GetDict(prefs::kTrustedDevices).FindDict(kPhoneGuid);
  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->FindString("key"));
  task_environment_.FastForwardBy(BraveTransferService::kMinRetryDelay * 2);
  EXPECT_FALSE(MacSeesPhone());
}

TEST_F(BraveTransferServiceTest, PageCloseEndsConnectionsAfterGracePeriod) {
  ConnectDevices();
  mac_->service->RemovePage();
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
  EXPECT_TRUE(MacSeesPhone());
  task_environment_.FastForwardBy(BraveTransferService::kIdleGracePeriod);
  ASSERT_TRUE(
      RunUntilWithMockTime(task_environment_, [&] { return !MacSeesPhone(); }));
}

TEST_F(BraveTransferServiceTest, PolicyDisablesService) {
  ConnectDevices();
  mac_->prefs.SetBoolean(prefs::kEnabled, false);
  EXPECT_FALSE(mac_->service->IsEnabled());
  EXPECT_FALSE(mac_->service->IsDiscoveryActive());
  EXPECT_EQ(mac_->service->SendText(kPhoneGuid, "hi"),
            mojom::SendResult::kFailed);
  ASSERT_TRUE(
      RunUntilWithMockTime(task_environment_, [&] { return !MacSeesPhone(); }));
}

TEST_F(BraveTransferServiceTest, UpdateSettingsCannotSetSaveFolder) {
  mojom::SettingsPtr settings = mac_->service->GetSettings();
  settings->auto_accept_trusted = true;
  settings->save_directory = "/etc";
  EXPECT_TRUE(mac_->service->UpdateSettings(std::move(settings)));
  EXPECT_TRUE(mac_->prefs.GetBoolean(prefs::kAutoAcceptTrusted));
  EXPECT_TRUE(mac_->prefs.GetFilePath(prefs::kSaveDirectory).empty());
  EXPECT_GT(mac_->observer.settings_changes, 0);

  mac_->service->SetSaveDirectory(temp_dir_.GetPath());
  EXPECT_EQ(mac_->service->GetSettings()->save_directory,
            temp_dir_.GetPath().AsUTF8Unsafe());
  settings = mac_->service->GetSettings();
  settings->save_directory.clear();
  EXPECT_TRUE(mac_->service->UpdateSettings(std::move(settings)));
  EXPECT_TRUE(mac_->service->GetSettings()->save_directory.empty());
}

TEST_F(BraveTransferServiceTest, ChosenSaveFolderIsUsed) {
  const base::FilePath chosen = temp_dir_.GetPath().AppendASCII("chosen");
  phone_->service->SetSaveDirectory(chosen);
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  std::vector<mojom::FileDataPtr> files;
  files.push_back(mojom::FileData::New("a.pdf", "application/pdf",
                                       std::vector<uint8_t>(10, 1)));
  ASSERT_EQ(mac_->service->SendFiles(kPhoneGuid, std::move(files)),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->Inbox().size() == 1; }));
  EXPECT_TRUE(base::PathExists(chosen.AppendASCII("a.pdf")));
  EXPECT_FALSE(phone_->Inbox()[0]->saved_to_fallback);
}

TEST_F(BraveTransferServiceTest, ClearHistory) {
  phone_->prefs.SetBoolean(prefs::kAutoAcceptTrusted, true);
  ConnectDevices();
  std::vector<mojom::FileDataPtr> files;
  files.push_back(mojom::FileData::New("keep.txt", "text/plain",
                                       std::vector<uint8_t>{'k'}));
  ASSERT_EQ(mac_->service->SendFiles(kPhoneGuid, std::move(files)),
            mojom::SendResult::kSuccess);
  ASSERT_TRUE(RunUntilWithMockTime(
      task_environment_, [&] { return phone_->Inbox().size() == 1; }));

  base::test::TestFuture<bool> cleared;
  phone_->service->ClearHistory(cleared.GetCallback());
  EXPECT_TRUE(cleared.Get());
  EXPECT_TRUE(phone_->Inbox().empty());
  EXPECT_TRUE(base::PathExists(phone_downloads_.AppendASCII("keep.txt")));
}

}  // namespace brave_transfer

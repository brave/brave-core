/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/run_until.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/sync/base/command_line_switches.h"
#include "components/sync/base/data_type.h"
#include "components/sync/protocol/sync_entity.pb.h"
#include "components/sync/service/sync_user_settings.h"
#include "components/sync/test/embedded_fake_server_adapter.h"
#include "components/sync/test/fake_server.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

}  // namespace

// Smoke test for enabling Sync. It runs the real sync engine against an
// in-process `fake_server::FakeServer`, so it needs neither Brave's sync
// server nor an access token.
//
// Upstream code reached from the sync engine start-up frequently CHECKs on
// KeyedServices which Brave disables along with their features. Such a CHECK
// only fires once the engine completes a sync cycle, which is why reaching
// `MergeFullSyncData` matters here and why no unit test covers it.
class BraveSyncSmokeBrowserTest : public PlatformBrowserTest {
 public:
  void SetUp() override {
    // Reserves the port so `SetUpCommandLine()` can point --sync-url at it,
    // without accepting connections yet.
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    PlatformBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        syncer::kSyncServiceURL,
        embedded_test_server()
            ->GetURL(fake_server::EmbeddedFakeServerAdapter::kPath)
            .spec());
    command_line->AppendSwitchASCII(syncer::kSyncDeferredStartupTimeoutSeconds,
                                    "0");
    command_line->AppendSwitch(syncer::kSyncShortInitialRetryOverride);
    command_line->AppendSwitch(syncer::kSyncShortNudgeDelayForTest);
  }

  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    // FakeServer binds to the sequence it is created on, so it cannot be
    // created before the UI thread exists. Nothing contacts the server until
    // the sync code is set below.
    fake_server_ = std::make_unique<fake_server::FakeServer>();
    embedded_fake_server_adapter_ =
        std::make_unique<fake_server::EmbeddedFakeServerAdapter>(
            fake_server_->AsWeakPtr());
    embedded_fake_server_adapter_->RegisterRequestHandler(
        *embedded_test_server());
    embedded_test_server_handle_ =
        embedded_test_server()->StartAcceptingConnectionsAndReturnHandle();
  }

  void TearDownOnMainThread() override {
    embedded_test_server_handle_ = net::test_server::EmbeddedTestServerHandle();
    embedded_fake_server_adapter_.reset();
    fake_server_.reset();
    PlatformBrowserTest::TearDownOnMainThread();
  }

 protected:
  std::unique_ptr<fake_server::FakeServer> fake_server_;

 private:
  std::unique_ptr<fake_server::EmbeddedFakeServerAdapter>
      embedded_fake_server_adapter_;
  net::test_server::EmbeddedTestServerHandle embedded_test_server_handle_;
};

IN_PROC_BROWSER_TEST_F(BraveSyncSmokeBrowserTest, EnableSync) {
  auto* sync_service = static_cast<syncer::BraveSyncServiceImpl*>(
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(
          chrome_test_utils::GetProfile(this)));
  ASSERT_TRUE(sync_service);

  // Mirrors joining a sync chain, see `BraveSyncHandler::HandleSetSyncCode()`
  // and `BraveSyncWorker::SaveCodeWords()`.
  ASSERT_TRUE(sync_service->SetSyncCode(kValidSyncCode));
  sync_service->GetUserSettings()->SetInitialSyncFeatureSetupComplete();

  // A committed DEVICE_INFO entity proves the engine got through the initial
  // GetUpdates for that type, which is where `DeviceInfoSyncBridge::
  // MergeFullSyncData()` collects the local device info.
  EXPECT_TRUE(base::test::RunUntil([&]() {
    return !fake_server_->GetSyncEntitiesByDataType(syncer::DEVICE_INFO)
                .empty();
  }));

  EXPECT_TRUE(sync_service->IsSyncFeatureActive());
}

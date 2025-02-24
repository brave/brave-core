/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kValidSyncCode[] =
    "fringe digital begin feed equal output proof cheap "
    "exotic ill sure question trial squirrel glove celery "
    "awkward push jelly logic broccoli almost grocery drift";

}  // namespace

// This test ensures that the browser doesn't crashes as it is described at
// https://github.com/brave/brave-browser/issues/43727
// The conditions for the crash were:
//    1. Sync chain is set up
//    2. Command line has --sync-deferred-startup-timeout-seconds=0

class BraveSyncNetworkTimeHelperBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(syncer::kSyncDeferredStartupTimeoutSeconds,
                                    "0");
  }

  void SetUpInProcessBrowserTestFixture() override {
    PlatformBrowserTest::SetUpInProcessBrowserTestFixture();
    create_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(
                base::BindRepeating(&BraveSyncNetworkTimeHelperBrowserTest ::
                                        OnWillCreateBrowserContextServices,
                                    base::Unretained(this)));
  }

  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    // At this point profile and preferences are created, but sync service is
    // not yet. Pretend we have a configured sync chain by setting up the sync
    // code. This along with --sync-deferred-startup-timeout-seconds=0 will
    // cause SyncServiceImpl::Initialize() immediately post
    // SyncServiceImpl::TryStartImpl() and this would crash without
    // brave-core/pull/27499

    brave_sync::Prefs brave_sync_refs(
        static_cast<Profile*>(context)->GetPrefs());
    brave_sync_refs.SetSeed(kValidSyncCode);
  }

  base::CallbackListSubscription create_services_subscription_;
};

IN_PROC_BROWSER_TEST_F(BraveSyncNetworkTimeHelperBrowserTest, DidntCrash) {
  // The actual test is the fact we didn't crashed at
  //    brave_sync::NetworkTimeHelper::GetNetworkTime()
  //    syncer::BraveSyncAuthManager::RequestAccessToken()
  //    syncer::SyncAuthManager::ConnectionOpened()
  //    syncer::SyncServiceImpl::TryStartImpl()
  // because NetworkTimeHelper::ui_task_runner_ wasn't set at the time.
  // You can see the test crash failure by reverting commit
  // 92c41053e2da9d5931ed44036f7594b69559fa66

  EXPECT_TRUE(true);
}

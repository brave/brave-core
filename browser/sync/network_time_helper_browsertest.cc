/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/network_time_helper.h"

#include "base/base64.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/prefs/pref_service.h"
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
                base::BindRepeating(&BraveSyncNetworkTimeHelperBrowserTest::
                                        OnWillCreateBrowserContextServices,
                                    base::Unretained(this)));
  }

  void OnWillCreateBrowserContextServices(content::BrowserContext* context) {
    // At this point profile and preferences are created, but sync service is
    // not yet. Pretend we have a configured sync chain by setting the sync
    // seed directly in the pref. This along with
    // --sync-deferred-startup-timeout-seconds=0 will cause
    // SyncServiceImpl::Initialize() immediately post
    // SyncServiceImpl::TryStartImpl() and this would crash without
    // brave-core/pull/27499.
    // OSCryptAsync is already initialized by browser startup before profile
    // creation, so GetInstance fires synchronously here.
    g_browser_process->os_crypt_async()->GetInstance(base::BindLambdaForTesting(
        [context](scoped_refptr<os_crypt_async::Encryptor> e) {
          std::string encrypted_seed;
          if (e->EncryptString(kValidSyncCode, &encrypted_seed)) {
            static_cast<Profile*>(context)->GetPrefs()->SetString(
                brave_sync::Prefs::GetSeedPath(),
                base::Base64Encode(encrypted_seed));
          }
        }));
  }

 private:
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

// Deliberately does NOT derive from BraveSyncNetworkTimeHelperBrowserTest: the
// death test only needs `NetworkTimeHelper`, which is owned by the browser
// process and is initialized independently of sync, in
// BraveBrowserProcessImpl::PreMainMessageLoopRun(). Configuring a sync chain
// here would start the sync engine, whose
// `HttpBridge::MakeSynchronousPost()` blocks one `MayBlock()` ThreadPool
// sequence until a second ThreadPool sequence completes the request. Death
// tests run in the "threadsafe" style, so `EXPECT_DEATH` re-executes the test
// binary and blocks until that second browser process dies; while it starts
// up, the sequence that would complete the request can be starved for tens of
// seconds (~33s in the reported runs). That is long enough for the blocked
// sync task to exceed `TaskEnvironment`'s 30s `action_max_timeout()` per-task
// watchdog, which fails this test with "RunTask took more than 30 seconds.
// Posted from TrySyncCycleJob". The request itself is never slow: browser
// tests already fail external DNS instantly via content::TestHostResolver.
// See https://github.com/brave/brave-browser/issues/50706 and
// https://github.com/brave/brave-browser/issues/57184
using BraveSyncNetworkTimeHelperBrowserDeathTest = PlatformBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveSyncNetworkTimeHelperBrowserDeathTest,
                       CrashNoUiTaskRunner) {
  brave_sync::NetworkTimeHelper::GetInstance()->SetNetworkTimeTracker(
      g_browser_process->network_time_tracker(), nullptr);

  EXPECT_DEATH(brave_sync::NetworkTimeHelper::GetInstance()->GetNetworkTime(
                   base::DoNothing()),
               "");
}

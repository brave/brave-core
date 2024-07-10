/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test.h"

using extensions::ExtensionBrowserTest;

namespace {
constexpr const char kTestComponentId[] = "lcoibaikiallcnnjjjnbofjpfdkddfmp";
constexpr const char kTestComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2ykOuzPbWcPmZSA085mQW3qRpNI+"
    "B58lH9VftHTi1+oXXpbada5UmLI67Pc/"
    "QsbHVCi515Q6EaSOUblBUEOMPbps40YxMjj2W5aWLab/"
    "xwq0yhKFrk4x5f0GukOlFlaZuN0kfqknPnMuLKqzDHkC/"
    "OaednbB4MhQWJ8Rf80OumyQgWeokxzlIeZf/"
    "CVW2CXtzpk6gNYvASJBXG3y34W0tR7HwUX9ghAIgAawWIBRPNMeUauBhZU6/"
    "nh0COMyJy2WDdvW9RPRdAZF3JR1c99kcEluirw3Ah5znnYjyKV21mgeVVMfg6SLMoO6G2Nmqql"
    "NIoM8dx1YGn7IdC6b9LpTcQIDAQAB";
}  // namespace

class LeoLocalModelsUpdaterTest : public ExtensionBrowserTest {
 public:
  LeoLocalModelsUpdaterTest() = default;

  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    test_data_dir_ = base::PathService::CheckedGet(brave::DIR_TEST_DATA);
  }

  bool PathExists(const base::FilePath& file_path) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathExists(file_path);
  }

  void SetComponentIdAndBase64PublicKeyForTest(
      const std::string& component_id,
      const std::string& component_base64_public_key) {
    ai_chat::LeoLocalModelsUpdater::SetComponentIdAndBase64PublicKeyForTest(
        component_id, component_base64_public_key);
  }

  // ExtensionBrowserTest install extensions at user_data_dir/Extensions so we
  // need to set user_data_dir to test Cleanup.
  void SetUserDataDirForTest() {
    ai_chat::LeoLocalModelsUpdater::SetUserDataDirForTest(
        profile()->GetPath().AppendASCII("Extensions"));
  }

  bool InstallUpdater() {
    // base::ScopedAllowBlockingForTesting allow_blocking;
    const extensions::Extension* updater =
        InstallExtension(test_data_dir_.AppendASCII("leo").AppendASCII(
                             "leo-local-models-updater"),
                         1);
    if (!updater) {
      return false;
    }
    // Wait for the updater to be installed.
    WaitForUpdater();
    g_brave_browser_process->leo_local_models_updater()->OnComponentReady(
        updater->id(), updater->path(), "");
    updater_path_ = updater->path();
    return true;
  }

  void WaitForUpdater() {
    scoped_refptr<base::ThreadTestHelper> updater_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->leo_local_models_updater()
                ->GetTaskRunner()));
    ASSERT_TRUE(updater_helper->Run());
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(content::GetIOThreadTaskRunner({}).get()));
    ASSERT_TRUE(io_helper->Run());
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::FilePath test_data_dir_;
  base::FilePath updater_path_;
};

IN_PROC_BROWSER_TEST_F(LeoLocalModelsUpdaterTest, InstallAndCheckPath) {
  SetComponentIdAndBase64PublicKeyForTest(kTestComponentId,
                                          kTestComponentBase64PublicKey);
  ASSERT_TRUE(InstallUpdater());
  EXPECT_TRUE(PathExists(updater_path_));
  EXPECT_EQ(g_brave_browser_process->leo_local_models_updater()
                ->GetUniversalQAModel(),
            updater_path_.AppendASCII(ai_chat::kUniversalQAModelName));
  EXPECT_TRUE(PathExists(g_brave_browser_process->leo_local_models_updater()
                             ->GetUniversalQAModel()));
}

IN_PROC_BROWSER_TEST_F(LeoLocalModelsUpdaterTest, Cleanup) {
  SetUserDataDirForTest();
  SetComponentIdAndBase64PublicKeyForTest(kTestComponentId,
                                          kTestComponentBase64PublicKey);
  ASSERT_TRUE(InstallUpdater());
  ASSERT_TRUE(PathExists(updater_path_));
  ASSERT_TRUE(PathExists(g_brave_browser_process->leo_local_models_updater()
                             ->GetUniversalQAModel()));
  base::RunLoop run_loop;
  g_brave_browser_process->leo_local_models_updater()->Cleanup(
      base::BindLambdaForTesting([&run_loop](bool result) {
        EXPECT_TRUE(result);
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_FALSE(PathExists(g_brave_browser_process->leo_local_models_updater()
                              ->GetUniversalQAModel()));
  EXPECT_FALSE(PathExists(updater_path_));
}

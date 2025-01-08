/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/custom_background_file_manager.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/ntp_background/constants.h"
#include "brave/components/constants/brave_paths.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"

// CustomBackgroundFileManager requires data decoder which can't be initialized
// in unit tests.
class CustomBackgroundFileManagerBrowserTest : public InProcessBrowserTest {
 public:
  static constexpr char kTestImageName[] = "background.jpg";

  CustomBackgroundFileManagerBrowserTest() = default;
  ~CustomBackgroundFileManagerBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    file_manager_ = std::make_unique<CustomBackgroundFileManager>(profile());

    base::ScopedAllowBlockingForTesting allow_blocking_call;
    base::FilePath test_data_dir;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir));

    ASSERT_TRUE(
        base::CopyFile(test_data_dir.Append(
                           FILE_PATH_LITERAL("ntp_background/background.jpg")),
                       profile()->GetPath().AppendASCII(kTestImageName)));

    run_loop_ = std::make_unique<base::RunLoop>();
  }

  void TearDownOnMainThread() override {
    run_loop_.reset();
    file_manager_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 protected:
  base::FilePath test_file() const {
    return profile()->GetPath().AppendASCII(kTestImageName);
  }

  Profile* profile() { return browser()->profile(); }
  const Profile* profile() const { return browser()->profile(); }

  CustomBackgroundFileManager& custom_file_manager() { return *file_manager_; }

  void Wait() { run_loop_->Run(); }
  base::OnceClosure run_loop_quit_closure() { return run_loop_->QuitClosure(); }

 private:
  std::unique_ptr<CustomBackgroundFileManager> file_manager_;

  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(CustomBackgroundFileManagerBrowserTest,
                       CustomBackgroundDirectory) {
  EXPECT_EQ(profile()->GetPath().AppendASCII(
                ntp_background_images::kSanitizedImageDirName),
            custom_file_manager().GetCustomBackgroundDirectory());
}

IN_PROC_BROWSER_TEST_F(CustomBackgroundFileManagerBrowserTest,
                       MoveImageToCustomBackgroundDir) {
  auto check_result = base::BindOnce([](bool result) {
                        EXPECT_TRUE(result);
                      }).Then(run_loop_quit_closure());

  custom_file_manager().MoveImage(test_file(), std::move(check_result));
  Wait();

  base::ScopedAllowBlockingForTesting allow_blocking_call;
  EXPECT_TRUE(base::PathExists(
      custom_file_manager().GetCustomBackgroundDirectory().AppendASCII(
          kTestImageName)));
  EXPECT_FALSE(base::PathExists(test_file()));
}

IN_PROC_BROWSER_TEST_F(CustomBackgroundFileManagerBrowserTest,
                       SaveImageToCustomBackgroundDir) {
  auto check_result = base::BindOnce([](const base::FilePath& path) {
                        EXPECT_FALSE(path.empty());
                      }).Then(run_loop_quit_closure());

  custom_file_manager().SaveImage(test_file(), std::move(check_result));
  Wait();

  base::ScopedAllowBlockingForTesting allow_blocking_call;
  EXPECT_TRUE(base::PathExists(
      custom_file_manager().GetCustomBackgroundDirectory().AppendASCII(
          kTestImageName)));
  EXPECT_TRUE(base::PathExists(test_file()));
}

// On Mac x64 CI node we presume this test crashes the whole suite.
// https://github.com/brave/brave-browser/issues/38988
#if BUILDFLAG(IS_MAC) && defined(ARCH_CPU_ARM_FAMILY)
#define MAYBE_SaveImageMultipleTimes SaveImageMultipleTimes
#else
#define MAYBE_SaveImageMultipleTimes DISABLED_SaveImageMultipleTimes
#endif

IN_PROC_BROWSER_TEST_F(CustomBackgroundFileManagerBrowserTest,
                       MAYBE_SaveImageMultipleTimes) {
  for (int i = 0; i < 3; i++) {
    base::RunLoop run_loop;
    base::FilePath expected_path =
        custom_file_manager().GetCustomBackgroundDirectory().AppendASCII(
            kTestImageName);
    if (i > 0) {
      expected_path = expected_path.InsertBeforeExtensionASCII(
          base::StringPrintf("-%d", i));
    }

    auto check_res =
        base::BindOnce(
            [](base::FilePath expected_path, const base::FilePath& path) {
              EXPECT_EQ(expected_path, path);
            },
            expected_path)
            .Then(run_loop.QuitClosure());

    custom_file_manager().SaveImage(test_file(), std::move(check_res));
    run_loop.Run();

    base::ScopedAllowBlockingForTesting allow_blocking_call;
    EXPECT_TRUE(base::PathExists(expected_path));
    EXPECT_TRUE(base::PathExists(test_file()));
  }
}

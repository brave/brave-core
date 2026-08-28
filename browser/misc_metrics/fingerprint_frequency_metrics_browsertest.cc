/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/fingerprint_frequency_metrics.h"

#include <memory>

#include "base/test/scoped_run_loop_timeout.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_test.h"

namespace misc_metrics {

namespace {

constexpr char kLanguagesKey[] = "navigator_languages";
constexpr char kScreenSizeKey[] = "screenSize";

#if BUILDFLAG(IS_ANDROID)
// Renderer execution can exceed the default timeout on Android. The execution
// timeout and the RunLoop timeout must be raised together.
constexpr base::TimeDelta kAndroidExecutionTimeout = base::Seconds(60);
#endif

}  // namespace

class FingerprintFrequencyMetricsBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    FingerprintFrequencyMetrics::RegisterPrefs(local_state_.registry());
    metrics_ = std::make_unique<FingerprintFrequencyMetrics>(
        &local_state_, chrome_test_utils::GetProfile(this));
#if BUILDFLAG(IS_ANDROID)
    metrics_->SetExecutionTimeoutForTesting(kAndroidExecutionTimeout);
#endif
  }

  void TearDownOnMainThread() override {
    metrics_.reset();
    PlatformBrowserTest::TearDownOnMainThread();
  }

 protected:
  base::DictValue ExecuteRenderer() {
#if BUILDFLAG(IS_ANDROID)
    base::test::ScopedRunLoopTimeout run_timeout(FROM_HERE,
                                                 kAndroidExecutionTimeout);
#endif
    base::test::TestFuture<base::DictValue> future;
    metrics_->ExecuteRendererForTesting(future.GetCallback());
    return future.Take();
  }

  TestingPrefServiceSimple local_state_;
  std::unique_ptr<FingerprintFrequencyMetrics> metrics_;
};

// TODO(https://github.com/brave/brave-browser/issues/58513): Enable this test
// once fixed.
#if BUILDFLAG(IS_ANDROID)
#define NON_ANDROID_TEST(test) DISABLED_##test
#else
#define NON_ANDROID_TEST(test) test
#endif

IN_PROC_BROWSER_TEST_F(FingerprintFrequencyMetricsBrowserTest,
                       NON_ANDROID_TEST(AcceptLanguagesChangeAltersHash)) {
  base::DictValue first = ExecuteRenderer();
  ASSERT_TRUE(first.FindInt(kLanguagesKey));
  ASSERT_TRUE(first.FindInt(kScreenSizeKey));

  // The hashes are stable while nothing changes.
  base::DictValue second = ExecuteRenderer();
  EXPECT_EQ(first.FindInt(kLanguagesKey), second.FindInt(kLanguagesKey));
  EXPECT_EQ(first.FindInt(kScreenSizeKey), second.FindInt(kScreenSizeKey));

  chrome_test_utils::GetProfile(this)->GetPrefs()->SetString(
      language::prefs::kAcceptLanguages, "fr-FR,fr");

  base::DictValue third = ExecuteRenderer();
  EXPECT_NE(second.FindInt(kLanguagesKey), third.FindInt(kLanguagesKey));
  EXPECT_EQ(second.FindInt(kScreenSizeKey), third.FindInt(kScreenSizeKey));
}

}  // namespace misc_metrics

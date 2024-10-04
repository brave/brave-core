/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "sandbox/policy/features.h"

// This header is private to //content/browser and can be used only from
// content_browser_tests, but we don't have such target, so we workaround it
// with `nogncheck`.
#include "content/browser/gpu/gpu_process_host.h"  // nogncheck

using sandbox::policy::features::kModuleFileNamePatch;

namespace {

void NonBlockingDelay(base::TimeDelta delay) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitWhenIdleClosure(), delay);
  run_loop.Run();
}

size_t GetSubStringsCount(const std::string& s, const std::string& ss) {
  size_t result = 0;

  for (size_t pos = 0; pos != std::string::npos; pos = s.find(ss, pos + 1)) {
    ++result;
  }
  return result - 1;
}

}  // namespace

class ModuleFileNameBrowserTest : public InProcessBrowserTest,
                                  public ::testing::WithParamInterface<bool> {
 public:
  ModuleFileNameBrowserTest() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeature(kModuleFileNamePatch);
    } else {
      feature_list_.InitAndDisableFeature(kModuleFileNamePatch);
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

INSTANTIATE_TEST_SUITE_P(, ModuleFileNameBrowserTest, ::testing::Bool());

IN_PROC_BROWSER_TEST_P(ModuleFileNameBrowserTest, CheckPath) {
  std::string path;
  while (path.empty()) {
    NonBlockingDelay(base::Milliseconds(10));
    ASSERT_NE(nullptr, content::GpuProcessHost::Get());
    path = content::GpuProcessHost::Get()->executable_path();
  }

  SCOPED_TRACE(path);

  WCHAR main_path[MAX_PATH] = {0};
  GetModuleFileNameW(nullptr, main_path, MAX_PATH);
  EXPECT_TRUE(base::EndsWith(main_path, L"brave_browser_tests.exe"))
      << main_path;

  constexpr const size_t kInterceptedFunctions = 4u;
#if !defined(ADDRESS_SANITIZER)
  constexpr const size_t kExpectedReplacements = 4u;
#else
  constexpr const size_t kExpectedReplacements = 3u;
#endif

  if (GetParam()) {
    EXPECT_EQ(kInterceptedFunctions - kExpectedReplacements,
              GetSubStringsCount(path, "brave_browser_tests.exe"));
    EXPECT_EQ(kExpectedReplacements,
              GetSubStringsCount(path, "chrome_browser_tests.exe"));
  } else {
    EXPECT_EQ(kInterceptedFunctions,
              GetSubStringsCount(path, "brave_browser_tests.exe"));
    EXPECT_EQ(0u, GetSubStringsCount(path, "chrome_browser_tests.exe"));
  }
}

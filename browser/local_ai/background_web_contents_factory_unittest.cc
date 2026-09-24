// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/background_web_contents_factory.h"

#include <memory>
#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/test/test_future.h"
#include "brave/components/local_ai/core/background_web_contents.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace local_ai {

namespace {

// Minimal delegate whose weak pointer we can drop on demand to exercise the
// "caller destroyed mid-async" guard.
class StubDelegate : public BackgroundWebContents::Delegate {
 public:
  StubDelegate() = default;
  ~StubDelegate() override = default;

  // BackgroundWebContents::Delegate:
  void OnBackgroundContentsDestroyed(
      BackgroundWebContents::DestroyReason reason) override {}

  base::WeakPtr<BackgroundWebContents::Delegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  void InvalidateWeakPtrs() { weak_factory_.InvalidateWeakPtrs(); }

 private:
  base::WeakPtrFactory<BackgroundWebContents::Delegate> weak_factory_{this};
};

}  // namespace

class BackgroundWebContentsFactoryTest : public testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_enabler_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
};

TEST_F(BackgroundWebContentsFactoryTest, CreatesContentsInGuestOtrProfile) {
  StubDelegate delegate;
  base::test::TestFuture<std::unique_ptr<BackgroundWebContents>, Profile*>
      future;
  CreateBackgroundWebContents(GURL("chrome-untrusted://test/"),
                              IDS_LOCAL_AI_TASK_MANAGER_TITLE,
                              /*sandbox_flags=*/std::nullopt,
                              delegate.GetWeakPtr(), future.GetCallback());

  auto [contents, otr_profile] = future.Take();
  EXPECT_TRUE(contents);
  ASSERT_TRUE(otr_profile);
  EXPECT_TRUE(otr_profile->IsOffTheRecord());
  EXPECT_EQ(otr_profile->GetOriginalProfile()->GetPath(),
            ProfileManager::GetGuestProfilePath());

  // Destroy the worker before the profile manager tears down, so its
  // WebContents does not outlive its BrowserContext.
  contents.reset();
}

TEST_F(BackgroundWebContentsFactoryTest, DelegateDestroyedYieldsNull) {
  StubDelegate delegate;
  base::test::TestFuture<std::unique_ptr<BackgroundWebContents>, Profile*>
      future;
  CreateBackgroundWebContents(GURL("chrome-untrusted://test/"),
                              IDS_LOCAL_AI_TASK_MANAGER_TITLE,
                              /*sandbox_flags=*/std::nullopt,
                              delegate.GetWeakPtr(), future.GetCallback());

  // Simulate the caller being destroyed while the guest profile is still
  // being created asynchronously. The reply must yield (nullptr, nullptr)
  // instead of dereferencing the dead delegate.
  delegate.InvalidateWeakPtrs();

  auto [contents, otr_profile] = future.Take();
  EXPECT_FALSE(contents);
  EXPECT_FALSE(otr_profile);
}

}  // namespace local_ai

/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
constexpr char kTestProfileName[] = "TestProfile";
}  // namespace

namespace ipfs {
class IpfsBlobContextGetterFactoryUnitTest : public testing::Test {
 public:
  IpfsBlobContextGetterFactoryUnitTest() = default;
  IpfsBlobContextGetterFactoryUnitTest(
      const IpfsBlobContextGetterFactoryUnitTest&) = delete;
  IpfsBlobContextGetterFactoryUnitTest& operator=(
      const IpfsBlobContextGetterFactoryUnitTest&) = delete;
  ~IpfsBlobContextGetterFactoryUnitTest() override = default;

  void SetUp() override {
    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_.reset(new TestingProfileManager(browser_process));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);
  }
  void TearDown() override {
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  content::BrowserContext* browser_context() { return profile_; }

  void RetrieveStorageIO(BlobContextGetterFactory* getter_factory,
                         base::OnceClosure closure) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    ASSERT_TRUE(getter_factory);
    auto storage_context = getter_factory->RetrieveStorageContext();
    ASSERT_TRUE(storage_context);
    std::move(closure).Run();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  Profile* profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

TEST_F(IpfsBlobContextGetterFactoryUnitTest, GetStorageContext) {
  IpfsBlobContextGetterFactory factory(browser_context());
  std::unique_ptr<base::RunLoop> run_loop(new base::RunLoop());
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&IpfsBlobContextGetterFactoryUnitTest::RetrieveStorageIO,
                     base::Unretained(this), &factory,
                     run_loop->QuitClosure()));
  run_loop->Run();
  run_loop.reset(new base::RunLoop());
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&IpfsBlobContextGetterFactoryUnitTest::RetrieveStorageIO,
                     base::Unretained(this), &factory,
                     run_loop->QuitClosure()));
  run_loop->Run();
}

}  // namespace ipfs

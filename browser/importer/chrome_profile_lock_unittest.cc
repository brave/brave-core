/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/importer/chrome_profile_lock.h"

#if defined(OS_POSIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
const char kLockFile[] = "lockfile";
#endif

class ChromeProfileLockTest : public testing::Test {
 public:
   ChromeProfileLockTest ()
     : test_browser_thread_bundle_(
         content::TestBrowserThreadBundle::REAL_IO_THREAD) {}
 protected:
  void SetUp() override {
    testing::Test::SetUp();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    user_data_path_ = temp_dir_.GetPath();
#if defined(OS_POSIX)
    lock_file_path_ =
      user_data_path_.Append(chrome::kSingletonLockFilename);
#elif defined(OS_WIN)
    lock_file_path_ = user_data_path_.AppendASCII(kLockFile);
#else
#error unsupport platforms
#endif
  }

  void LockFileExists(bool expect);

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  base::ScopedTempDir temp_dir_;
  base::FilePath user_data_path_;
  base::FilePath lock_file_path_;
};

void ChromeProfileLockTest::LockFileExists(bool expect) {
#if defined(OS_POSIX)
  struct stat statbuf;
  if (expect) {
    ASSERT_EQ(0, lstat(lock_file_path_.value().c_str(), &statbuf));
    ASSERT_TRUE(S_ISLNK(statbuf.st_mode));
    char buf[PATH_MAX];
    ssize_t len = readlink(lock_file_path_.value().c_str(), buf, PATH_MAX);
    ASSERT_GT(len, 0);
  } else {
    ASSERT_GT(0, lstat(lock_file_path_.value().c_str(), &statbuf));
  }
#elif defined(OS_WIN)
  if (expect)
    EXPECT_TRUE(base::PathExists(lock_file_path_));
  else
    EXPECT_FALSE(base::PathExists(lock_file_path_));
#endif
}

TEST_F(ChromeProfileLockTest, LockTest) {
  ChromeProfileLock lock(user_data_path_);
  lock.Lock();
  ASSERT_TRUE(lock.HasAcquired());
  lock.Unlock();
  ASSERT_FALSE(lock.HasAcquired());
  lock.Lock();
  ASSERT_TRUE(lock.HasAcquired());
}

// Tests basic functionality and verifies that the lock file is deleted after
// use.
TEST_F(ChromeProfileLockTest, ProfileLock) {
  std::unique_ptr<ChromeProfileLock> lock;
  EXPECT_EQ(static_cast<ChromeProfileLock*>(NULL), lock.get());
  LockFileExists(false);
  lock.reset(new ChromeProfileLock(user_data_path_));
  lock->Lock();
  EXPECT_TRUE(lock->HasAcquired());
  LockFileExists(true);
  lock->Unlock();
  EXPECT_FALSE(lock->HasAcquired());

  lock->Lock();
  EXPECT_TRUE(lock->HasAcquired());
  LockFileExists(true);
  lock->Lock();
  EXPECT_TRUE(lock->HasAcquired());
  lock->Unlock();
  EXPECT_FALSE(lock->HasAcquired());
}

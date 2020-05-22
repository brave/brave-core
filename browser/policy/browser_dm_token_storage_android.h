// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_ANDROID_H_
#define CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_ANDROID_H_

#include "chrome/browser/policy/browser_dm_token_storage.h"

#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"

namespace policy {

// Implementation of BrowserDMTokenStorage for Android. The global singleton
// instance can be retrieved by calling BrowserDMTokenStorage::Get().
class BrowserDMTokenStorageAndroid : public BrowserDMTokenStorage {
 public:
  // Get the global singleton instance by calling BrowserDMTokenStorage::Get().
  BrowserDMTokenStorageAndroid();
  ~BrowserDMTokenStorageAndroid() override;

 private:
  // override BrowserDMTokenStorage
  std::string InitClientId() override;
  std::string InitEnrollmentToken() override;
  std::string InitDMToken() override;
  bool InitEnrollmentErrorOption() override;
  StoreTask SaveDMTokenTask(const std::string& token,
                            const std::string& client_id) override;
  scoped_refptr<base::TaskRunner> SaveDMTokenTaskRunner() override;

  // Returns the content of "/etc/machine-id". Virtual for tests.
  virtual std::string ReadMachineIdFile();

  scoped_refptr<base::TaskRunner> task_runner_;

  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest, InitClientId);
  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest, InitEnrollmentToken);
  // TODO(crbug.com/907589): Remove once no longer in use.
  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest,
                           InitOldEnrollmentToken);
  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest, InitDMToken);
  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest,
                           InitDMTokenWithoutDirectory);
  FRIEND_TEST_ALL_PREFIXES(BrowserDMTokenStorageAndroidTest, SaveDMToken);

  DISALLOW_COPY_AND_ASSIGN(BrowserDMTokenStorageAndroid);
};

}  // namespace policy
#endif  // CHROME_BROWSER_POLICY_BROWSER_DM_TOKEN_STORAGE_ANDROID_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_MANAGER_H_
#define BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/strings/string_piece_forward.h"
#include "services/network/public/cpp/simple_url_loader.h"

class GURL;

namespace base {
class FilePath;
class SequencedTaskRunner;
}

class BraveWidevineBundleManager {
 public:
  static char kWidevineInvalidVersion[];

  // Called when install process is finished.
  using DoneCallback = base::OnceCallback<void(const std::string& error)>;

  BraveWidevineBundleManager();
  ~BraveWidevineBundleManager();

  void InstallWidevineBundle(DoneCallback done_callback, bool user_gesture);

  // Check consistency of library file, installed version and latest version
  // and fix mismatches if needed. Also backgroud update is triggered if
  // installed version is outdated.
  void StartupCheck();

  // Returns true when install/update is in progress.
  // True doesn't mean install/update success. It means just finishing of
  // install/update request.
  bool in_progress() const;

  // Returns true when newly installed/updated version is ready.
  bool needs_restart() const;

  int GetWidevinePermissionRequestTextFragment() const;

  void WillRestart() const;

  bool is_test() const { return is_test_; }

 private:
  friend class BraveWidevineBundleManagerTest;

  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, InProgressTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, UpdateTriggerTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, UpdateFailTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest,
                           UpdateRetryAndFinallyFailedTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest,
                           UpdateRetryAndFinallySuccessTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, InstallSuccessTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, DownloadFailTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, UnzipFailTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest, MessageStringTest);
  FRIEND_TEST_ALL_PREFIXES(BraveWidevineBundleManagerTest,
                           RetryInstallAfterFail);
  FRIEND_TEST_ALL_PREFIXES(WidevinePermissionRequestBrowserTest,
                           TriggerTwoPermissionTest);

  void DownloadWidevineBundle(
      const GURL& bundle_zipfile_url,
      network::SimpleURLLoader::DownloadToFileCompleteCallback callback);
  void OnBundleDownloaded(base::FilePath tmp_bundle_zip_file_path);
  void OnGetTargetWidevineBundleDir(
      const base::FilePath& tmp_bundle_zip_file_path,
      base::Optional<base::FilePath> target_bundle_dir);
  void set_in_progress(bool in_progress);
  void set_needs_restart(bool needs_restart);
  void Unzip(const base::FilePath& tmp_bundle_zip_file_path,
             const base::FilePath& target_bundle_dir);
  void OnBundleUnzipped(const std::string& error);

  void InstallDone(const std::string& error);
  void DoDelayedBackgroundUpdate();
  void ScheduleBackgroundUpdate();
  void OnBackgroundUpdateFinished(const std::string& error);

  scoped_refptr<base::SequencedTaskRunner> file_task_runner();

  bool is_test_ = false;
  bool update_requested_ = false;
  bool startup_checked_ = false;
  DoneCallback done_callback_;
  bool in_progress_ = false;
  bool needs_restart_ = false;
  int background_update_retry_ = 0;
  std::unique_ptr<network::SimpleURLLoader> bundle_loader_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  base::WeakPtrFactory<BraveWidevineBundleManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveWidevineBundleManager);
};

#endif  // BRAVE_BROWSER_WIDEVINE_BRAVE_WIDEVINE_BUNDLE_MANAGER_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"

#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"

#include "content/public/browser/browser_thread.h"

#include "third_party/leveldatabase/src/include/leveldb/db.h"

namespace chrome {
namespace android {

#define DB_FILE_NAME      "brave_sync_db"

BraveSyncWorker::BraveSyncWorker(JNIEnv* env, jobject obj):
  weak_java_shields_config_(env, obj) {
}

BraveSyncWorker::~BraveSyncWorker() {
}

static void JNI_BraveSyncWorker_DestroyV1LevelDb(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  base::FilePath app_data_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
  base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);

  leveldb::Status status =
      leveldb::DestroyDB(dbFilePath.value().c_str(), leveldb::Options());
  VLOG(3) << "[BraveSync] " << __func__ << " destroy DB status is "
          << status.ToString();
}

static void JNI_BraveSyncWorker_MarkSyncV1WasEnabledAndMigrated(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  Profile* profile =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  brave_sync_prefs.SetSyncV1WasEnabled();
  brave_sync_prefs.SetSyncV1Migrated(true);
  VLOG(3) << "[BraveSync] " << __func__ << " done";
}

void BraveSyncWorker::ResetSync(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
}

}  // namespace android
}  // namespace chrome

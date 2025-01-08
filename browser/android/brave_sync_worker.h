/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_

#include <jni.h>

#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_service_observer.h"

class Profile;

namespace syncer {
class BraveSyncServiceImpl;
}  // namespace syncer

namespace chrome {
namespace android {

class BraveSyncWorker : public syncer::SyncServiceObserver {
 public:
  BraveSyncWorker(JNIEnv* env, const base::android::JavaRef<jobject>& obj);
  BraveSyncWorker(const BraveSyncWorker&) = delete;
  BraveSyncWorker& operator=(const BraveSyncWorker&) = delete;
  ~BraveSyncWorker() override;

  void Destroy(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetSyncCodeWords(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetSyncServiceURL(JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetDefaultSyncServiceURL(
      JNIEnv* env);

  base::android::ScopedJavaLocalRef<jstring> GetCustomSyncServiceURL(
      JNIEnv* env);

  bool SetCustomSyncServiceURL(
      JNIEnv* env,
      const base::android::JavaParamRef<jstring>& custom_sync_url);

  void SaveCodeWords(JNIEnv* env,
                     const base::android::JavaParamRef<jstring>& passphrase);

  void RequestSync(JNIEnv* env);

  void FinalizeSyncSetup(JNIEnv* env);

  bool IsInitialSyncFeatureSetupComplete(JNIEnv* env);

  void ResetSync(JNIEnv* env);

  void PermanentlyDeleteAccount(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& callback);

  void SetJoinSyncChainCallback(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& callback);

  void ClearAccountDeletedNoticePending(JNIEnv* env);
  bool IsAccountDeletedNoticePending(JNIEnv* env);

 private:
  syncer::BraveSyncServiceImpl* GetSyncService() const;
  void MarkFirstSetupComplete();

  // syncer::SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* service) override;

  void OnResetDone();

  void SetEncryptionPassphrase(syncer::SyncService* service);
  void SetDecryptionPassphrase(syncer::SyncService* service);

  JavaObjectWeakGlobalRef weak_java_brave_sync_worker_;
  raw_ptr<Profile> profile_ = nullptr;

  std::string passphrase_;

  base::ScopedMultiSourceObservation<syncer::SyncService,
                                     syncer::SyncServiceObserver>
      sync_service_observer_{this};
  base::WeakPtrFactory<BraveSyncWorker> weak_ptr_factory_{this};
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_SYNC_WORKER_H_

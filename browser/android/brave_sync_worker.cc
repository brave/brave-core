/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_sync_worker.h"

#include <string>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"

#include "brave/build/android/jni_headers/BraveSyncWorker_jni.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/profile_sync_service_helper.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"

#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/unified_consent/unified_consent_metrics.h"

#include "content/public/browser/browser_thread.h"

#include "third_party/leveldatabase/src/include/leveldb/db.h"

// TODO(alexeybarabash): consider use of java ProfileSyncService methods:
//    addSyncStateChangedListener
//    removeSyncStateChangedListener
//    requestStart
//    requestStop
//    setFirstSetupComplete
//    isFirstSetupComplete

namespace {
static const size_t SEED_BYTES_COUNT = 32u;
}  // namespace

namespace chrome {
namespace android {

// Keep this to clear V1 stuff on migrating
#define DB_FILE_NAME      "brave_sync_db"

BraveSyncWorker::BraveSyncWorker(JNIEnv* env,
                                 const base::android::JavaRef<jobject>& obj)
    : weak_java_brave_sync_worker_(env, obj) {
  Java_BraveSyncWorker_setNativePtr(env, obj, reinterpret_cast<intptr_t>(this));

  profile_ = ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  DCHECK_NE(profile_, nullptr);
}

BraveSyncWorker::~BraveSyncWorker() {}

void BraveSyncWorker::Destroy(JNIEnv* env) {
  delete this;
}

static void JNI_BraveSyncWorker_DestroyV1LevelDb(JNIEnv* env) {
  base::FilePath app_data_path;
  base::PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_path);
  base::FilePath dbFilePath = app_data_path.Append(DB_FILE_NAME);

  leveldb::Status status =
      leveldb::DestroyDB(dbFilePath.value().c_str(), leveldb::Options());
  VLOG(3) << "[BraveSync] " << __func__ << " destroy DB status is "
          << status.ToString();
}

static void JNI_BraveSyncWorker_MarkSyncV1WasEnabledAndMigrated(JNIEnv* env) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  Profile* profile =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  brave_sync_prefs.SetSyncV1WasEnabled();
  brave_sync_prefs.SetSyncV1Migrated(true);
  VLOG(3) << "[BraveSync] " << __func__ << " done";
}

base::android::ScopedJavaLocalRef<jstring> BraveSyncWorker::GetSyncCodeWords(
    JNIEnv* env) {
  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service)
    sync_code = sync_service->GetOrCreateSyncCode();

  return base::android::ConvertUTF8ToJavaString(env, sync_code);
}

void BraveSyncWorker::SaveCodeWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& passphrase) {
  std::string str_passphrase =
      base::android::ConvertJavaStringToUTF8(passphrase);

  auto* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(str_passphrase)) {
    const std::string error_msg =
      sync_service
      ? "invalid sync code:" + str_passphrase
      : "sync service is not available";
    LOG(ERROR) << error_msg;
    return;
  }

  passphrase_ = str_passphrase;
}

syncer::BraveProfileSyncService* BraveSyncWorker::GetSyncService() const {
  return SyncServiceFactory::IsSyncAllowed(profile_)
             ? static_cast<syncer::BraveProfileSyncService*>(
                   SyncServiceFactory::GetForProfile(profile_))
             : nullptr;
}

// Most of methods below were taken from by PeopleHandler class to
// bring the logic of enabling / disabling sync from deskop to Android

void BraveSyncWorker::RequestSync(JNIEnv* env) {
  syncer::SyncService* service = SyncServiceFactory::GetForProfile(profile_);

  if (service && !sync_service_observer_.IsObservingSource(service)) {
    sync_service_observer_.AddObservation(service);
  }

  // Mark Sync as requested by the user. It might already be requested, but
  // it's not if this is either the first time the user is setting up Sync, or
  // Sync was set up but then was reset via the dashboard. This also pokes the
  // SyncService to start up immediately, i.e. bypass deferred startup.
  if (service) {
    service->GetUserSettings()->SetSyncRequested(true);
  }
}

void BraveSyncWorker::MarkFirstSetupComplete() {
  syncer::SyncService* service = GetSyncService();

  // The sync service may be nullptr if it has been just disabled by policy.
  if (!service)
    return;

  service->GetUserSettings()->SetSyncRequested(true);

  // If the first-time setup is already complete, there's nothing else to do.
  if (service->GetUserSettings()->IsFirstSetupComplete())
    return;

  unified_consent::metrics::RecordSyncSetupDataTypesHistrogam(
      service->GetUserSettings(), profile_->GetPrefs());

  // We're done configuring, so notify SyncService that it is OK to start
  // syncing.
  service->GetUserSettings()->SetFirstSetupComplete(
      syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
}

void BraveSyncWorker::FinalizeSyncSetup(JNIEnv* env) {
  MarkFirstSetupComplete();
}

bool BraveSyncWorker::IsFirstSetupComplete(JNIEnv* env) {
  syncer::SyncService* sync_service = GetSyncService();
  return sync_service &&
         sync_service->GetUserSettings()->IsFirstSetupComplete();
}

void BraveSyncWorker::ResetSync(JNIEnv* env) {
  auto* sync_service = GetSyncService();

  if (!sync_service)
    return;

  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::ResetSync(sync_service, device_info_sync_service,
                        base::BindOnce(&BraveSyncWorker::OnResetDone,
                                       weak_ptr_factory_.GetWeakPtr()));
}

bool BraveSyncWorker::GetSyncV1WasEnabled(JNIEnv* env) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  bool sync_v1_was_enabled = brave_sync_prefs.IsSyncV1Enabled();
  return sync_v1_was_enabled;
}

bool BraveSyncWorker::GetSyncV2MigrateNoticeDismissed(JNIEnv* env) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  bool sync_v2_migration_notice_dismissed =
      brave_sync_prefs.IsSyncMigrateNoticeDismissed();
  return sync_v2_migration_notice_dismissed;
}

void BraveSyncWorker::SetSyncV2MigrateNoticeDismissed(
    JNIEnv* env,
    bool sync_v2_migration_notice_dismissed) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetDismissSyncMigrateNotice(
      sync_v2_migration_notice_dismissed);
}

void BraveSyncWorker::OnResetDone() {
  syncer::SyncService* sync_service = GetSyncService();
  if (sync_service) {
    if (sync_service_observer_.IsObservingSource(sync_service)) {
      sync_service_observer_.RemoveObservation(sync_service);
    }
  }
}

void BraveSyncWorker::SetEncryptionPassphrase(syncer::SyncService* service) {
  DCHECK(service);
  DCHECK(service->IsEngineInitialized());
  DCHECK(!this->passphrase_.empty());

  syncer::SyncUserSettings* sync_user_settings = service->GetUserSettings();
  DCHECK(!sync_user_settings->IsPassphraseRequired());

  if (sync_user_settings->IsCustomPassphraseAllowed() &&
      !sync_user_settings->IsUsingExplicitPassphrase() &&
      !sync_user_settings->IsTrustedVaultKeyRequired()) {
    sync_user_settings->SetEncryptionPassphrase(this->passphrase_);
    ProfileMetrics::LogProfileSyncInfo(
        ProfileMetrics::SYNC_CREATED_NEW_PASSPHRASE);
  }
}

void BraveSyncWorker::SetDecryptionPassphrase(syncer::SyncService* service) {
  DCHECK(service);
  DCHECK(service->IsEngineInitialized());
  DCHECK(!this->passphrase_.empty());
  syncer::SyncUserSettings* sync_user_settings = service->GetUserSettings();
  DCHECK(sync_user_settings->IsPassphraseRequired());

  if (sync_user_settings->SetDecryptionPassphrase(this->passphrase_)) {
    ProfileMetrics::LogProfileSyncInfo(
        ProfileMetrics::SYNC_ENTERED_EXISTING_PASSPHRASE);
  }
}

void BraveSyncWorker::OnStateChanged(syncer::SyncService* service) {
  // If the sync engine has shutdown for some reason, just give up
  if (!service || !service->IsEngineInitialized()) {
    VLOG(3) << "[BraveSync] " << __func__ << " sync engine is not initialized";
    return;
  }

  if (this->passphrase_.empty()) {
    VLOG(3) << "[BraveSync] " << __func__ << " empty passphrase";
    return;
  }

  if (service->GetUserSettings()->IsPassphraseRequired()) {
    SetDecryptionPassphrase(service);
  } else {
    SetEncryptionPassphrase(service);
  }
}

static void JNI_BraveSyncWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveSyncWorker(env, jcaller);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetSeedHexFromWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& seed_words) {
  std::string str_seed_words =
      base::android::ConvertJavaStringToUTF8(seed_words);
  DCHECK(!str_seed_words.empty());

  std::string sync_code_hex;
  std::vector<uint8_t> bytes;
  if (brave_sync::crypto::PassphraseToBytes32(str_seed_words, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    sync_code_hex = base::HexEncode(&bytes.at(0), bytes.size());
  } else {
    VLOG(1) << __func__ << " PassphraseToBytes32 failed for " << str_seed_words;
  }

  return base::android::ConvertUTF8ToJavaString(env, sync_code_hex);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetWordsFromSeedHex(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& seed_hex) {
  std::string str_seed_hex = base::android::ConvertJavaStringToUTF8(seed_hex);
  DCHECK(!str_seed_hex.empty());

  std::vector<uint8_t> bytes;
  std::string sync_code_words;
  if (base::HexStringToBytes(str_seed_hex, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    if (bytes.size(), SEED_BYTES_COUNT) {
      sync_code_words = brave_sync::crypto::PassphraseFromBytes32(bytes);
      if (sync_code_words.empty()) {
        VLOG(1) << __func__ << " PassphraseFromBytes32 failed for " << seed_hex;
      }
    } else {
      LOG(ERROR) << "wrong seed bytes " << bytes.size();
    }
    DCHECK_NE(sync_code_words, "");
  } else {
    VLOG(1) << __func__ << " HexStringToBytes failed for " << str_seed_hex;
  }

  return base::android::ConvertUTF8ToJavaString(env, sync_code_words);
}

}  // namespace android
}  // namespace chrome

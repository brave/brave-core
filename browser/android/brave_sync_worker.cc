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
#include "chrome/browser/sync/profile_sync_service_factory.h"

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

void BraveSyncWorker::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
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

base::android::ScopedJavaLocalRef<jstring> BraveSyncWorker::GetSyncCodeWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service)
    sync_code = sync_service->GetOrCreateSyncCode();

  return base::android::ConvertUTF8ToJavaString(env, sync_code);
}

void BraveSyncWorker::SaveCodeWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
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
  return ProfileSyncServiceFactory::IsSyncAllowed(profile_)
             ? static_cast<syncer::BraveProfileSyncService*>(
                 ProfileSyncServiceFactory::GetForProfile(profile_))
             : nullptr;
}

// Most of methods below were taken from by PeopleHandler class to
// bring logic of enablind / disabling sync from deskop to Android

void BraveSyncWorker::RequestSync(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  syncer::SyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile_);

  if (service && !sync_service_observer_.IsObserving(service)) {
    sync_service_observer_.Add(service);
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

void BraveSyncWorker::FinalizeSyncSetup(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  MarkFirstSetupComplete();
}

bool BraveSyncWorker::IsFirstSetupComplete(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  syncer::SyncService* sync_service = GetSyncService();
  return sync_service &&
         sync_service->GetUserSettings()->IsFirstSetupComplete();
}

void BraveSyncWorker::ResetSync(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  auto* sync_service = GetSyncService();

  if (!sync_service)
    return;

  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::ResetSync(sync_service, device_info_sync_service,
                        base::BindOnce(&BraveSyncWorker::OnResetDone,
                                       weak_ptr_factory_.GetWeakPtr()));
}

bool BraveSyncWorker::GetSyncV1WasEnabled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  bool sync_v1_was_enabled = brave_sync_prefs.IsSyncV1Enabled();
  return sync_v1_was_enabled;
}

bool BraveSyncWorker::GetSyncV2MigrateNoticeDismissed(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  bool sync_v2_migration_notice_dismissed =
      brave_sync_prefs.IsSyncMigrateNoticeDismissed();
  return sync_v2_migration_notice_dismissed;
}

void BraveSyncWorker::SetSyncV2MigrateNoticeDismissed(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    bool sync_v2_migration_notice_dismissed) {
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.SetDismissSyncMigrateNotice(
      sync_v2_migration_notice_dismissed);
}

void BraveSyncWorker::OnResetDone() {
  syncer::SyncService* sync_service = GetSyncService();
  if (sync_service) {
    if (sync_service_observer_.IsObserving(sync_service)) {
      sync_service_observer_.Remove(sync_service);
    }
  }
}

namespace {

// A structure which contains all the configuration information for sync.
struct SyncConfigInfo {
  SyncConfigInfo();
  ~SyncConfigInfo();

  bool encrypt_all;
  std::string passphrase;
  bool set_new_passphrase;
};

SyncConfigInfo::SyncConfigInfo()
    : encrypt_all(false), set_new_passphrase(false) {}

SyncConfigInfo::~SyncConfigInfo() {}

// Return false if we are not interested configure encryption
bool FillSyncConfigInfo(syncer::SyncService* service,
                        SyncConfigInfo* configuration,
                        const std::string& passphrase) {
  bool first_setup_in_progress =
      service && !service->GetUserSettings()->IsFirstSetupComplete();

  configuration->encrypt_all =
      service->GetUserSettings()->IsEncryptEverythingEnabled();

  bool sync_prefs_passphrase_required =
      service->GetUserSettings()->IsPassphraseRequired();

  if (!first_setup_in_progress) {
    if (!configuration->encrypt_all) {
      configuration->encrypt_all = true;
      configuration->set_new_passphrase = true;
      DCHECK_NE(passphrase.size(), 0u);
      configuration->passphrase = passphrase;
    } else if (sync_prefs_passphrase_required) {
      configuration->set_new_passphrase = false;
      DCHECK_NE(passphrase.size(), 0u);
      configuration->passphrase = passphrase;
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace

void BraveSyncWorker::OnStateChanged(syncer::SyncService* sync) {
  // Fill SyncConfigInfo as it is done in
  // brave_sync_subpage.js:handleSyncPrefsChanged_ and then configure encryption
  // as in  PeopleHandler::HandleSetEncryption

  SyncConfigInfo configuration;

  syncer::SyncService* service = GetSyncService();

  // If the sync engine has shutdown for some reason, just give up
  if (!service || !service->IsEngineInitialized()) {
    VLOG(3) << "[BraveSync] " << __func__ << " sync engine is not initialized";
    return;
  }

  if (!FillSyncConfigInfo(service, &configuration, this->passphrase_)) {
    VLOG(3) << "[BraveSync] " << __func__
            << " operations with passphrase are not required";
    return;
  }

  if (!service->GetUserSettings()->IsEncryptEverythingAllowed()) {
    // Don't allow "encrypt all" if the SyncService doesn't allow it.
    // The UI is hidden, but the user may have enabled it e.g. by fiddling with
    // the web inspector.
    configuration.set_new_passphrase = false;
  }

  bool passphrase_failed = false;
  if (!configuration.passphrase.empty()) {
    // We call IsPassphraseRequired() here (instead of
    // IsPassphraseRequiredForPreferredDataTypes()) because the user may try to
    // enter a passphrase even though no encrypted data types are enabled.
    if (service->GetUserSettings()->IsPassphraseRequired()) {
      // If we have pending keys, try to decrypt them with the provided
      // passphrase. We track if this succeeds or fails because a failed
      // decryption should result in an error even if there aren't any encrypted
      // data types.
      passphrase_failed = !service->GetUserSettings()->SetDecryptionPassphrase(
          configuration.passphrase);
    } else if (service->GetUserSettings()->IsTrustedVaultKeyRequired()) {
      // There are pending keys due to trusted vault keys being required, likely
      // because something changed since the UI was displayed. A passphrase
      // cannot be set in such circumstances.
      passphrase_failed = true;
    } else {
      // OK, the user sent us a passphrase, but we don't have pending keys. So
      // it either means that the pending keys were resolved somehow since the
      // time the UI was displayed (re-encryption, pending passphrase change,
      // etc) or the user wants to re-encrypt.
      if (configuration.set_new_passphrase &&
          !service->GetUserSettings()->IsUsingSecondaryPassphrase()) {
        service->GetUserSettings()->SetEncryptionPassphrase(
            configuration.passphrase);
      }
    }
  }

  if (passphrase_failed ||
      service->GetUserSettings()->IsPassphraseRequiredForPreferredDataTypes()) {
    VLOG(1) << __func__ << " setup passphrase failed";
  }
}

static void JNI_BraveSyncWorker_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveSyncWorker(env, jcaller);
}

base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetSeedHexFromWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
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

base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetWordsFromSeedHex(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
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

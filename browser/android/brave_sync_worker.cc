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
#include "brave/components/brave_sync/qr_code_data.h"
#include "brave/components/brave_sync/qr_code_validator.h"
#include "brave/components/brave_sync/sync_service_impl_helper.h"
#include "brave/components/brave_sync/time_limited_words.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_user_settings.h"
#include "components/unified_consent/unified_consent_metrics.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/time_format.h"

// TODO(alexeybarabash): consider use of java SyncServiceImpl methods:
//    addSyncStateChangedListener
//    removeSyncStateChangedListener
//    requestStart
//    requestStop
//    setInitialSyncFeatureSetupComplete
//    isInitialSyncFeatureSetupComplete

using base::android::ConvertUTF8ToJavaString;
using brave_sync::TimeLimitedWords;
using content::BrowserThread;

namespace {
static const size_t SEED_BYTES_COUNT = 32u;
}  // namespace

namespace chrome {
namespace android {

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

base::android::ScopedJavaLocalRef<jstring> BraveSyncWorker::GetSyncCodeWords(
    JNIEnv* env) {
  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service) {
    sync_code = sync_service->GetOrCreateSyncCode();
  }

  CHECK(brave_sync::crypto::IsPassphraseValid(sync_code));

  return ConvertUTF8ToJavaString(env, sync_code);
}

void BraveSyncWorker::SaveCodeWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& passphrase) {
  std::string str_passphrase =
      base::android::ConvertJavaStringToUTF8(passphrase);

  auto* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(str_passphrase)) {
    const std::string error_msg = sync_service
                                      ? "invalid sync code:" + str_passphrase
                                      : "sync service is not available";
    LOG(ERROR) << error_msg;
    return;
  }

  passphrase_ = str_passphrase;
}

syncer::BraveSyncServiceImpl* BraveSyncWorker::GetSyncService() const {
  return SyncServiceFactory::IsSyncAllowed(profile_)
             ? static_cast<syncer::BraveSyncServiceImpl*>(
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

  // Upstream shows the notification when passphrase is required but not set,
  // see SyncErrorNotifier.computeGoalNotificationState .
  // Brave always set the passphrase eventually, so we don't need this
  // notification. The same for scanning QR code and using the codewords.
  service->GetUserSettings()
      ->MarkPassphrasePromptMutedForCurrentProductVersion();

  // Mark Sync as requested by the user. It might already be requested, but
  // it's not if this is either the first time the user is setting up Sync, or
  // Sync was set up but then was reset via the dashboard. This also pokes the
  // SyncService to start up immediately, i.e. bypass deferred startup.
  if (service) {
    service->SetSyncFeatureRequested();
  }
}

void BraveSyncWorker::MarkFirstSetupComplete() {
  syncer::SyncService* service = GetSyncService();

  // The sync service may be nullptr if it has been just disabled by policy.
  if (!service) {
    return;
  }

  service->SetSyncFeatureRequested();

  // If the first-time setup is already complete, there's nothing else to do.
  if (service->GetUserSettings()->IsInitialSyncFeatureSetupComplete()) {
    return;
  }

  unified_consent::metrics::RecordSyncSetupDataTypesHistrogam(
      service->GetUserSettings());

  // We're done configuring, so notify SyncService that it is OK to start
  // syncing.
  service->GetUserSettings()->SetInitialSyncFeatureSetupComplete(
      syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
}

void BraveSyncWorker::FinalizeSyncSetup(JNIEnv* env) {
  MarkFirstSetupComplete();
}

bool BraveSyncWorker::IsInitialSyncFeatureSetupComplete(JNIEnv* env) {
  syncer::SyncService* sync_service = GetSyncService();
  return sync_service &&
         sync_service->GetUserSettings()->IsInitialSyncFeatureSetupComplete();
}

void BraveSyncWorker::ResetSync(JNIEnv* env) {
  auto* sync_service = GetSyncService();

  if (!sync_service) {
    return;
  }

  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::ResetSync(sync_service, device_info_sync_service,
                        base::BindOnce(&BraveSyncWorker::OnResetDone,
                                       weak_ptr_factory_.GetWeakPtr()));
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
  }
}

void BraveSyncWorker::SetDecryptionPassphrase(syncer::SyncService* service) {
  DCHECK(service);
  DCHECK(service->IsEngineInitialized());
  DCHECK(!this->passphrase_.empty());
  syncer::SyncUserSettings* sync_user_settings = service->GetUserSettings();
  DCHECK(sync_user_settings->IsPassphraseRequired());

  bool set_decryption_result =
      sync_user_settings->SetDecryptionPassphrase(this->passphrase_);
  DCHECK(set_decryption_result);
  VLOG_IF(3, !set_decryption_result)
      << "[BraveSync] " << __func__ << " SetDecryptionPassphrase failed";
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

base::android::ScopedJavaLocalRef<jstring> BraveSyncWorker::GetSyncServiceURL(
    JNIEnv* env) {
  auto* sync_service = GetSyncService();
  std::string sync_url;
  if (sync_service) {
    sync_url = sync_service->GetBraveSyncServiceURL().spec();
  }

  return ConvertUTF8ToJavaString(env, sync_url);
}

base::android::ScopedJavaLocalRef<jstring>
BraveSyncWorker::GetDefaultSyncServiceURL(JNIEnv* env) {
  auto* sync_service = GetSyncService();
  std::string sync_url;
  if (sync_service) {
    sync_url = sync_service->GetBraveDefaultSyncServiceURL().spec();
  }

  return ConvertUTF8ToJavaString(env, sync_url);
}

base::android::ScopedJavaLocalRef<jstring>
BraveSyncWorker::GetCustomSyncServiceURL(JNIEnv* env) {
  auto* sync_service = GetSyncService();
  std::string custom_sync_url;
  if (sync_service) {
    custom_sync_url = sync_service->GetCustomSyncServiceURL();
  }

  return ConvertUTF8ToJavaString(env, custom_sync_url);
}

bool BraveSyncWorker::SetCustomSyncServiceURL(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& custom_sync_url) {
  std::string str_custom_sync_url =
      base::android::ConvertJavaStringToUTF8(custom_sync_url);

  auto* sync_service = GetSyncService();

  return sync_service &&
         sync_service->SetCustomSyncServiceURL(str_custom_sync_url);
}

namespace {
std::string GetErrorDescription(
    const syncer::SyncProtocolError& sync_protocol_error) {
  if (sync_protocol_error.error_type == syncer::SYNC_SUCCESS) {
    return std::string();
  } else if (sync_protocol_error.error_type != syncer::SYNC_SUCCESS &&
             sync_protocol_error.error_description.empty()) {
    return GetSyncErrorTypeString(sync_protocol_error.error_type);
  } else {
    return sync_protocol_error.error_description;
  }
}

void NativePermanentlyDeleteAccountCallback(
    JNIEnv* env,
    const base::android::ScopedJavaGlobalRef<jobject>& callback,
    const syncer::SyncProtocolError& sync_protocol_error) {
  std::string error_description = GetErrorDescription(sync_protocol_error);

  Java_BraveSyncWorker_onPermanentlyDeleteAccountResult(
      env, callback, ConvertUTF8ToJavaString(env, error_description));
}
}  // namespace

void BraveSyncWorker::PermanentlyDeleteAccount(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* sync_service = GetSyncService();
  CHECK_NE(sync_service, nullptr);

  base::android::ScopedJavaGlobalRef<jobject> java_callback;
  java_callback.Reset(env, callback);

  sync_service->PermanentlyDeleteAccount(base::BindOnce(
      &NativePermanentlyDeleteAccountCallback, env, java_callback));
}

namespace {

base::android::ScopedJavaLocalRef<jobject> GetJavaBoolean(
    JNIEnv* env,
    const bool& native_bool) {
  jclass booleanClass = env->FindClass("java/lang/Boolean");
  jmethodID methodID = env->GetMethodID(booleanClass, "<init>", "(Z)V");
  jobject booleanObject = env->NewObject(booleanClass, methodID, native_bool);

  return base::android::ScopedJavaLocalRef<jobject>(env, booleanObject);
}

void NativeJoinSyncChainCallback(
    JNIEnv* env,
    const base::android::ScopedJavaGlobalRef<jobject>& callback,
    bool result) {
  Java_BraveSyncWorker_onJoinSyncChainResult(env, callback,
                                             GetJavaBoolean(env, result));
}

}  // namespace

void BraveSyncWorker::SetJoinSyncChainCallback(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* sync_service = GetSyncService();
  CHECK_NE(sync_service, nullptr);

  base::android::ScopedJavaGlobalRef<jobject> java_callback;
  java_callback.Reset(env, callback);

  sync_service->SetJoinChainResultCallback(
      base::BindOnce(&NativeJoinSyncChainCallback, env, java_callback));
}

void BraveSyncWorker::ClearAccountDeletedNoticePending(JNIEnv* env) {
  Profile* profile =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
}

bool BraveSyncWorker::IsAccountDeletedNoticePending(JNIEnv* env) {
  Profile* profile =
      ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
  brave_sync::Prefs brave_sync_prefs(profile->GetPrefs());
  return brave_sync_prefs.IsSyncAccountDeletedNoticePending();
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

  return ConvertUTF8ToJavaString(env, sync_code_hex);
}

std::string GetWordsFromSeedHex(const std::string& str_seed_hex) {
  DCHECK(!str_seed_hex.empty());

  std::vector<uint8_t> bytes;
  std::string sync_code_words;
  if (base::HexStringToBytes(str_seed_hex, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    if (bytes.size(), SEED_BYTES_COUNT) {
      sync_code_words = brave_sync::crypto::PassphraseFromBytes32(bytes);
      if (sync_code_words.empty()) {
        VLOG(1) << __func__ << " PassphraseFromBytes32 failed for "
                << str_seed_hex;
      }
    } else {
      LOG(ERROR) << "wrong seed bytes " << bytes.size();
    }
    DCHECK_NE(sync_code_words, "");
  } else {
    VLOG(1) << __func__ << " HexStringToBytes failed for " << str_seed_hex;
  }

  return sync_code_words;
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetWordsFromSeedHex(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& seed_hex) {
  std::string str_seed_hex = base::android::ConvertJavaStringToUTF8(seed_hex);
  std::string sync_code_words = GetWordsFromSeedHex(str_seed_hex);
  return ConvertUTF8ToJavaString(env, sync_code_words);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetQrDataJson(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& seed_hex) {
  std::string str_seed_hex = base::android::ConvertJavaStringToUTF8(seed_hex);
  DCHECK(!str_seed_hex.empty());

  const std::string qr_code_string =
      brave_sync::QrCodeData::CreateWithActualDate(str_seed_hex)->ToJson();

  return ConvertUTF8ToJavaString(env, qr_code_string);
}

int JNI_BraveSyncWorker_GetQrCodeValidationResult(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& json_qr) {
  std::string str_json_qr = base::android::ConvertJavaStringToUTF8(json_qr);
  DCHECK(!str_json_qr.empty());
  return static_cast<int>(
      brave_sync::QrCodeDataValidator::ValidateQrDataJson(str_json_qr));
}

int JNI_BraveSyncWorker_GetWordsValidationResult(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& time_limited_words) {
  std::string str_time_limited_words =
      base::android::ConvertJavaStringToUTF8(time_limited_words);
  DCHECK(!str_time_limited_words.empty());

  auto pure_words_with_status = TimeLimitedWords::Parse(str_time_limited_words);

  if (pure_words_with_status.has_value()) {
    using ValidationStatus = TimeLimitedWords::ValidationStatus;
    return static_cast<int>(ValidationStatus::kValid);
  }
  return static_cast<int>(pure_words_with_status.error());
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetPureWordsFromTimeLimited(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& time_limited_words) {
  std::string str_time_limited_words =
      base::android::ConvertJavaStringToUTF8(time_limited_words);
  DCHECK(!str_time_limited_words.empty());

  auto pure_words_with_status = TimeLimitedWords::Parse(str_time_limited_words);
  DCHECK(pure_words_with_status.has_value());

  return base::android::ConvertUTF8ToJavaString(env,
                                                pure_words_with_status.value());
}

static int64_t JNI_BraveSyncWorker_GetNotAfterFromFromTimeLimitedWords(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& time_limited_words) {
  std::string str_time_limited_words =
      base::android::ConvertJavaStringToUTF8(time_limited_words);
  DCHECK(!str_time_limited_words.empty());

  auto not_after = TimeLimitedWords::GetNotAfter(str_time_limited_words);

  return not_after.InMillisecondsSinceUnixEpoch() / 1000;
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetFormattedTimeDelta(JNIEnv* env, jlong seconds) {
  auto delta = base::Seconds(seconds);

  using ui::TimeFormat;
  std::u16string duration_string;
  if (delta.InDays() > 0) {
    duration_string += TimeFormat::Detailed(TimeFormat::FORMAT_DURATION,
                                            TimeFormat::LENGTH_LONG, 0,
                                            base::Days(delta.InDays()));
    duration_string += u" ";
  }

  int remaining_hours =
      delta.InHours() - delta.InDays() * base::Time::kHoursPerDay;
  if (remaining_hours > 0) {
    duration_string += TimeFormat::Detailed(TimeFormat::FORMAT_DURATION,
                                            TimeFormat::LENGTH_LONG, 0,
                                            base::Hours(remaining_hours));
    duration_string += u" ";
  }

  int remaining_minutes =
      delta.InMinutes() - delta.InHours() * base::Time::kMinutesPerHour;
  if (remaining_minutes > 0) {
    duration_string += TimeFormat::Detailed(TimeFormat::FORMAT_DURATION,
                                            TimeFormat::LENGTH_LONG, 0,
                                            base::Minutes(remaining_minutes));
    duration_string += u" ";
  }

  int remaining_seconds =
      delta.InSeconds() - delta.InMinutes() * base::Time::kSecondsPerMinute;
  if (remaining_seconds > 0) {
    duration_string += TimeFormat::Detailed(TimeFormat::FORMAT_DURATION,
                                            TimeFormat::LENGTH_LONG, 0,
                                            base::Seconds(remaining_seconds));
    duration_string += u" ";
  }

  if (!duration_string.empty()) {
    duration_string.resize(duration_string.length() - 1);
  }

  return base::android::ConvertUTF16ToJavaString(env, duration_string);
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetTimeLimitedWordsFromPure(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& pure_words) {
  std::string str_pure_words =
      base::android::ConvertJavaStringToUTF8(pure_words);
  DCHECK(!str_pure_words.empty());

  auto time_limited_words = TimeLimitedWords::GenerateForNow(str_pure_words);

  DCHECK(time_limited_words.has_value());
  return base::android::ConvertUTF8ToJavaString(env,
                                                time_limited_words.value());
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveSyncWorker_GetSeedHexFromQrJson(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& json_qr) {
  std::string str_json_qr = base::android::ConvertJavaStringToUTF8(json_qr);
  DCHECK(!str_json_qr.empty());

  auto qr_data = brave_sync::QrCodeData::FromJson(str_json_qr);

  std::string result;
  if (qr_data) {
    result = qr_data->sync_code_hex;
  } else {
    result = str_json_qr;
  }

  DCHECK(!GetWordsFromSeedHex(result).empty());

  return ConvertUTF8ToJavaString(env, result);
}

static int JNI_BraveSyncWorker_GetWordsCount(
    JNIEnv* env,
    const base::android::JavaParamRef<jstring>& words) {
  return TimeLimitedWords::GetWordsCount(
      base::android::ConvertJavaStringToUTF8(words));
}

}  // namespace android
}  // namespace chrome

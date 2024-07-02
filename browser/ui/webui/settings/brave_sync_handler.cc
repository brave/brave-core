// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_sync_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/qr_code_data.h"
#include "brave/components/brave_sync/sync_service_impl_helper.h"
#include "brave/components/brave_sync/time_limited_words.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "brave/components/sync_device_info/brave_device_info.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/qr_code_generator/bitmap_generator.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/service/sync_user_settings.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

using brave_sync::TimeLimitedWords;

namespace {

std::string GetSyncCodeValidationString(
    TimeLimitedWords::ValidationStatus validation_result) {
  using ValidationStatus = TimeLimitedWords::ValidationStatus;
  switch (validation_result) {
    case ValidationStatus::kWrongWordsNumber:
    case ValidationStatus::kNotValidPureWords:
      return l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_CODE_INVALID);
    case ValidationStatus::kVersionDeprecated:
      return l10n_util::GetStringUTF8(
          IDS_BRAVE_SYNC_CODE_FROM_DEPRECATED_VERSION);
    case ValidationStatus::kExpired:
      return l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_CODE_EXPIRED);
    case ValidationStatus::kValidForTooLong:
      return l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_CODE_VALID_FOR_TOO_LONG);
    default:
      NOTREACHED_IN_MIGRATION();
      return "";
  }
}

}  // namespace

BraveSyncHandler::BraveSyncHandler() : weak_ptr_factory_(this) {}

BraveSyncHandler::~BraveSyncHandler() = default;

void BraveSyncHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  web_ui()->RegisterMessageCallback(
      "SyncGetDeviceList",
      base::BindRepeating(&BraveSyncHandler::HandleGetDeviceList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncSetupSetSyncCode",
      base::BindRepeating(&BraveSyncHandler::HandleSetSyncCode,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncSetupGetSyncCode",
      base::BindRepeating(&BraveSyncHandler::HandleGetSyncCode,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncSetupGetPureSyncCode",
      base::BindRepeating(&BraveSyncHandler::HandleGetPureSyncCode,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncGetQRCode", base::BindRepeating(&BraveSyncHandler::HandleGetQRCode,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncSetupReset", base::BindRepeating(&BraveSyncHandler::HandleReset,
                                            base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncDeleteDevice",
      base::BindRepeating(&BraveSyncHandler::HandleDeleteDevice,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "SyncPermanentlyDeleteAccount",
      base::BindRepeating(&BraveSyncHandler::HandlePermanentlyDeleteAccount,
                          base::Unretained(this)));
}

void BraveSyncHandler::OnJavascriptAllowed() {
  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  if (tracker) {
    device_info_tracker_observer_.Reset();
    device_info_tracker_observer_.Observe(tracker);
  }
}

void BraveSyncHandler::OnJavascriptDisallowed() {
  device_info_tracker_observer_.Reset();
}

void BraveSyncHandler::OnDeviceInfoChange() {
  if (IsJavascriptAllowed())
    FireWebUIListener("device-info-changed", GetSyncDeviceList());
}

void BraveSyncHandler::HandleGetDeviceList(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());
  ResolveJavascriptCallback(args[0], GetSyncDeviceList());
}

void BraveSyncHandler::HandleGetSyncCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service)
    sync_code = sync_service->GetOrCreateSyncCode();

  auto time_limited_sync_code = TimeLimitedWords::GenerateForNow(sync_code);
  if (time_limited_sync_code.has_value()) {
    ResolveJavascriptCallback(args[0],
                              base::Value(time_limited_sync_code.value()));
  } else {
    LOG(ERROR) << "Failed to generate time limited sync code, "
               << TimeLimitedWords::GenerateResultToText(
                      time_limited_sync_code.error());
    RejectJavascriptCallback(args[0], base::Value());
  }
}

void BraveSyncHandler::HandleGetPureSyncCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service)
    sync_code = sync_service->GetOrCreateSyncCode();

  ResolveJavascriptCallback(args[0], base::Value(sync_code));
}

void BraveSyncHandler::HandleGetQRCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string time_limited_sync_code = args[1].GetString();

  // Sync code arrives here with time-limit 25th word, remove it to get proper
  // pure seed for QR generation  (QR codes have their own expiry)
  auto pure_words_with_status =
      TimeLimitedWords::ParseIgnoreDate(time_limited_sync_code);
  CHECK(pure_words_with_status.has_value());
  CHECK_NE(pure_words_with_status.value().size(), 0u);

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(pure_words_with_status.value(),
                                               &seed)) {
    LOG(ERROR) << "invalid sync code when generating qr code";
    RejectJavascriptCallback(args[0], base::Value("invalid sync code"));
    return;
  }

  // QR code version 3 can only carry 84 bytes so we hex encode 32 bytes
  // seed then we will have 64 bytes input data
  const std::string sync_code_hex = base::HexEncode(seed.data(), seed.size());
  const std::string qr_code_string =
      brave_sync::QrCodeData::CreateWithActualDate(sync_code_hex)->ToJson();

  auto qr_image = qr_code_generator::GenerateBitmap(
      base::as_byte_span(qr_code_string),
      qr_code_generator::ModuleStyle::kCircles,
      qr_code_generator::LocatorStyle::kRounded,
      qr_code_generator::CenterImage::kDino,
      qr_code_generator::QuietZone::kWillBeAddedByClient);

  if (!qr_image.has_value()) {
    VLOG(1) << "QR code generator failure: "
            << base::to_underlying(qr_image.error());
    ResolveJavascriptCallback(args[0].Clone(), base::Value(false));
    return;
  }

  const std::string data_url = webui::GetBitmapDataUrl(qr_image.value());
  VLOG(1) << "QR code data url: " << data_url;
  ResolveJavascriptCallback(args[0].Clone(), base::Value(data_url));
}

void BraveSyncHandler::HandleSetSyncCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string time_limited_sync_code = args[1].GetString();
  if (time_limited_sync_code.empty()) {
    LOG(ERROR) << "No sync code parameter provided!";
    RejectJavascriptCallback(
        args[0].Clone(), l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_CODE_EMPTY));
    return;
  }

  auto pure_words_with_status = TimeLimitedWords::Parse(time_limited_sync_code);

  if (!pure_words_with_status.has_value()) {
    LOG(ERROR) << "Could not validate a sync code, validation_result="
               << static_cast<int>(pure_words_with_status.error()) << " "
               << GetSyncCodeValidationString(pure_words_with_status.error());
    RejectJavascriptCallback(args[0], base::Value(GetSyncCodeValidationString(
                                          pure_words_with_status.error())));
    return;
  }

  CHECK(!pure_words_with_status.value().empty());

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    LOG(ERROR) << "Cannot get sync_service";
    RejectJavascriptCallback(
        args[0].Clone(),
        l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_INTERNAL_SETUP_ERROR));
    return;
  }

  base::Value callback_id_arg(args[0].Clone());
  sync_service->SetJoinChainResultCallback(base::BindOnce(
      &BraveSyncHandler::OnJoinChainResult, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback_id_arg)));

  if (!sync_service->SetSyncCode(pure_words_with_status.value())) {
    RejectJavascriptCallback(
        args[0].Clone(),
        l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_INTERNAL_SETUP_ERROR));
    return;
  }

  // Originally it was invoked through
  // #2 syncer::SyncPrefs::SetSyncRequested()
  // #3 settings::PeopleHandler::MarkFirstSetupComplete()
  // #4 settings::PeopleHandler::OnDidClosePage()
  // #4 brave_sync_subpage.js didNavigateAwayFromSyncPage()
  // #5 brave_sync_subpage.js onNavigateAwayFromPage_()
  // But we forcing it here because we need detect the case when we are trying
  // to join the deleted chain. So we allow Sync system to proceed and then
  // we will set the result at BraveSyncHandler::OnJoinChainResult.
  // Otherwise we will not let to send request to the server.

  sync_service->SetSyncFeatureRequested();
  sync_service->GetUserSettings()->SetInitialSyncFeatureSetupComplete(
      syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
}

void BraveSyncHandler::OnJoinChainResult(base::Value callback_id, bool result) {
  if (result) {
    ResolveJavascriptCallback(callback_id, base::Value(true));
  } else {
    std::string errorText =
        l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_JOINING_DELETED_ACCOUNT);
    RejectJavascriptCallback(callback_id, base::Value(errorText));
  }
}

void BraveSyncHandler::HandleReset(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    ResolveJavascriptCallback(args[0], base::Value(true));
    return;
  }

  base::Value callback_id_arg(args[0].Clone());
  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::ResetSync(sync_service, device_info_sync_service,
                        base::BindOnce(&BraveSyncHandler::OnResetDone,
                                       weak_ptr_factory_.GetWeakPtr(),
                                       std::move(callback_id_arg)));
}

void BraveSyncHandler::OnAccountPermanentlyDeleted(
    base::Value callback_id,
    const syncer::SyncProtocolError& sync_protocol_error) {
  if (sync_protocol_error.error_description.empty()) {
    ResolveJavascriptCallback(callback_id, base::Value(true));
  } else {
    RejectJavascriptCallback(
        callback_id, base::Value(sync_protocol_error.error_description));
  }
}

void BraveSyncHandler::HandlePermanentlyDeleteAccount(
    const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    RejectJavascriptCallback(
        args[0].Clone(),
        l10n_util::GetStringUTF8(IDS_BRAVE_SYNC_INTERNAL_ACCOUNT_DELETE_ERROR));
    return;
  }

  base::Value callback_id_arg(args[0].Clone());
  sync_service->PermanentlyDeleteAccount(base::BindOnce(
      &BraveSyncHandler::OnAccountPermanentlyDeleted,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback_id_arg)));
}

void BraveSyncHandler::HandleDeleteDevice(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string device_guid = args[1].GetString();

  if (device_guid.empty()) {
    LOG(ERROR) << "No device id to remove!";
    RejectJavascriptCallback(args[0], base::Value(false));
    return;
  }

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    ResolveJavascriptCallback(args[0], base::Value(false));
    return;
  }

  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::DeleteDevice(sync_service, device_info_sync_service, device_guid);
  ResolveJavascriptCallback(args[0], base::Value(true));
}

syncer::BraveSyncServiceImpl* BraveSyncHandler::GetSyncService() const {
  return SyncServiceFactory::IsSyncAllowed(profile_)
             ? static_cast<syncer::BraveSyncServiceImpl*>(
                   SyncServiceFactory::GetForProfile(profile_))
             : nullptr;
}

syncer::DeviceInfoTracker* BraveSyncHandler::GetDeviceInfoTracker() const {
  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  return device_info_sync_service->GetDeviceInfoTracker();
}

syncer::LocalDeviceInfoProvider* BraveSyncHandler::GetLocalDeviceInfoProvider()
    const {
  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  return device_info_sync_service->GetLocalDeviceInfoProvider();
}

void BraveSyncHandler::OnResetDone(base::Value callback_id) {
  ResolveJavascriptCallback(callback_id, base::Value(true));
}

base::Value::List BraveSyncHandler::GetSyncDeviceList() {
  AllowJavascript();
  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  const syncer::DeviceInfo* local_device_info =
      GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  base::Value::List device_list;

  for (const auto& device : tracker->GetAllBraveDeviceInfo()) {
    auto device_value = device->ToValue();
    bool is_current_device =
        local_device_info ? local_device_info->guid() == device->guid() : false;
    device_value.Set("isCurrentDevice", is_current_device);
    device_value.Set("guid", device->guid());
    device_value.Set("supportsSelfDelete",
                     !is_current_device && device->is_self_delete_supported());

    device_list.Append(std::move(device_value));
  }

  return device_list;
}

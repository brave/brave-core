// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_sync_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "content/public/browser/web_ui.h"

namespace {

std::string StrFromUint8Array(const std::vector<uint8_t>& arr) {
  std::string result;
  for (size_t i = 0; i < arr.size(); ++i) {
    result += base::NumberToString(static_cast<unsigned char>(arr.at(i)));
    if (i != arr.size() - 1) {
      result += ", ";
    }
  }
  return result;
}

}  // namespace

BraveSyncHandler::BraveSyncHandler() : weak_ptr_factory_(this) {}
BraveSyncHandler::~BraveSyncHandler() {}

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
      "SyncGetQRCode", base::BindRepeating(&BraveSyncHandler::HandleGetQRCode,
                                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "SyncSetupReset", base::BindRepeating(&BraveSyncHandler::HandleReset,
                                            base::Unretained(this)));
}

void BraveSyncHandler::OnJavascriptAllowed() {
  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  if (tracker)
    device_info_tracker_observer_.Add(tracker);
}

void BraveSyncHandler::OnJavascriptDisallowed() {
  device_info_tracker_observer_.RemoveAll();
}

void BraveSyncHandler::OnDeviceInfoChange() {
  if (IsJavascriptAllowed())
    FireWebUIListener("device-info-changed", GetSyncDeviceList());
}

void BraveSyncHandler::HandleGetDeviceList(const base::ListValue* args) {
  AllowJavascript();
  const auto& list = args->GetList();
  CHECK_EQ(1U, list.size());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  ResolveJavascriptCallback(*callback_id, GetSyncDeviceList());
}

void BraveSyncHandler::HandleGetSyncCode(const base::ListValue* args) {
  AllowJavascript();

  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));

  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  std::string sync_code = brave_sync_prefs.GetSeed();
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
  }

  ResolveJavascriptCallback(*callback_id, base::Value(sync_code));
}

void BraveSyncHandler::HandleGetQRCode(const base::ListValue* args) {
  AllowJavascript();
  CHECK_EQ(2U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  const base::Value* sync_code;
  CHECK(args->Get(1, &sync_code));

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code->GetString(), &seed)) {
    LOG(ERROR) << "invalid sync code";
    ResolveJavascriptCallback(*callback_id, base::Value(false));
    return;
  }

  std::string seed_serialized = StrFromUint8Array(seed);

  // TODO(petemill): Use QRCodeGenerator
  // const char* input_data_string = sync_code.c_str();
  // auto input_data = base::span<const uint8_t>(
  //         reinterpret_cast<const uint8_t*>(input_data_string),
  //             strlen(input_data_string));
  // // Generate QR code data
  // uint8_t qr_data_buf[QRCode::kInputBytes];
  // QRCodeGenerator generator;
  // base::span<const uint8_t, QRCodeGenerator::kTotalSize> code =
  //     generator.Generate(input_data);
  ResolveJavascriptCallback(*callback_id, base::Value(seed_serialized));
  // ResolveJavascriptCallback(*callback_id, base::Value(false));
}

void BraveSyncHandler::HandleSetSyncCode(const base::ListValue* args) {
  AllowJavascript();
  CHECK_EQ(2U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  const base::Value* sync_code;
  CHECK(args->Get(1, &sync_code));

  if (sync_code->GetString().empty()) {
    LOG(ERROR) << "No sync code parameter provided!";
    RejectJavascriptCallback(*callback_id, base::Value(false));
    return;
  }

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code->GetString(), &seed)) {
    LOG(ERROR) << "invalid sync code";
    RejectJavascriptCallback(*callback_id, base::Value(false));
    return;
  }
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  if (!brave_sync_prefs.SetSeed(sync_code->GetString())) {
    ResolveJavascriptCallback(*callback_id, base::Value(false));
    return;
  }
  ResolveJavascriptCallback(*callback_id, base::Value(true));
}

void BraveSyncHandler::HandleReset(const base::ListValue* args) {
  AllowJavascript();
  CHECK_EQ(1U, args->GetSize());
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  base::Value callback_id_arg(callback_id->Clone());

  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  const syncer::DeviceInfo* local_device_info =
      GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  tracker->DeleteDeviceInfo(local_device_info->guid(),
                            base::BindOnce(&BraveSyncHandler::OnSelfDeleted,
                                           weak_ptr_factory_.GetWeakPtr(),
                                           std::move(callback_id_arg)));
}

syncer::SyncService* BraveSyncHandler::GetSyncService() const {
  return ProfileSyncServiceFactory::IsSyncAllowed(profile_)
             ? ProfileSyncServiceFactory::GetForProfile(profile_)
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

void BraveSyncHandler::OnSelfDeleted(base::Value callback_id) {
  auto* sync_service = GetSyncService();
  if (sync_service) {
    sync_service->GetUserSettings()->SetSyncRequested(false);
    sync_service->StopAndClear();
  }
  brave_sync::Prefs brave_sync_prefs(profile_->GetPrefs());
  brave_sync_prefs.Clear();
  // Sync prefs will be clear in ProfileSyncService::StopImpl

  ResolveJavascriptCallback(callback_id, base::Value());
}

base::Value BraveSyncHandler::GetSyncDeviceList() {
  AllowJavascript();
  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  const syncer::DeviceInfo* local_device_info =
      GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  base::Value device_list(base::Value::Type::LIST);

  for (const auto& device : tracker->GetAllDeviceInfo()) {
    auto device_value = base::Value::FromUniquePtrValue(device->ToValue());
    bool is_current_device =
        local_device_info ? local_device_info->guid() == device->guid() : false;
    device_value.SetBoolKey("isCurrentDevice", is_current_device);
    device_list.Append(std::move(device_value));
  }
  return device_list;
}

// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_sync_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/qr_code_data.h"
#include "brave/components/brave_sync/sync_service_impl_helper.h"
#include "brave/components/sync/driver/brave_sync_service_impl.h"
#include "brave/components/sync_device_info/brave_device_info.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/webui/web_ui_util.h"

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

  web_ui()->RegisterMessageCallback(
      "SyncDeleteDevice",
      base::BindRepeating(&BraveSyncHandler::HandleDeleteDevice,
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
  ResolveJavascriptCallback(args[0].Clone(), GetSyncDeviceList());
}

void BraveSyncHandler::HandleGetSyncCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service)
    sync_code = sync_service->GetOrCreateSyncCode();

  ResolveJavascriptCallback(args[0].Clone(), base::Value(sync_code));
}

void BraveSyncHandler::HandleGetQRCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string sync_code = args[1].GetString();

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed)) {
    LOG(ERROR) << "invalid sync code when generating qr code";
    RejectJavascriptCallback(args[0].Clone(), base::Value("invalid sync code"));
    return;
  }

  // QR code version 3 can only carry 84 bytes so we hex encode 32 bytes
  // seed then we will have 64 bytes input data
  const std::string sync_code_hex = base::HexEncode(seed.data(), seed.size());
  const std::string qr_code_string =
      brave_sync::QrCodeData::CreateWithActualDate(sync_code_hex)->ToJson();

  base::Value callback_id_disconnect(args[0].Clone());
  base::Value callback_id_arg(args[0].Clone());

  qr_code_service_remote_ = qrcode_generator::LaunchQRCodeGeneratorService();
  qr_code_service_remote_.set_disconnect_handler(
      base::BindOnce(&BraveSyncHandler::OnCodeGeneratorResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback_id_disconnect), nullptr));
  qrcode_generator::mojom::QRCodeGeneratorService* generator =
      qr_code_service_remote_.get();

  qrcode_generator::mojom::GenerateQRCodeRequestPtr request =
      qrcode_generator::mojom::GenerateQRCodeRequest::New();
  request->data = qr_code_string;
  request->should_render = true;
  request->render_dino = false;
  request->render_module_style =
      qrcode_generator::mojom::ModuleStyle::DEFAULT_SQUARES;
  request->render_locator_style =
      qrcode_generator::mojom::LocatorStyle::DEFAULT_SQUARE;

  generator->GenerateQRCode(
      std::move(request),
      base::BindOnce(&BraveSyncHandler::OnCodeGeneratorResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback_id_arg)));
}

void BraveSyncHandler::HandleSetSyncCode(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string sync_code = args[1].GetString();

  if (sync_code.empty()) {
    LOG(ERROR) << "No sync code parameter provided!";
    RejectJavascriptCallback(args[0].Clone(), base::Value(false));
    return;
  }

  auto* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(sync_code)) {
    RejectJavascriptCallback(args[0].Clone(), base::Value(false));
    return;
  }

  ResolveJavascriptCallback(args[0].Clone(), base::Value(true));
}

void BraveSyncHandler::HandleReset(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(1U, args.size());

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    ResolveJavascriptCallback(args[0].Clone(), base::Value(true));
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

void BraveSyncHandler::HandleDeleteDevice(const base::Value::List& args) {
  AllowJavascript();
  CHECK_EQ(2U, args.size());
  CHECK(args[1].is_string());
  const std::string device_guid = args[1].GetString();

  if (device_guid.empty()) {
    LOG(ERROR) << "No device id to remove!";
    RejectJavascriptCallback(args[0].Clone(), base::Value(false));
    return;
  }

  auto* sync_service = GetSyncService();
  if (!sync_service) {
    ResolveJavascriptCallback(args[0].Clone(), base::Value(false));
    return;
  }

  auto* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile_);
  brave_sync::DeleteDevice(sync_service, device_info_sync_service, device_guid);
  ResolveJavascriptCallback(args[0].Clone(), base::Value(true));
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

base::Value BraveSyncHandler::GetSyncDeviceList() {
  AllowJavascript();
  syncer::DeviceInfoTracker* tracker = GetDeviceInfoTracker();
  DCHECK(tracker);
  const syncer::DeviceInfo* local_device_info =
      GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();

  base::Value device_list(base::Value::Type::LIST);

  for (const auto& device : tracker->GetAllBraveDeviceInfo()) {
    auto device_value = base::Value::FromUniquePtrValue(device->ToValue());
    bool is_current_device =
        local_device_info ? local_device_info->guid() == device->guid() : false;
    device_value.SetBoolKey("isCurrentDevice", is_current_device);
    device_value.SetStringKey("guid", device->guid());
    device_value.SetBoolKey(
        "supportsSelfDelete",
        !is_current_device && device->is_self_delete_supported());

    device_list.Append(std::move(device_value));
  }

  return device_list;
}

void BraveSyncHandler::OnCodeGeneratorResponse(
    base::Value callback_id,
    const qrcode_generator::mojom::GenerateQRCodeResponsePtr response) {
  if (!response || response->error_code !=
                       qrcode_generator::mojom::QRCodeGeneratorError::NONE) {
    VLOG(1) << "QR code generator failure: " << response->error_code;
    ResolveJavascriptCallback(callback_id, base::Value(false));
    return;
  }

  const std::string data_url = webui::GetBitmapDataUrl(response->bitmap);
  VLOG(1) << "QR code data url: " << data_url;

  qr_code_service_remote_.reset();
  ResolveJavascriptCallback(callback_id, base::Value(data_url));
}

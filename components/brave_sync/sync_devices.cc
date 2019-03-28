/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/sync_devices.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_sync/jslib_const.h"

namespace brave_sync {

SyncDevice::SyncDevice() :
  last_active_ts_(0) {
}

SyncDevice::SyncDevice(const SyncDevice& other) = default;

SyncDevice::SyncDevice(const std::string& name,
  const std::string& object_id,
  const std::string& device_id,
  const double last_active_ts) :
  name_(name),
  object_id_(object_id),
  device_id_(device_id),
  last_active_ts_(last_active_ts) {
}

SyncDevice& SyncDevice::operator=(const SyncDevice&) & = default;

SyncDevice::~SyncDevice() = default;

std::unique_ptr<base::Value> SyncDevice::ToValue() const {
  auto val_sync_device =
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

  val_sync_device->SetKey("name", base::Value(name_));
  val_sync_device->SetKey("object_id", base::Value(object_id_));
  val_sync_device->SetKey("device_id", base::Value(device_id_));
  val_sync_device->SetKey("last_active", base::Value(last_active_ts_));

  return val_sync_device;
}

SyncDevices::SyncDevices() = default;
SyncDevices::~SyncDevices() = default;

std::string SyncDevices::ToJson() const {
  // devices_ => base::Value => json
  std::string json;
  bool result = base::JSONWriter::WriteWithOptions(
    *this->ToValue(),
    0,
    &json);

  DCHECK(result);
  return json;
}

std::unique_ptr<base::Value> SyncDevices::ToValue() const {
  auto val_sync_device =
      std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

  auto arr_devices = std::make_unique<base::Value>(base::Value::Type::LIST);
  for (const SyncDevice &device : devices_) {
    arr_devices->GetList().push_back(std::move(*device.ToValue()));
  }

  val_sync_device->SetKey("devices", std::move(*arr_devices));

  return val_sync_device;
}

std::unique_ptr<base::Value> SyncDevices::ToValueArrOnly() const {
  auto arr_devices = std::make_unique<base::Value>(base::Value::Type::LIST);
  for (const SyncDevice &device : devices_) {
    arr_devices->GetList().push_back(std::move(*device.ToValue()));
  }

  return arr_devices;
}

void SyncDevices::FromJson(const std::string& str_json) {
  if (str_json.empty()) {
    devices_.clear();
    return;
  }

  // JSON ==> Value
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          str_json, base::JSONParserOptions::JSON_PARSE_RFC);
  base::Optional<base::Value>& records_v = value_with_error.value;

  DCHECK(records_v);
  if (!records_v) {
    return;
  }

  devices_.clear();
  const base::Value* pv_arr = records_v->FindKey("devices");
  CHECK(pv_arr->is_list());
  for (const base::Value &val : pv_arr->GetList()) {
    std::string name = val.FindKey("name")->GetString();
    std::string object_id = val.FindKey("object_id")->GetString();
    std::string device_id = val.FindKey("device_id")->GetString();
    double last_active = 0;
    const base::Value *v_last_active = val.FindKey("last_active");
    if (v_last_active->is_double()) {
      last_active = v_last_active->GetDouble();
    } else {
      LOG(WARNING) << "SyncDevices::FromJson: last_active is not a double";
    }

    devices_.push_back(SyncDevice(
      name,
      object_id,
      device_id,
      last_active) );
  }
}

void SyncDevices::Merge(const SyncDevice& device,
                        int action,
                        bool* actually_merged) {
  *actually_merged = false;
  auto existing_it = std::find_if(std::begin(devices_),
                                  std::end(devices_),
      [device](const SyncDevice &cur_dev) {
        return cur_dev.object_id_ == device.object_id_;
      });

  switch (action) {
    case jslib_const::kActionCreate: {
      if (existing_it == std::end(devices_)) {
        devices_.push_back(device);
        *actually_merged = true;
      } else {
        // ignoring create, already have device
      }
      break;
    }
    case jslib_const::kActionUpdate: {
      DCHECK(existing_it != std::end(devices_));
      *existing_it = device;
      *actually_merged = true;
      break;
    }
    case jslib_const::kActionDelete: {
      // Sync js lib does not merge several DELETE records into one,
      // at this point existing_it can be equal to std::end(devices_)
      if (existing_it != std::end(devices_)) {
        devices_.erase(existing_it);
        *actually_merged = true;
      } else {
        // ignoring delete, already deleted
      }
      break;
    }
  }
}

SyncDevice* SyncDevices::GetByObjectId(const std::string &object_id) {
  for (auto& device : devices_) {
    if (device.object_id_ == object_id) {
      return &device;
    }
  }

  return nullptr;
}

const SyncDevice* SyncDevices::GetByDeviceId(const std::string &device_id) {
  for (const auto& device : devices_) {
    if (device.device_id_ == device_id) {
      return &device;
    }
  }

  return nullptr;
}

void SyncDevices::DeleteByObjectId(const std::string &object_id) {
  auto existing_it =
      std::find_if(std::begin(devices_),
                    std::end(devices_),
                    [object_id](const SyncDevice &dev) {
                      return dev.object_id_ == object_id;
                    });

  if (existing_it != std::end(devices_)) {
    devices_.erase(existing_it);
  } else {
    // TODO(bridiver) - is this correct?
    NOTREACHED();
  }
}

}  // namespace brave_sync

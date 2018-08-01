/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_devices.h"

#include "base/values.h"
#include "base/json/json_writer.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"

#include "brave/components/brave_sync/brave_sync_jslib_const.h"
#include "brave/components/brave_sync/value_debug.h"

namespace brave_sync {

SyncDevice::SyncDevice() :
  last_active_ts_(0) {
}

SyncDevice::SyncDevice(const SyncDevice& other) = default;

SyncDevice::SyncDevice(const std::string &name,
  const std::string &object_id,
  const std::string &device_id,
  const double &last_active_ts) :
  name_(name),
  object_id_(object_id),
  device_id_(device_id),
  last_active_ts_(last_active_ts) {
}

SyncDevice& SyncDevice::operator=(const SyncDevice&) & = default;

SyncDevice::~SyncDevice() = default;

std::unique_ptr<base::Value> SyncDevice::ToValue() const {
  auto val_sync_device = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

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
  auto val_sync_device = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);

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

void SyncDevices::FromJson(const std::string &str_json) {
  LOG(ERROR) << "TAGAB SyncDevices::FromJson: str_json=<" << str_json << ">";
  if (str_json.empty()) {
    devices_.clear();
    return;
  }

  //JSON ==> Value
  int error_code_out = 0;
  std::string error_msg_out;
  int error_line_out = 0;
  int error_column_out = 0;
  std::unique_ptr<base::Value> records_v = base::JSONReader::ReadAndReturnError(
    str_json,
    base::JSONParserOptions::JSON_PARSE_RFC,
    &error_code_out,
    &error_msg_out,
    &error_line_out,
    &error_column_out);

  LOG(ERROR) << "TAGAB SyncDevices::FromJson: records_v.get()="<<records_v.get();
  LOG(ERROR) << "TAGAB SyncDevices::FromJson: error_msg_out="<<error_msg_out;

  DCHECK(records_v);
  if (!records_v) {
    return;
  }
  //LOG(ERROR) << "SyncDevices::FromJson bv_devices: " << brave::debug::ToPrintableString(*records_v);

  devices_.clear();
  const base::Value* pv_arr = records_v->FindKey("devices");
  CHECK(pv_arr->is_list());
  for (const base::Value &val : pv_arr->GetList() ) {
    std::string name = val.FindKey("name")->GetString();
    std::string object_id = val.FindKey("object_id")->GetString();
    std::string device_id = val.FindKey("device_id")->GetString();
    double last_active = 0;
    const base::Value *v_last_active = val.FindKey("last_active");
    if (v_last_active->is_double()) {
      last_active = v_last_active->GetDouble();
    } else {
      LOG(ERROR) << "TAGAB SyncDevices::FromJson: last_active is not double";
    }

    LOG(ERROR) << "TAGAB SyncDevices::FromJson: name="<<name;
    LOG(ERROR) << "TAGAB SyncDevices::FromJson: object_id="<<object_id;
    LOG(ERROR) << "TAGAB SyncDevices::FromJson: device_id="<<device_id;
    LOG(ERROR) << "TAGAB SyncDevices::FromJson: last_active="<<last_active;
    LOG(ERROR) << "TAGAB SyncDevices::FromJson: base::Time::FromJsTime="<<base::Time::FromJsTime(last_active);

    devices_.push_back(SyncDevice(
      name,
      object_id,
      device_id,
      last_active
    ));
  }

}

void SyncDevices::Merge(const SyncDevice &device, int action) {
  LOG(ERROR) << "SyncDevices::Merge device.object_id_==" << device.object_id_ << " action="<<action;
  /*
  const int kActionCreate = 0;
  const int kActionUpdate = 1;
  const int kActionDelete = 2;
  */
  //SyncDevice* existing_device = GetByObjectId(device.object_id_);
  auto existing_it = std::find_if(std::begin(devices_), std::end(devices_), [device](const SyncDevice &cur_dev) {
    return cur_dev.object_id_ == device.object_id_;
  });

  if (existing_it == std::end(devices_)) {
    LOG(ERROR) << "SyncDevices::Merge: existing device not found";
  } else {
    LOG(ERROR) << "SyncDevices::Merge: found existing device name=" << existing_it->name_;
  }

  switch (action) {
    case jslib_const::kActionCreate: {
      //DCHECK(existing_device == nullptr);
      if (existing_it == std::end(devices_)) {
        devices_.push_back(device);
      } else {
        // ignoring create, already have device
      }
      LOG(ERROR) << "SyncDevices::GetByObjectId device created";
      break;
    }
    case jslib_const::kActionUpdate: {
      //DCHECK(existing_device != nullptr);
      DCHECK(existing_it != std::end(devices_));
      //*existing_device = device;
      *existing_it = device;
      LOG(ERROR) << "SyncDevices::GetByObjectId device updated";
      break;
    }
    case jslib_const::kActionDelete: {
      //DCHECK(existing_device != nullptr);
      DCHECK(existing_it != std::end(devices_));
      //DeleteByObjectId(device.object_id_);
      devices_.erase(existing_it);
      LOG(ERROR) << "SyncDevices::GetByObjectId device deleted";
      break;
    }
  }
}

SyncDevice* SyncDevices::GetByObjectId(const std::string &object_id) {
  LOG(ERROR) << "SyncDevices::GetByObjectId object_id="<<object_id;
  for (auto & device: devices_) {
    LOG(ERROR) << "SyncDevices::GetByObjectId device.object_id_="<<device.object_id_;
    if (device.object_id_ == object_id) {
      return &device;
    }
  }

  //DCHECK(false) << "Not expected to find no device";
  LOG(ERROR) << "SyncDevices::GetByObjectId found no devices";
  return nullptr;
}

const SyncDevice* SyncDevices::GetByDeviceId(const std::string &device_id) {
  LOG(ERROR) << "SyncDevices::GetByDeviceId device_id="<<device_id;
  for (const auto & device: devices_) {
    LOG(ERROR) << "SyncDevices::GetByDeviceId device.device_id_="<<device.device_id_;
    if (device.device_id_ == device_id) {
      return &device;
    }
  }

  //DCHECK(false) << "Not expected to find no device";
  LOG(ERROR) << "SyncDevices::GetByDeviceId found no devices";
  return nullptr;
}

void SyncDevices::DeleteByObjectId(const std::string &object_id) {
  LOG(ERROR) << "SyncDevices::DeleteByObjectId object_id="<<object_id;
  auto existing_it = std::find_if(std::begin(devices_), std::end(devices_), [object_id](const SyncDevice &dev) {
    return dev.object_id_ == object_id;
  });

  if (existing_it != std::end(devices_)) {
    devices_.erase(existing_it);
    LOG(ERROR) << "SyncDevices::DeleteByObjectId erased object_id="<<object_id;
  } else {
    DCHECK(false);
    LOG(ERROR) << "SyncDevices::DeleteByObjectId not found object_id="<<object_id;
  }
}

} // namespace brave_sync

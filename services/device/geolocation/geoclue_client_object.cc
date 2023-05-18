// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_client_object.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"

GeoClueClientObject::LocationProperties::LocationProperties(
    dbus::ObjectProxy* proxy)
    : dbus::PropertySet(proxy, kLocationInterfaceName, base::NullCallback()) {
  RegisterProperty("Latitude", &latitude);
  RegisterProperty("Longitude", &longitude);
  RegisterProperty("Accuracy", &accuracy);
  RegisterProperty("Altitude", &altitude);
  RegisterProperty("Speed", &speed);
  RegisterProperty("Heading", &heading);
}

GeoClueClientObject::LocationProperties::~LocationProperties() = default;

void GeoClueClientObject::LocationProperties::GetAll(
    base::OnceCallback<void()> on_got_all) {
  // We only support this one at a time. It's fine for now.
  DCHECK(!on_got_all_);
  on_got_all_ = std::move(on_got_all);
  dbus::PropertySet::GetAll();
}

// dbus::PropertySet:
void GeoClueClientObject::LocationProperties::OnGetAll(
    dbus::Response* response) {
  dbus::PropertySet::OnGetAll(response);

  if (on_got_all_) {
    std::move(on_got_all_).Run();
  }
}

GeoClueClientObject::Properties::Properties(dbus::ObjectProxy* proxy)
    : dbus::PropertySet(proxy,
                        GeoClueClientObject::kInterfaceName,
                        base::NullCallback()) {
  RegisterProperty("DesktopId", &desktop_id);
  RegisterProperty("RequestedAccuracyLevel", &requested_accuracy_level);
}

GeoClueClientObject::Properties::~Properties() = default;

using GetClientCallback =
    base::OnceCallback<void(std::unique_ptr<GeoClueClientObject>)>;
// static
void GeoClueClientObject::GetClient(scoped_refptr<dbus::Bus> bus,
                                    GetClientCallback callback) {
  auto* manager_proxy =
      bus->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));
  dbus::MethodCall get_client(kManagerInterfaceName, "GetClient");
  manager_proxy->CallMethod(
      &get_client, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(
          [](scoped_refptr<dbus::Bus> bus, GetClientCallback callback,
             dbus::Response* response) {
            if (!response) {
              std::move(callback).Run(nullptr);
              return;
            }

            dbus::MessageReader reader(response);
            dbus::ObjectPath client_path;
            if (!reader.PopObjectPath(&client_path)) {
              std::move(callback).Run(nullptr);
              return;
            }

            std::move(callback).Run(std::unique_ptr<GeoClueClientObject>(
                new GeoClueClientObject(bus, client_path)));
          },
          bus, std::move(callback)));
}

GeoClueClientObject::GeoClueClientObject(scoped_refptr<dbus::Bus> bus,
                                         const dbus::ObjectPath& object_path)
    : bus_(bus) {
  proxy_ = bus_->GetObjectProxy(kServiceName, object_path);
  properties_ = std::make_unique<Properties>(proxy_.get());
}

GeoClueClientObject::~GeoClueClientObject() = default;

void GeoClueClientObject::Start(dbus::ObjectProxy::ResponseCallback callback) {
  dbus::MethodCall method(kInterfaceName, "Start");
  proxy_->CallMethod(&method, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                     std::move(callback));
}

void GeoClueClientObject::Stop(dbus::ObjectProxy::ResponseCallback callback) {
  dbus::MethodCall method(kInterfaceName, "Stop");
  proxy_->CallMethod(&method, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                     std::move(callback));
}

using LocationChangedCallback = base::RepeatingCallback<void(
    std::unique_ptr<GeoClueClientObject::LocationProperties>)>;
void GeoClueClientObject::ConnectToLocationUpdatedSignal(
    LocationChangedCallback callback,
    dbus::ObjectProxy::OnConnectedCallback on_connected) {
  proxy_->ConnectToSignal(
      kInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](scoped_refptr<dbus::Bus> bus, LocationChangedCallback callback,
             dbus::Signal* signal) {
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            dbus::ObjectPath new_location;
            if (!reader.PopObjectPath(&old_location) ||
                !reader.PopObjectPath(&new_location)) {
              callback.Run(nullptr);
              return;
            }
            auto properties = std::make_unique<LocationProperties>(
                bus->GetObjectProxy(kServiceName, new_location));
            properties->GetAll(base::BindOnce(callback, std::move(properties)));
          },
          bus_, std::move(callback)),
      std::move(on_connected));
}

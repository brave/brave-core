// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_client_object.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/string_piece_forward.h"
#include "dbus/object_proxy.h"

namespace {}

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

GeoClueClientObject::GeoClueClientObject(
    scoped_refptr<dbus::ObjectProxy> object_proxy)
    : proxy_(object_proxy) {
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

void GeoClueClientObject::ConnectToSignal(
    const std::string& signal,
    dbus::ObjectProxy::SignalCallback signal_callback,
    dbus::ObjectProxy::OnConnectedCallback on_connected) {
      proxy_->ConnectToSignal(kInterfaceName, signal, std::move(signal_callback), std::move(on_connected));
    }

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/time/time.h"
#include "brave/services/device/geolocation/geoclue_client_object.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "services/device/public/cpp/device_features.h"
#include "services/device/public/mojom/geolocation_internals.mojom.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

namespace {

// constexpr char kBraveDesktopId[] = "com.brave.Browser";
// TODO: I think this should be added to /etc/geoclue/geoclue.conf @ install
constexpr char kBraveDesktopId[] = "firefox";

// Note: This method blocks because the call to NewSystemProvider is not
// asynchronous, but it happens on the background geolocation thread.
//
// The easiest way to determine if a DBus service exists is to try and call a
// method on it, and see if it fails. For this, I chose
// |GeoClue2.Manager.GetClient| (this is cached on the GeoClue2 side, so it will
// be the same client we get when we start our service).
bool GeoClueAvailable() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::PRIVATE;
  auto bus = base::MakeRefCounted<dbus::Bus>(options);

  dbus::ObjectProxy* proxy = bus->GetObjectProxy(
      GeoClueClientObject::kServiceName,
      dbus::ObjectPath(GeoClueClientObject::kManagerObjectPath));

  dbus::MethodCall call(GeoClueClientObject::kManagerInterfaceName,
                        "GetClient");
  auto response =
      proxy->CallMethodAndBlock(&call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT);

  // If the response is |nullptr| then the GeoClue2.Manager does not exist.
  bool available = response->get();

  // Shutdown this bus - we'll use one on the DBus thread for our actual
  // provider.
  bus->ShutdownAndBlock();

  return available;
}

}  // namespace

GeoClueLocationProvider::GeoClueLocationProvider() {
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::PRIVATE;
  options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();

  bus_ = base::MakeRefCounted<dbus::Bus>(options);
}

GeoClueLocationProvider::~GeoClueLocationProvider() {
  dbus_thread_linux::GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&dbus::Bus::ShutdownAndBlock, std::move(bus_)));
}

void GeoClueLocationProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  position_update_callback_ = callback;
}

void GeoClueLocationProvider::StartProvider(bool high_accuracy) {
  // The GeoClue2 Client doesn't provide a location with the updated accuracy
  // unless it is restarted, so if the accuracy level has changed we need to
  // restart the provider.
  if (!high_accuracy_requested_ && high_accuracy) {
    client_.reset();
    high_accuracy_requested_ = true;
  }

  if (!client_) {
    GeoClueClientObject::CreateParams params;
    params.bus = bus_;
    params.desktop_id = kBraveDesktopId;
    params.high_accuracy = high_accuracy_requested_;
    params.on_location_changed = base::BindRepeating(
        &GeoClueLocationProvider::OnLocationUpdated, base::Unretained(this));
    params.on_error = base::BindRepeating(&GeoClueLocationProvider::OnError,
                                          base::Unretained(this));
    client_ = std::make_unique<GeoClueClientObject>(std::move(params));
  }

  MaybeStartClient();
}

void GeoClueLocationProvider::StopProvider() {
  // Reset the client.
  client_.reset();
}

const mojom::GeopositionResult* GeoClueLocationProvider::GetPosition() {
  return last_position_.get();
}

void GeoClueLocationProvider::OnPermissionGranted() {
  permission_granted_ = true;
  MaybeStartClient();
}

void GeoClueLocationProvider::FillDiagnostics(
    mojom::GeolocationDiagnostics& diagnostics) {
  if (!client_) {
    diagnostics.provider_state =
        mojom::GeolocationDiagnostics::ProviderState::kStopped;
  } else if (!permission_granted_) {
    diagnostics.provider_state = mojom::GeolocationDiagnostics::ProviderState::
        kBlockedBySystemPermission;
  } else if (high_accuracy_requested_) {
    diagnostics.provider_state =
        mojom::GeolocationDiagnostics::ProviderState::kHighAccuracy;
  } else {
    diagnostics.provider_state =
        mojom::GeolocationDiagnostics::ProviderState::kLowAccuracy;
  }
}

void GeoClueLocationProvider::SetPosition(
    mojom::GeopositionResultPtr position) {
  last_position_ = std::move(position);
  position_update_callback_.Run(this, last_position_.Clone());
}

void GeoClueLocationProvider::MaybeStartClient() {
  if (!client_ || !permission_granted_) {
    return;
  }

  client_->Start();
}

void GeoClueLocationProvider::OnLocationUpdated(
    GeoClueClientObject::LocationProperties* properties) {
  if (!properties) {
    OnError("Failed to read updated location");
    return;
  }

  auto position = mojom::Geoposition::New();
  position->latitude = properties->latitude.value();
  position->longitude = properties->longitude.value();
  position->accuracy = properties->accuracy.value();
  position->altitude = properties->altitude.value();
  position->heading = properties->heading.value();
  position->speed = properties->speed.value();
  position->timestamp = base::Time::Now();

  SetPosition(mojom::GeopositionResult::NewPosition(std::move(position)));
}

void GeoClueLocationProvider::OnError(const std::string& error_message) {
  auto error = mojom::GeopositionError::New();
  error->error_code = mojom::GeopositionErrorCode::kPositionUnavailable;
  error->error_message = error_message;
  SetPosition(mojom::GeopositionResult::NewError(std::move(error)));
}

std::unique_ptr<GeoClueLocationProvider> MaybeCreateGeoClueLocationProvider() {
  if (!base::FeatureList::IsEnabled(features::kLinuxGeoClueLocationBackend)) {
    return nullptr;
  }

  // If GeoClue2 is not available return |nullptr| so we fall back to the
  // network location provider.
  if (!GeoClueAvailable()) {
    return nullptr;
  }

  return std::make_unique<GeoClueLocationProvider>();
}

}  // namespace device

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_location_provider.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/time/time.h"
#include "brave/services/device/geolocation/geoclue_client_object.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "services/device/public/cpp/device_features.h"
#include "services/device/public/cpp/geolocation/geoposition.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

namespace {

constexpr char kBraveDesktopId[] = "com.brave.Browser";

mojom::GeopositionResultPtr GetError() {
  auto error = mojom::GeopositionError::New();
  error->error_code = mojom::GeopositionErrorCode::kPositionUnavailable;
  error->error_message = "Unable to create instance of Geolocation API";
  return mojom::GeopositionResult::NewError(std::move(error));
}

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
  bool available = response.get();

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
      FROM_HERE, base::BindOnce(&dbus::Bus::ShutdownAndBlock, bus_));
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
    if (client_state_ != kStopped) {
      StopProvider();
    }
    high_accuracy_requested_ = true;
  }

  if (client_state_ != kStopped) {
    return;
  }
  client_state_ = kInitializing;

  GeoClueClientObject::GetClient(
      bus_, base::BindOnce(&GeoClueLocationProvider::OnGetClientCompleted,
                           weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::StopProvider() {
  if (client_state_ == kStopped) {
    return;
  }

  client_state_ = kStopped;

  // Invalidate weak pointers, so we don't continue any async operations.
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Stop can be called before the client_ has resolved.
  if (!client_) {
    return;
  }

  client_->Stop();

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

void GeoClueLocationProvider::SetPosition(
    mojom::GeopositionResultPtr position) {
  last_position_ = std::move(position);

  if (client_state_ != kStarted) {
    return;
  }

  if (last_position_->is_position() &&
      device::ValidateGeoposition(*last_position_->get_position())) {
    position_update_callback_.Run(this, last_position_.Clone());
    return;
  }
}

void GeoClueLocationProvider::OnGetClientCompleted(
    std::unique_ptr<GeoClueClientObject> client) {
  if (!client) {
    SetPosition(GetError());
    return;
  }

  client_ = std::move(client);
  SetClientProperties();
}

void GeoClueLocationProvider::SetClientProperties() {
  auto* properties = client_->properties();

  // Wait for all properties to be set before starting the client.
  const auto callback = base::BarrierCallback<bool>(
      2, base::BindOnce(&GeoClueLocationProvider::OnSetClientProperties,
                        weak_ptr_factory_.GetWeakPtr()));

  properties->requested_accuracy_level.Set(
      high_accuracy_requested_ ? GeoClueClientObject::AccuracyLevel::kExact
                               : GeoClueClientObject::AccuracyLevel::kCity,
      callback);

  properties->desktop_id.Set(kBraveDesktopId, callback);
}

void GeoClueLocationProvider::OnSetClientProperties(std::vector<bool> success) {
  if (base::ranges::any_of(success, [](const auto& value) { return !value; })) {
    VLOG(1) << "Failed to set properties. GeoClue2 location provider will "
               "not work properly";
    SetPosition(GetError());
    return;
  }

  ConnectSignal();
}

void GeoClueLocationProvider::ConnectSignal() {
  CHECK(client_);

  client_->ConnectToLocationUpdatedSignal(
      base::BindRepeating(&GeoClueLocationProvider::OnReadGeoClueLocation,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&GeoClueLocationProvider::OnSignalConnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnSignalConnected(
    const std::string& interface_name,
    const std::string& signal_name,
    bool success) {
  if (!success) {
    VLOG(1) << "Failed to connect to LocationUpdated Signal. GeoClue2 "
               "location provider will "
               "not work properly";
    SetPosition(GetError());
    return;
  }

  client_state_ = kInitialized;
  MaybeStartClient();
}

void GeoClueLocationProvider::MaybeStartClient() {
  if (!client_ || !permission_granted_ || client_state_ != kInitialized) {
    return;
  }

  client_state_ = kStarting;

  client_->Start(base::BindOnce(&GeoClueLocationProvider::OnClientStarted,
                                weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnClientStarted(dbus::Response* response) {
  client_state_ = kStarted;
}

void GeoClueLocationProvider::OnReadGeoClueLocation(
    std::unique_ptr<GeoClueClientObject::LocationProperties> properties) {
  if (!properties) {
    SetPosition(GetError());
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

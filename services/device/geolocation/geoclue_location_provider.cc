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
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "dbus/property.h"
#include "services/device/public/cpp/device_features.h"
#include "services/device/public/cpp/geolocation/geoposition.h"

namespace device {

namespace {

constexpr char kServiceName[] = "org.freedesktop.GeoClue2";
constexpr char kLocationInterfaceName[] = "org.freedesktop.GeoClue2.Location";
constexpr char kClientInterfaceName[] = "org.freedesktop.GeoClue2.Client";
constexpr char kManagerInterfaceName[] = "org.freedesktop.GeoClue2.Manager";
constexpr char kManagerObjectPath[] = "/org/freedesktop/GeoClue2/Manager";
constexpr char kBraveDesktopId[] = "com.brave.Browser";

// This corresponds to the GeoClue2 AccuracyLevel: Neighborhood
constexpr uint32_t kGeoClueAccuracyLow = 5;

// This corresponds to the GeoClue2 AccuracyLevel: Exact
constexpr uint32_t kGeoClueAccuracyHigh = 8;

mojom::Geoposition GetErrorPosition() {
  mojom::Geoposition response;
  response.error_code = mojom::Geoposition_ErrorCode::POSITION_UNAVAILABLE;
  response.error_message = "Unable to create instance of Geolocation API";
  return response;
}

// Note: This method blocks because the call to NewSystemProvider is not
// asynchronous, but it happens on the background geolocation thread.
//
// The easiest way to determine if a DBus service exists is to try and call a
// method on it, and see if it fails. For this, I chose
// |GeoClue2.Manager.GetClient| (this is cached on the GeoClue2 side, so it will
// be the same client we get when we start our service).
bool GeoClueAvailable() {
  dbus::Bus::Options options;
  options.bus_type = dbus::Bus::SYSTEM;
  options.connection_type = dbus::Bus::PRIVATE;
  auto bus = base::MakeRefCounted<dbus::Bus>(options);

  dbus::ObjectProxy* proxy =
      bus->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));

  dbus::MethodCall call(kManagerInterfaceName, "GetClient");
  auto response =
      proxy->CallMethodAndBlock(&call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT);

  // If the response is |nullptr| then the GeoClue2.Manager does not exist.
  bool available = response.get();

  // Shutdown this bus - we'll use one on the DBus thread for our actual
  // provider.
  bus->ShutdownAndBlock();

  return available;
}

struct GeoClueProperties : public dbus::PropertySet {
  dbus::Property<std::string> desktop_id;
  dbus::Property<uint32_t> requested_accuracy_level;

  explicit GeoClueProperties(dbus::ObjectProxy* proxy)
      : dbus::PropertySet(proxy, kClientInterfaceName, base::NullCallback()) {
    RegisterProperty("DesktopId", &desktop_id);
    RegisterProperty("RequestedAccuracyLevel", &requested_accuracy_level);
  }

  ~GeoClueProperties() override = default;
};

}  // namespace

struct GeoClueLocationProperties : public dbus::PropertySet {
  dbus::Property<double> latitude;
  dbus::Property<double> longitude;
  dbus::Property<double> accuracy;
  dbus::Property<double> altitude;
  dbus::Property<double> speed;
  dbus::Property<double> heading;

  explicit GeoClueLocationProperties(dbus::ObjectProxy* proxy)
      : dbus::PropertySet(proxy, kLocationInterfaceName, base::NullCallback()) {
    RegisterProperty("Latitude", &latitude);
    RegisterProperty("Longitude", &longitude);
    RegisterProperty("Accuracy", &accuracy);
    RegisterProperty("Altitude", &altitude);
    RegisterProperty("Speed", &speed);
    RegisterProperty("Heading", &heading);
  }

  ~GeoClueLocationProperties() override = default;

  using dbus::PropertySet::GetAll;
  void GetAll(base::OnceCallback<void()> on_got_all) {
    // We only support this one at a time. It's fine for now.
    DCHECK(!on_got_all_);
    on_got_all_ = std::move(on_got_all);
    dbus::PropertySet::GetAll();
  }

  // dbus::PropertySet:
  void OnGetAll(dbus::Response* response) override {
    dbus::PropertySet::OnGetAll(response);

    if (on_got_all_) {
      std::move(on_got_all_).Run();
    }
  }

 private:
  base::OnceCallback<void()> on_got_all_;
};

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

  dbus::ObjectProxy* proxy =
      bus_->GetObjectProxy(kServiceName, dbus::ObjectPath(kManagerObjectPath));

  dbus::MethodCall call(kManagerInterfaceName, "GetClient");
  proxy->CallMethod(
      &call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&GeoClueLocationProvider::OnGetClientCompleted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::StopProvider() {
  if (client_state_ == kStopped) {
    return;
  }

  client_state_ = kStopped;

  // Invalidate weak pointers, so we don't continue any async operations.
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Stop can be called before the gclue_client_ has resolved.
  if (!gclue_client_) {
    return;
  }

  dbus::MethodCall stop(kClientInterfaceName, "Stop");
  gclue_client_->CallMethod(&stop, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::DoNothing());

  // Reset pointers to dbus objects.
  gclue_client_.reset();
}

const mojom::Geoposition& GeoClueLocationProvider::GetPosition() {
  return last_position_;
}

void GeoClueLocationProvider::OnPermissionGranted() {
  permission_granted_ = true;
  MaybeStartClient();
}

void GeoClueLocationProvider::SetPosition(const mojom::Geoposition& position) {
  last_position_ = position;

  if (client_state_ != kStarted) {
    return;
  }

  if (last_position_.error_code == mojom::Geoposition_ErrorCode::NONE &&
      !device::ValidateGeoposition(last_position_)) {
    return;
  }
  position_update_callback_.Run(this, last_position_);
}

void GeoClueLocationProvider::OnGetClientCompleted(dbus::Response* response) {
  if (!response) {
    SetPosition(GetErrorPosition());
    return;
  }

  dbus::MessageReader reader(response);
  dbus::ObjectPath path;
  if (!reader.PopObjectPath(&path)) {
    SetPosition(GetErrorPosition());
    return;
  }

  gclue_client_ = bus_->GetObjectProxy(kServiceName, path);
  SetClientProperties();
}

void GeoClueLocationProvider::SetClientProperties() {
  auto properties = std::make_unique<GeoClueProperties>(gclue_client_.get());
  auto* properties_raw = properties.get();

  // Wait for all properties to be set before starting the client.
  const auto callback = base::BarrierCallback<bool>(
      2, base::BindOnce(&GeoClueLocationProvider::OnSetClientProperties,
                        weak_ptr_factory_.GetWeakPtr(), std::move(properties)));

  properties_raw->requested_accuracy_level.Set(
      high_accuracy_requested_ ? kGeoClueAccuracyHigh : kGeoClueAccuracyLow,
      callback);

  properties_raw->desktop_id.Set(kBraveDesktopId, callback);
}

void GeoClueLocationProvider::OnSetClientProperties(
    std::unique_ptr<dbus::PropertySet> property_set,
    std::vector<bool> success) {
  if (base::ranges::any_of(success, [](const auto& value) { return !value; })) {
    VLOG(1) << "Failed to set properties. GeoClue2 location provider will "
               "not work properly";
    SetPosition(GetErrorPosition());
    return;
  }

  ConnectSignal();
}

void GeoClueLocationProvider::ConnectSignal() {
  CHECK(gclue_client_);

  gclue_client_->ConnectToSignal(
      kClientInterfaceName, "LocationUpdated",
      base::BindRepeating(
          [](base::WeakPtr<GeoClueLocationProvider> provider,
             dbus::Signal* signal) {
            dbus::MessageReader reader(signal);
            dbus::ObjectPath old_location;
            dbus::ObjectPath new_location;
            if (!reader.PopObjectPath(&old_location) ||
                !reader.PopObjectPath(&new_location)) {
              if (provider) {
                provider->SetPosition(GetErrorPosition());
              }
              return;
            }

            if (provider) {
              provider->ReadGeoClueLocation(new_location);
            }
          },
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
    SetPosition(GetErrorPosition());
    return;
  }

  client_state_ = kInitialized;
  MaybeStartClient();
}

void GeoClueLocationProvider::MaybeStartClient() {
  if (!gclue_client_ || !permission_granted_ || client_state_ != kInitialized) {
    return;
  }

  client_state_ = kStarting;

  dbus::MethodCall start(kClientInterfaceName, "Start");
  gclue_client_->CallMethod(
      &start, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::BindOnce(&GeoClueLocationProvider::OnClientStarted,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueLocationProvider::OnClientStarted(dbus::Response* response) {
  client_state_ = kStarted;
}

void GeoClueLocationProvider::ReadGeoClueLocation(
    const dbus::ObjectPath& location_path) {
  auto location_properties = std::make_unique<GeoClueLocationProperties>(
      bus_->GetObjectProxy(kServiceName, location_path));
  location_properties->GetAll(base::BindOnce(
      &GeoClueLocationProvider::OnReadGeoClueLocation,
      weak_ptr_factory_.GetWeakPtr(), std::move(location_properties)));
}

void GeoClueLocationProvider::OnReadGeoClueLocation(
    std::unique_ptr<GeoClueLocationProperties> properties) {
  mojom::Geoposition position;
  position.latitude = properties->latitude.value();
  position.longitude = properties->longitude.value();
  position.accuracy = properties->accuracy.value();
  position.altitude = properties->altitude.value();
  position.heading = properties->heading.value();
  position.speed = properties->speed.value();
  position.error_code = mojom::Geoposition::ErrorCode::NONE;
  position.timestamp = base::Time::Now();
  SetPosition(position);
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

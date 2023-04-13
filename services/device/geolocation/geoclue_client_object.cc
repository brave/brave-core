// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/services/device/geolocation/geoclue_client_object.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "base/ranges/algorithm.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"

namespace device {
namespace {

enum class AccuracyLevel : uint32_t {
  kNone = 0,
  kCountry = 1,
  kCity = 4,
  kNeighborhood = 5,
  kStreet = 6,
  kExact = 8,
};

}  // namespace

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

GeoClueClientObject::CreateParams::CreateParams() = default;
GeoClueClientObject::CreateParams::~CreateParams() = default;

GeoClueClientObject::CreateParams::CreateParams(
    const GeoClueClientObject::CreateParams&) = default;

GeoClueClientObject::CreateParams& GeoClueClientObject::CreateParams::operator=(
    const GeoClueClientObject::CreateParams&) = default;

GeoClueClientObject::CreateParams::CreateParams(
    GeoClueClientObject::CreateParams&&) noexcept = default;

GeoClueClientObject::CreateParams& GeoClueClientObject::CreateParams::operator=(
    GeoClueClientObject::CreateParams&&) noexcept = default;

GeoClueClientObject::GeoClueClientObject(CreateParams params)
    : creation_params_(params) {
  GetClient();
}

GeoClueClientObject::~GeoClueClientObject() {
  // If we have a proxy, call Stop on it. In the worst case (we weren't
  // initialized) this is just a noop but otherwise this let's GeoClue2 know we
  // aren't listening to the location any more.
  // Note: Even though our reference to |proxy_| will be deleted here it's safe
  // to call a method on it because it will be kept alive by the Bus.
  if (proxy_) {
    dbus::MethodCall stop(kInterfaceName, "Stop");
    proxy_->CallMethod(&stop, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                       base::DoNothing());
  }
}

void GeoClueClientObject::NotifyError(const std::string& error_message) {
  weak_ptr_factory_.InvalidateWeakPtrs();

  state_ = State::kError;
  creation_params_.on_error.Run(error_message);
}

void GeoClueClientObject::NotifyLocationChanged() {
  creation_params_.on_location_changed.Run(location_.get());
}

void GeoClueClientObject::GetClient() {
  CHECK_EQ(state_, State::kInitializing);

  auto* manager_proxy = creation_params_.bus->GetObjectProxy(
      kServiceName, dbus::ObjectPath(kManagerObjectPath));
  dbus::MethodCall get_client(kManagerInterfaceName, "GetClient");
  manager_proxy->CallMethod(&get_client, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                            base::BindOnce(&GeoClueClientObject::OnGotClient,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueClientObject::OnGotClient(dbus::Response* response) {
  if (!response) {
    NotifyError("Failed to get a GeoClue2.Client object");
    return;
  }

  dbus::MessageReader reader(response);
  dbus::ObjectPath client_path;
  if (!reader.PopObjectPath(&client_path)) {
    NotifyError("Failed to read object path for GeoClue2.Client");
    return;
  }

  proxy_ = creation_params_.bus->GetObjectProxy(kServiceName,
                                                dbus::ObjectPath(client_path));
  SetProperties();
}

void GeoClueClientObject::SetProperties() {
  CHECK(proxy_);

  properties_ = std::make_unique<Properties>(proxy_.get());

  const auto callback = base::BarrierCallback<bool>(
      2, base::BindOnce(&GeoClueClientObject::OnSetProperties,
                        weak_ptr_factory_.GetWeakPtr()));

  properties_->requested_accuracy_level.Set(
      static_cast<uint32_t>(creation_params_.high_accuracy
                                ? AccuracyLevel::kExact
                                : AccuracyLevel::kCity),
      callback);
  properties_->desktop_id.Set(creation_params_.desktop_id, callback);
}

void GeoClueClientObject::OnSetProperties(std::vector<bool> results) {
  CHECK(properties_);

  if (base::ranges::any_of(results, [](const auto& value) { return !value; })) {
    NotifyError("Failed to set desktop_id and accuracy level.");
    return;
  }

  ConnectLocationUpdated();
}

void GeoClueClientObject::ConnectLocationUpdated() {
  CHECK(proxy_);

  proxy_->ConnectToSignal(
      kInterfaceName, "LocationUpdated",
      base::BindRepeating(&GeoClueClientObject::OnLocationUpdated,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&GeoClueClientObject::OnLocationUpdatedConnected,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueClientObject::OnLocationUpdatedConnected(
    const std::string& service,
    const std::string& interface,
    bool success) {
  if (!success) {
    NotifyError("Failed to connect to LocationUpdate signal");
    return;
  }

  state_ = State::kInitialized;
  MaybeStartClient();
}

void GeoClueClientObject::OnLocationUpdated(dbus::Signal* signal) {
  dbus::MessageReader reader(signal);
  dbus::ObjectPath old_location;
  dbus::ObjectPath new_location;
  if (!reader.PopObjectPath(&old_location) ||
      !reader.PopObjectPath(&new_location)) {
    location_.reset();
    NotifyLocationChanged();
    return;
  }

  location_ = std::make_unique<LocationProperties>(
      creation_params_.bus->GetObjectProxy(kServiceName, new_location));
  location_->GetAll(base::BindOnce(&GeoClueClientObject::NotifyLocationChanged,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueClientObject::MaybeStartClient() {
  if (!should_start_ || state_ != State::kInitialized) {
    return;
  }

  state_ = State::kStarting;

  dbus::MethodCall method(kInterfaceName, "Start");
  proxy_->CallMethod(&method, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                     base::BindOnce(&GeoClueClientObject::OnStartedClient,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void GeoClueClientObject::OnStartedClient(dbus::Response* response) {
  if (!response) {
    NotifyError("Failed to start GeoClue2.Client");
    return;
  }

  state_ = State::kStarted;
}

void GeoClueClientObject::Start() {
  should_start_ = true;
  MaybeStartClient();
}

}  // namespace device

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "brave/services/device/geolocation/geoclue_client_object.h"
#include "dbus/bus.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

class COMPONENT_EXPORT(BRAVE_GEOLOCATION) GeoClueLocationProvider
    : public LocationProvider {
 public:
  GeoClueLocationProvider();

  GeoClueLocationProvider(const GeoClueLocationProvider&) = delete;
  GeoClueLocationProvider& operator=(const GeoClueLocationProvider&) = delete;

  ~GeoClueLocationProvider() override;

  // LocationProvider:
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const mojom::GeopositionResult* GetPosition() override;
  void OnPermissionGranted() override;

 protected:
  // Stores whether or not permission has been granted.
  bool permission_granted_ = false;
  bool high_accuracy_requested_ = false;

  void SetPosition(mojom::GeopositionResultPtr position);

 private:
  friend class TestGeoClueLocationProvider;

  // Starts the client when both:
  // 1. Permission has been granted.
  // 2. Start has been called.
  void MaybeStartClient();

  void OnLocationUpdated(GeoClueClientObject::LocationProperties* properties);

  void OnError(const std::string& error);

  scoped_refptr<dbus::Bus> bus_;
  std::unique_ptr<GeoClueClientObject> client_;

  mojom::GeopositionResultPtr last_position_;
  LocationProviderUpdateCallback position_update_callback_;
};

COMPONENT_EXPORT(BRAVE_GEOLOCATION)
std::unique_ptr<GeoClueLocationProvider> MaybeCreateGeoClueLocationProvider();

}  // namespace device

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_

#include <memory>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_refptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"

class GeoClueClientObject {
 public:
  static constexpr char kServiceName[] = "org.freedesktop.GeoClue2";
  static constexpr char kManagerInterfaceName[] =
      "org.freedesktop.GeoClue2.Manager";
  static constexpr char kManagerObjectPath[] =
      "/org/freedesktop/GeoClue2/Manager";
  static constexpr char kInterfaceName[] = "org.freedesktop.GeoClue2.Client";
  static constexpr char kLocationInterfaceName[] =
      "org.freedesktop.GeoClue2.Location";

  enum AccuracyLevel : uint32_t {
    kNone = 0,
    kCountry = 1,
    kCity = 4,
    kNeighborhood = 5,
    kStreet = 6,
    kExact = 8,
  };

  struct LocationProperties : public dbus::PropertySet {
    dbus::Property<double> latitude;
    dbus::Property<double> longitude;
    dbus::Property<double> accuracy;
    dbus::Property<double> altitude;
    dbus::Property<double> speed;
    dbus::Property<double> heading;

    explicit LocationProperties(dbus::ObjectProxy* proxy);
    ~LocationProperties() override;

    using dbus::PropertySet::GetAll;
    void GetAll(base::OnceCallback<void()> on_got_all);

    // dbus::PropertySet:
    void OnGetAll(dbus::Response* response) override;

   private:
    base::OnceCallback<void()> on_got_all_;
  };

  struct Properties : public dbus::PropertySet {
    dbus::Property<std::string> desktop_id;
    dbus::Property<uint32_t> requested_accuracy_level;

    explicit Properties(dbus::ObjectProxy* proxy);
    ~Properties() override;
  };

  static void GetClient(
      scoped_refptr<dbus::Bus> bus,
      base::OnceCallback<void(std::unique_ptr<GeoClueClientObject>)> callback);

  GeoClueClientObject(const GeoClueClientObject&) = delete;
  GeoClueClientObject& operator=(const GeoClueClientObject&) = delete;

  ~GeoClueClientObject();

  void Start(dbus::ObjectProxy::ResponseCallback callback = base::DoNothing());
  void Stop(dbus::ObjectProxy::ResponseCallback callback = base::DoNothing());

  void ConnectToLocationUpdatedSignal(
      base::RepeatingCallback<
          void(std::unique_ptr<LocationProperties> location)> callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected);

  Properties* properties() { return properties_.get(); }

 private:
  GeoClueClientObject(scoped_refptr<dbus::Bus> bus,
                      const dbus::ObjectPath& object_path);

  scoped_refptr<dbus::Bus> bus_;
  scoped_refptr<dbus::ObjectProxy> proxy_;
  std::unique_ptr<Properties> properties_;
};

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_

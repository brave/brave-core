// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"

namespace device {

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

  using LocationChangedCallback =
      base::RepeatingCallback<void(LocationProperties*)>;
  using ErrorCallback = base::RepeatingCallback<void(const std::string&)>;

  struct CreateParams {
    CreateParams();
    ~CreateParams();
    CreateParams(const CreateParams&);
    CreateParams& operator=(const CreateParams&);
    CreateParams(CreateParams&&) noexcept;
    CreateParams& operator=(CreateParams&& other) noexcept;

    scoped_refptr<dbus::Bus> bus;
    std::string desktop_id;
    bool high_accuracy;
    LocationChangedCallback on_location_changed;
    ErrorCallback on_error;
  };

  explicit GeoClueClientObject(CreateParams create_params);
  GeoClueClientObject(const GeoClueClientObject&) = delete;
  GeoClueClientObject& operator=(const GeoClueClientObject&) = delete;

  ~GeoClueClientObject();

  void Start();

 private:
  friend class TestGeoClueLocationProvider;

  enum class State { kInitializing, kInitialized, kStarting, kStarted, kError };

  // First the error callback from |CreateParams|, sets the state of the
  // GeoClueClientObject to |kError| and invalidates any weak pointers, stopping
  // any initialization logic and preventing signals from firing.
  void NotifyError(const std::string& error_message);

  // Fires the |on_location_changed| event from the CreateParams
  void NotifyLocationChanged();

  // Step 1: Get the client from the GeoClue2.Manager
  void GetClient();
  void OnGotClient(dbus::Response* response);

  // Step 2: Set the DesktopId & Accuracy
  void SetProperties();
  void OnSetProperties(std::vector<bool> results);

  // Step 3: Connect the LocationUpdate signal, so we're notified of Location
  // changes.
  void ConnectLocationUpdated();
  void OnLocationUpdatedConnected(const std::string& service,
                                  const std::string& interface,
                                  bool success);

  // Called every time the LocationUpdate signal is fired. Note: This signal is
  // also fired when the client is started.
  void OnLocationUpdated(dbus::Signal* signal);

  // MaybeStartClient starts the GeoClue2 client when the Client has finished
  // initializing **AND** GeoClueClientObject::Start has been called.
  void MaybeStartClient();
  void OnStartedClient(dbus::Response* response);

  CreateParams creation_params_;

  State state_ = State::kInitializing;
  bool should_start_ = false;

  scoped_refptr<dbus::ObjectProxy> proxy_;
  std::unique_ptr<Properties> properties_;
  std::unique_ptr<LocationProperties> location_;

  base::WeakPtrFactory<GeoClueClientObject> weak_ptr_factory_{this};
};

}  // namespace device

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_CLIENT_OBJECT_H_

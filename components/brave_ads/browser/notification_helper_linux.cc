/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_linux.h"

#include <memory>
#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "chrome/browser/fullscreen.h"
#include "chrome/common/chrome_features.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace brave_ads {

namespace {

// DBus name
const char kFreedesktopNotificationsName[] = "org.freedesktop.Notifications";

// DBus methods
const char kMethodListActivatableNames[] = "ListActivatableNames";
const char kMethodNameHasOwner[] = "NameHasOwner";

}  // namespace

NotificationHelperLinux::NotificationHelperLinux() = default;

NotificationHelperLinux::~NotificationHelperLinux() = default;

bool NotificationHelperLinux::ShouldShowNotifications() {
  if (IsFullScreenMode()) {
    LOG(WARNING) << "Notification not made: Full screen mode";
    return false;
  }

  if (!base::FeatureList::IsEnabled(features::kNativeNotifications)) {
    LOG(WARNING) << "Native notification feature is disabled so falling back to"
        " Message Center";
    return true;
  }

  if (!IsNotificationsEnabled()) {
    LOG(INFO) << "Notification not made: Notifications are disabled";
    return false;
  }

  return true;
}

bool NotificationHelperLinux::ShowMyFirstAdNotification() {
  return false;
}

bool NotificationHelperLinux::CanShowBackgroundNotifications() const {
  return true;
}

NotificationHelperLinux* NotificationHelperLinux::GetInstanceImpl() {
  return base::Singleton<NotificationHelperLinux>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperLinux::GetInstanceImpl();
}

///////////////////////////////////////////////////////////////////////////////

bool NotificationHelperLinux::IsNotificationsEnabled() {
  dbus::Bus::Options bus_options;
  bus_options.bus_type = dbus::Bus::SESSION;
  bus_options.connection_type = dbus::Bus::PRIVATE;
  bus_options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();

  scoped_refptr<dbus::Bus> bus = base::MakeRefCounted<dbus::Bus>(bus_options);

  dbus::ObjectProxy* dbus_proxy =
      bus->GetObjectProxy(DBUS_SERVICE_DBUS, dbus::ObjectPath(DBUS_PATH_DBUS));

  dbus::MethodCall name_has_owner_call(DBUS_INTERFACE_DBUS,
                                       kMethodNameHasOwner);
  dbus::MessageWriter writer(&name_has_owner_call);

  writer.AppendString(kFreedesktopNotificationsName);

  std::unique_ptr<dbus::Response> name_has_owner_response =
      dbus_proxy->CallMethodAndBlock(&name_has_owner_call,
          dbus::ObjectProxy::TIMEOUT_USE_DEFAULT);
  dbus::MessageReader name_has_owner_response_reader(
      name_has_owner_response.get());

  bool owned = false;
  if (name_has_owner_response &&
      name_has_owner_response_reader.PopBool(&owned) &&
      owned) {
    return true;
  }

  // If the service currently isn't running, maybe it is activatable
  dbus::MethodCall list_activatable_names_call(DBUS_INTERFACE_DBUS,
                                               kMethodListActivatableNames);

  std::unique_ptr<dbus::Response> list_activatable_names_response =
      dbus_proxy->CallMethodAndBlock(&list_activatable_names_call,
          dbus::ObjectProxy::TIMEOUT_USE_DEFAULT);

  if (!list_activatable_names_response) {
    return false;
  }

  dbus::MessageReader list_activatable_names_response_reader(
      list_activatable_names_response.get());
  std::vector<std::string> activatable_names;
  list_activatable_names_response_reader.PopArrayOfStrings(&activatable_names);

  return base::Contains(activatable_names, kFreedesktopNotificationsName);
}

}  // namespace brave_ads

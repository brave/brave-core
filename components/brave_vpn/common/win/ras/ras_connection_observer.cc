/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/win/ras/ras_connection_observer.h"

#include <windows.h>  // must be before ras.h

#include <ras.h>

namespace brave_vpn {

namespace ras {

RasConnectionObserver::RasConnectionObserver() = default;

RasConnectionObserver::~RasConnectionObserver() = default;

void RasConnectionObserver::OnObjectSignaled(HANDLE object) {
  // Check connection state for BraveVPN entry again when connected or
  // disconnected events are arrived because we can get both event from any os
  // vpn entry. All other events are sent by our code at utils_win.cc.
  if (object != event_handle_for_connected_disconnected_.get()) {
    return;
  }

  OnRasConnectionStateChanged();
}

void RasConnectionObserver::StartRasConnectionChangeMonitoring() {
  DCHECK(!event_handle_for_connected_disconnected_.get());

  event_handle_for_connected_disconnected_.Set(
      CreateEvent(NULL, false, false, NULL));

  // Ase we pass INVALID_HANDLE_VALUE, we can get connected or disconnected
  // event from any os vpn entry. It's filtered by OnObjectSignaled().
  RasConnectionNotificationW(static_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                             event_handle_for_connected_disconnected_.get(),
                             RASCN_Connection | RASCN_Disconnection);
  connected_disconnected_event_watcher_.StartWatchingMultipleTimes(
      event_handle_for_connected_disconnected_.get(), this);
}

void RasConnectionObserver::StopRasConnectionChangeMonitoring() {
  connected_disconnected_event_watcher_.StopWatching();
  event_handle_for_connected_disconnected_.Close();
}

bool RasConnectionObserver::IsRasConnectionObserverActive() const {
  return connected_disconnected_event_watcher_.IsWatching() &&
         event_handle_for_connected_disconnected_.IsValid();
}

}  // namespace ras
}  // namespace brave_vpn

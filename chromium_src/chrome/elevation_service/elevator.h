/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_ELEVATION_SERVICE_ELEVATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_ELEVATION_SERVICE_ELEVATOR_H_

#include "chrome/elevation_service/elevation_service_idl.h"

#define IElevatorChromeDev IElevatorChromeDev, IElevatorDevelopment
#define DecryptData              \
  InstallVPNServices() override; \
  IFACEMETHODIMP DecryptData

#include "src/chrome/elevation_service/elevator.h"  // IWYU pragma: export

#undef DecryptData
#undef IElevatorChromeDev

#endif  // BRAVE_CHROMIUM_SRC_CHROME_ELEVATION_SERVICE_ELEVATOR_H_

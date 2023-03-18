/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/safe_browsing/client_side_detection_service_factory.h"

#define ClientSideDetectionServiceFactory \
  ClientSideDetectionServiceFactory_ChromiumImpl
#include "src/chrome/browser/safe_browsing/client_side_detection_service_factory.cc"
#undef ClientSideDetectionServiceFactory

namespace safe_browsing {

// static
ClientSideDetectionService* ClientSideDetectionServiceFactory::GetForProfile(
    Profile* profile) {
  return nullptr;
}

// static
ClientSideDetectionServiceFactory*
ClientSideDetectionServiceFactory::GetInstance() {
  return nullptr;
}

}  // namespace safe_browsing

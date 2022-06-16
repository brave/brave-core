/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_URL_LOADER_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_URL_LOADER_H_

#define SetEnableReportingRawHeaders                      \
  SetEnableReportingRawHeaders_ChromiumImpl(bool enable); \
  void SetEnableReportingRawHeaders

#include "src/services/network/url_loader.h"

#undef SetEnableReportingRawHeaders

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_URL_LOADER_H_

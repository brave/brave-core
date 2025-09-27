/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/devtools/gdp_service_handler.h"

// Override to disable GDP service handler requests as it sends them to
// googleapis.com.
#define CanMakeRequest CanMakeRequest_Unused
#include <chrome/browser/devtools/gdp_service_handler.cc>
#undef CanMakeRequest

void GdpServiceHandler::CanMakeRequest(
    Profile* profile,
    base::OnceCallback<void(bool success)> callback) {
  std::move(callback).Run(false);
}

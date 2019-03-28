/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_CONTENT_PACKAGED_SERVICE_OVERLAY_MANIFEST_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_CONTENT_PACKAGED_SERVICE_OVERLAY_MANIFEST_H_

#include <vector>

#include "services/service_manager/public/cpp/manifest.h"

const service_manager::Manifest&
GetBraveContentPackagedServiceOverlayManifest();

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_CONTENT_PACKAGED_SERVICE_OVERLAY_MANIFEST_H_

/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UTILITY_TOR_PUBLIC_CPP_MANIFEST_H_
#define BRAVE_UTILITY_TOR_PUBLIC_CPP_MANIFEST_H_

#include "services/service_manager/public/cpp/manifest.h"

namespace tor {

const service_manager::Manifest& GetTorLauncherManifest();

}  // namespace tor

#endif  // BRAVE_UTILITY_TOR_PUBLIC_CPP_MANIFEST_H_

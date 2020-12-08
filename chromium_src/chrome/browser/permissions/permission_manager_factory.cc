/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/geolocation/brave_geolocation_permission_context_delegate.h"

#define GeolocationPermissionContextDelegate \
  BraveGeolocationPermissionContextDelegate
#include "../../../../../chrome/browser/permissions/permission_manager_factory.cc"
#undef GeolocationPermissionContextDelegate

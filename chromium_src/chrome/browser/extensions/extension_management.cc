/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#define BRAVE_EXTENSION_MANAGEMENT_FACTORY_BUILD_SERVICE_INSTANCE_FOR \
  return new BraveExtensionManagement(Profile::FromBrowserContext(context));

#include "../../../../../chrome/browser/extensions/extension_management.cc"
#undef BRAVE_EXTENSION_MANAGEMENT_FACTORY_BUILD_SERVICE_INSTANCE_FOR

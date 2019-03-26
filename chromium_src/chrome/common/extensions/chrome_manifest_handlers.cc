/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/extensions/manifest_handlers/brave_manifest_override.h"

#define ExtensionActionHandler BraveOverridesHandler
#include "../../../../../chrome/common/extensions/chrome_manifest_handlers.cc"  // NOLINT
#undef ExtensionActionHandler


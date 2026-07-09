/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/manifest_v2_handler.h"

// This override is used to allow using MV2 extensions.
#define BRAVE_SHOULD_DISABLE_LEGACY_EXTENSIONS return false;

#include <extensions/browser/manifest_v2_handler.cc>
#undef BRAVE_SHOULD_DISABLE_LEGACY_EXTENSIONS

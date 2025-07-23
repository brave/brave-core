/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/contains.h"
#include "brave/browser/extensions/manifest_v2/brave_hosted_extensions.h"
#include "extensions/common/extension.h"

#define Contains(...)      \
  Contains(__VA_ARGS__) || \
      extensions_mv2::IsKnownBraveHostedExtension(extension_id)

#include "src/chrome/browser/extensions/mv2_deprecation_impact_checker.cc"

#undef Contains

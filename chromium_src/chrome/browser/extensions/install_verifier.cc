/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/extensions/extension_constants.h"

bool IsMediaRouterExtension(const std::string& id) {
  return id == extension_misc::kMediaRouterStableExtensionId;
}

#define RedoSignatureCheckingForMediaRouter(id) \
    if (IsMediaRouterExtension(id)) \
        AddMany(ids, ADD_ALL_BOOTSTRAP); \
        return;
#include "../../../../../chrome/browser/extensions/install_verifier.cc"  // NOLINT
#undef RedoSignatureCheckingForMediaRouter

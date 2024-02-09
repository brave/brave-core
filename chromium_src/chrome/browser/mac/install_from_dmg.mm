/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/keystone_glue.h"

#import "chrome/browser/mac/dock.h"

#define AddIcon                                                          \
  ChromeIsInTheDock();                                                   \
  KeystoneGlue* keystone_glue = [KeystoneGlue defaultKeystoneGlue];      \
  [keystone_glue setAppPath:target_path];                                \
  [keystone_glue promoteTicketWithAuthorization:std::move(authorization) \
                                    synchronous:YES];                    \
  dock::AddIcon

#include "src/chrome/browser/mac/install_from_dmg.mm"

#undef AddIcon

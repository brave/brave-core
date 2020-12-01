/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_DEVELOPER_PRIVATE_UPDATE_EXTENSION_CONFIGURATION_FUNCTION_RUN \
  if (update.tor_access) {                                                  \
    util::SetIsTorEnabled(extension->id(), browser_context(),               \
                          *update.tor_access);                              \
  }
#include "../../../../../../../chrome/browser/extensions/api/developer_private/developer_private_api.cc"
#undef BRAVE_DEVELOPER_PRIVATE_UPDATE_EXTENSION_CONFIGURATION_FUNCTION_RUN

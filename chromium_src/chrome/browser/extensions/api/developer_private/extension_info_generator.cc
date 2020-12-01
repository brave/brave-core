/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_CREATE_EXTENSION_INFO_HELPER                        \
  info->tor_access.is_enabled = info->incognito_access.is_active; \
  info->tor_access.is_active =                                    \
      util::IsTorEnabled(extension.id(), browser_context_);
#include "../../../../../../../chrome/browser/extensions/api/developer_private/extension_info_generator.cc"
#undef BRAVE_CREATE_EXTENSION_INFO_HELPER

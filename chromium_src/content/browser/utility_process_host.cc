/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_UTILITYPROCESSHOST_STARTPROCESS                              \
  const bool is_service = service_identity_.has_value();                   \
  if (is_service) {                                                        \
    GetContentClient()->browser()->AdjustUtilityServiceProcessCommandLine( \
        *service_identity_, cmd_line.get());                               \
  }

#include "../../../content/browser/utility_process_host.cc"
#undef BRAVE_UTILITYPROCESSHOST_STARTPROCESS

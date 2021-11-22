/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_NAVIGATOR_PLUGINS_SHOULD_RETURN_FIXED_PLUGIN_DATA \
  return false;                                                 \
  if (false)

#include "src/third_party/blink/renderer/modules/plugins/navigator_plugins.cc"

#undef BRAVE_NAVIGATOR_PLUGINS_SHOULD_RETURN_FIXED_PLUGIN_DATA
#undef BRAVE_NAVIGATOR_PLUGINS_SHOULD_RETURN_FIXED_PLUGIN_DATA

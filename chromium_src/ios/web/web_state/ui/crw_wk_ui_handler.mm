// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/public/web_client.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

#define IsAppSpecificURL(URL) \
  IsAppSpecificURL(URL) &&    \
      self.mojoFacade->IsWebUIMessageAllowedForFrame(origin_url, prompt)

#include <ios/web/web_state/ui/crw_wk_ui_handler.mm>

#undef IsAppSpecificURL

#pragma clang diagnostic pop

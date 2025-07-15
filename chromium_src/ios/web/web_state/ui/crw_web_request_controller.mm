// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/web_state/web_state_impl.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

// Replace WebUI handler with Brave's
#define CreateWebUI CreateBraveWebUI
#define ClearWebUI ClearBraveWebUI
#define HasWebUI HasBraveWebUI

#include <ios/web/web_state/ui/crw_web_request_controller.mm>

#undef HasWebUI
#undef ClearWebUI
#undef CreateWebUI

#pragma clang diagnostic pop

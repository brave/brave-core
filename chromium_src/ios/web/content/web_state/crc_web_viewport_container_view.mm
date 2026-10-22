// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "ios/web/content/web_state/crc_web_viewport_container_view.h"

#import "ios/web/common/crw_viewport_adjustment.h"
#import "ios/web/common/crw_viewport_adjustment_container.h"

// Upstream hardcodes Chrome's toolbar heights as Blink browser controls. Brave
// insets the content view for its own toolbars, so Blink must not reserve
// space for them again.
#define UIEdgeInsetsMake(...) UIEdgeInsetsZero

#include <ios/web/content/web_state/crc_web_viewport_container_view.mm>

#undef UIEdgeInsetsMake

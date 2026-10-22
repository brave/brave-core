// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "ios/web/content/web_state/content_web_state.h"

#import "content/public/browser/navigation_entry.h"
#import "content/public/browser/web_contents.h"
#import "ios/web/content/navigation/content_navigation_context.h"
#import "ios/web/public/navigation/navigation_item.h"

// Match WebStateImpl, which reports virtual URLs so that rewritten URLs such as
// about:newtab stay visible to the embedder.
#define GetURL GetVirtualURL
#include <ios/web/content/web_state/content_web_state.mm>
#undef GetURL

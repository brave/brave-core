// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#include "ios/web/public/web_state.h"

#define BRAVE_SHOULD_BLOCK_UNIVERSAL_LINKS                            \
  brave::ShouldBlockUniversalLinks(                                   \
      static_cast<web::WebState*>(self.webStateImpl), action.request, \
      &forceBlockUniversalLinks);
#include "src/ios/web/navigation/crw_wk_navigation_handler.mm"
#undef BRAVE_SHOULD_BLOCK_UNIVERSAL_LINKS

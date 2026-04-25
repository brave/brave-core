// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "ios/web/common/crw_edit_menu_builder.h"
#import "ios/web/web_state/crw_web_view.h"

@interface BraveCRWWebView : CRWWebView
@end

#define CRWWebView BraveCRWWebView
#include <ios/web/web_state/web_view_internal_creation_util.mm>
#undef CRWWebView

@implementation BraveCRWWebView

- (void)buildMenuWithBuilder:(id<UIMenuBuilder>)builder {
  [super buildMenuWithBuilder:builder];
  // Typically Chromium only calls buildMenuWithBuilder when there is text
  // selected by the user (`canPerformAction` returning true for the `copy`
  // selector). This override allows it to also call the edit menu builder when
  // paste is available so we can add additional menu items such as Force Paste
  if (![self canPerformAction:@selector(copy:) withSender:self] &&
      [self canPerformAction:@selector(paste:) withSender:self]) {
    [self.editMenuBuilder buildMenuWithBuilder:builder];
  }
}

@end

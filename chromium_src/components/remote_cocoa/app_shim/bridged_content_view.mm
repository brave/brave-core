/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/remote_cocoa/app_shim/bridged_content_view.mm"

@implementation BridgedContentView (BraveExtension)

- (void)selectAll:(id)sender {
  // Override BridgedContentView:selectAll with our own objective-C extension.
  // Upstream implementation sends ui::EF_CONTROL_DOWN, which is not correspond
  // to the actual shortcut. This caused a conflict with our accelerator
  // service.
  // https://github.com/brave/brave-browser/issues/31059
  [self handleAction:ui::TextEditCommand::SELECT_ALL
             keyCode:ui::VKEY_A
             domCode:ui::DomCode::US_A
          eventFlags:ui::EF_COMMAND_DOWN];
}

@end  // BridgedContentView(BraveExtension)

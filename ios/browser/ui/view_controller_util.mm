/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/view_controller_util.h"

#include "base/apple/foundation_util.h"

namespace brave {

UIViewController* ViewControllerForView(UIView* view) {
  for (UIResponder* responder = view.nextResponder; responder;
       responder = responder.nextResponder) {
    if (auto* controller = base::apple::ObjCCast<UIViewController>(responder)) {
      return controller;
    }
  }

  return nil;
}

}  // namespace brave

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_UI_VIEW_CONTROLLER_UTIL_H_
#define BRAVE_IOS_BROWSER_UI_VIEW_CONTROLLER_UTIL_H_

#import <UIKit/UIKit.h>

namespace brave {

// Returns the view controller whose view hierarchy contains `view`, found by
// walking the responder chain. nil if `view` is not in one.
UIViewController* ViewControllerForView(UIView* view);

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_UI_VIEW_CONTROLLER_UTIL_H_

/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_

namespace gfx {
class Point;
}  // namespace gfx

class BrowserView;

namespace brave {

// Helper function to set additional draggable area in client view.
// Returns HTNOWHERE if the point is not what we're interested in. In that
// case, caller should depend on the default behavior.
int NonClientHitTest(BrowserView* browser_view,
                     const gfx::Point& point_in_widget);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_

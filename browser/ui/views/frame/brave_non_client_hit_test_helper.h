/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_

#include "base/scoped_multi_source_observation.h"
#include "ui/views/view_observer.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace views {
class View;
}  // namespace views

class BrowserView;

// Helper class to set additional draggable area in client view.
// Returns HTNOWHERE if the point is not what we're interested in.
class BraveNonClientHitTestHelper : public views::ViewObserver {
 public:
  BraveNonClientHitTestHelper();
  ~BraveNonClientHitTestHelper() override;

  int NonClientHitTest(BrowserView* browser_view,
                       const gfx::Point& point_in_widget);

  // Calling this will register the given `view` as a caption area so that users
  // can drag the window by dragging the view. Note that the children of the
  // given `view` will not be considered as caption areas.
  void RegisterCaptionArea(views::View* view);

  // views::ViewObserver:
  void OnChildViewAdded(views::View* observed_view,
                        views::View* child) override;
  void OnViewIsDeleting(views::View* observed_view) override;

 private:
  base::ScopedMultiSourceObservation<views::View, views::ViewObserver>
      observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_NON_CLIENT_HIT_TEST_HELPER_H_

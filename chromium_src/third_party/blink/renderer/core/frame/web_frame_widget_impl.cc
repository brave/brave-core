/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/web_frame_widget_impl.h"

#include "third_party/blink/renderer/core/layout/hit_test_location.h"
#include "third_party/blink/renderer/core/layout/hit_test_request.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"

#include <third_party/blink/renderer/core/frame/web_frame_widget_impl.cc>

namespace blink {

// Like `HitTestResultAt`, but penetrates topmost elements to find an
// image/link underneath, the same way `ContextMenuController` does.
WebHitTestResult WebFrameWidgetImpl::HitTestResultForContextMenuAt(
    const gfx::PointF& point) {
  gfx::PointF point_in_viewport = point;
  point_in_viewport.Scale(
      1 / GetPage()->GetChromeClient().InputEventsScaleForEmulation());
  LocalFrameView* const view = LocalRootImpl()->GetFrameView();
  const gfx::PointF point_in_root_frame(
      view->ViewportToFrame(point_in_viewport));
  const gfx::PointF point_in_document =
      LocalRootImpl()->GetFrame()->View()->ConvertFromRootFrame(
          point_in_root_frame);
  const HitTestLocation location(point_in_document);
  HitTestResult result =
      LocalRootImpl()->GetFrame()->View()->HitTestWithThrottlingAllowed(
          location, HitTestRequest::kReadOnly | HitTestRequest::kActive |
                        HitTestRequest::kPenetratingList |
                        HitTestRequest::kListBased);
  if (!result.AbsoluteImageURL().IsEmpty() || result.URLElement()) {
    return result;
  }
  const HitTestResult::NodeSet& candidates = result.ListBasedTestResult();
  for (Node* const candidate : candidates) {
    if (!HitTestResult::AbsoluteImageURL(candidate).IsEmpty()) {
      result.SetInnerNode(candidate);
      return result;
    }
  }
  for (Node* const candidate : candidates) {
    if (Element* const link = candidate->EnclosingLinkEventParentOrSelf()) {
      result.SetURLElement(link);
      break;
    }
  }
  return result;
}

}  // namespace blink

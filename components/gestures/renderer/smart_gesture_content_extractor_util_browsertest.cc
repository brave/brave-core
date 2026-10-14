// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/renderer/smart_gesture_content_extractor_util.h"

#include "brave/components/gestures/core/common/mojom/smart_gesture_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view.h"
#include "ui/gfx/geometry/point_f.h"
#include "url/gurl.h"

namespace gestures {

class SmartGestureContentExtractorUtilRenderViewTest
    : public content::RenderViewTest {
 protected:
  void SetUp() override {
    content::RenderViewTest::SetUp();
    // This harness has no real URL loader factory, so an `<img>`'s src
    // must never actually be fetched; the hit test only needs the URL.
    web_view_->GetSettings()->SetImagesEnabled(false);
  }

  mojom::SmartGestureContentPtr ExtractContentAtHitTestPoint() {
    blink::WebLocalFrame* const web_frame = GetMainRenderFrame()->GetWebFrame();
    // Elements are positioned absolutely at the origin, so (10, 10) reliably
    // lands on whichever one the test's markup places there.
    const blink::WebHitTestResult hit_test_result =
        web_frame->FrameWidget()->HitTestResultAt(gfx::PointF(10, 10));
    return ExtractSmartGestureContent(hit_test_result, GetMainRenderFrame());
  }
};

TEST_F(SmartGestureContentExtractorUtilRenderViewTest,
       ReturnsNullWhenHitTestingPlainText) {
  LoadHTML("<body>qux quux</body>");

  EXPECT_TRUE(ExtractContentAtHitTestPoint().is_null());
}

TEST_F(SmartGestureContentExtractorUtilRenderViewTest, ReturnsLinkUrl) {
  LoadHTML(
      "<a href='https://foo.com/' "
      "style='position:absolute;left:0;top:0;width:50px;height:50px'>"
      "foo</a>");

  mojom::SmartGestureContentPtr gesture_content =
      ExtractContentAtHitTestPoint();

  ASSERT_TRUE(gesture_content->is_link_url());
  EXPECT_EQ(GURL("https://foo.com/"), gesture_content->get_link_url());
}

TEST_F(SmartGestureContentExtractorUtilRenderViewTest, ReturnsImageUrl) {
  LoadHTML(
      "<img src='https://bar.com/baz.png' "
      "style='position:absolute;left:0;top:0;width:50px;height:50px'>");

  mojom::SmartGestureContentPtr gesture_content =
      ExtractContentAtHitTestPoint();

  ASSERT_TRUE(gesture_content->is_image_url());
  EXPECT_EQ(GURL("https://bar.com/baz.png"), gesture_content->get_image_url());
}

TEST_F(SmartGestureContentExtractorUtilRenderViewTest,
       PrefersImageUrlOverOverlappingLinkUrl) {
  LoadHTML(
      "<a href='https://foo.com/' "
      "style='position:absolute;left:0;top:0;width:50px;height:50px'>"
      "<img src='https://bar.com/baz.png' "
      "style='position:absolute;left:0;top:0;width:50px;height:50px'>"
      "</a>");

  mojom::SmartGestureContentPtr gesture_content =
      ExtractContentAtHitTestPoint();

  ASSERT_TRUE(gesture_content->is_image_url());
  EXPECT_EQ(GURL("https://bar.com/baz.png"), gesture_content->get_image_url());
}

TEST_F(SmartGestureContentExtractorUtilRenderViewTest,
       ReturnsSelectedTextWhenNoLinkOrImageIsAtThePoint) {
  LoadHTML("<body>qux quux</body>");
  ExecuteJavaScriptForTests("document.execCommand('selectAll')");

  mojom::SmartGestureContentPtr gesture_content =
      ExtractContentAtHitTestPoint();

  ASSERT_TRUE(gesture_content->is_selected_text());
  EXPECT_EQ("qux quux", gesture_content->get_selected_text());
}

}  // namespace gestures

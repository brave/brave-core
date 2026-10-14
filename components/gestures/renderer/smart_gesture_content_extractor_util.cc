// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/renderer/smart_gesture_content_extractor_util.h"

#include <string>

#include "brave/components/gestures/renderer/gesture_selected_text_util.h"
#include "third_party/blink/public/platform/web_url.h"  // IWYU pragma: keep
#include "third_party/blink/public/web/web_hit_test_result.h"
#include "url/gurl.h"

namespace gestures {

namespace {

// Tried in order; the first non-null result wins. Adding a new content type
// is one new function plus one entry here, not a growing if/else chain.
using ContentExtractor =
    mojom::SmartGestureContentPtr (*)(const blink::WebHitTestResult&,
                                      content::RenderFrame*);

// Tried first, since an explicit text selection is the clearest signal of
// user intent, ahead of whatever happens to be under the cursor.
mojom::SmartGestureContentPtr ExtractSelectedText(
    const blink::WebHitTestResult& /*hit_test_result*/,
    content::RenderFrame* render_frame) {
  const std::string selected_text = GetSelectedText(render_frame);
  return selected_text.empty()
             ? nullptr
             : mojom::SmartGestureContent::NewSelectedText(selected_text);
}

// Tried before `ExtractLinkUrl`, since a thumbnail-style <img> wrapped in an
// <a> is still the more specific place to act from.
mojom::SmartGestureContentPtr ExtractImageUrl(
    const blink::WebHitTestResult& hit_test_result,
    content::RenderFrame* /*render_frame*/) {
  const GURL image_url(hit_test_result.AbsoluteImageURL());
  return image_url.is_valid()
             ? mojom::SmartGestureContent::NewImageUrl(image_url)
             : nullptr;
}

mojom::SmartGestureContentPtr ExtractLinkUrl(
    const blink::WebHitTestResult& hit_test_result,
    content::RenderFrame* /*render_frame*/) {
  const GURL link_url(hit_test_result.AbsoluteLinkURL());
  return link_url.is_valid() ? mojom::SmartGestureContent::NewLinkUrl(link_url)
                             : nullptr;
}

constexpr ContentExtractor kContentExtractors[] = {
    &ExtractSelectedText, &ExtractImageUrl, &ExtractLinkUrl};

}  // namespace

mojom::SmartGestureContentPtr ExtractSmartGestureContent(
    const blink::WebHitTestResult& hit_test_result,
    content::RenderFrame* render_frame) {
  for (ContentExtractor content_extractor : kContentExtractors) {
    if (mojom::SmartGestureContentPtr content =
            content_extractor(hit_test_result, render_frame)) {
      return content;
    }
  }
  return nullptr;
}

}  // namespace gestures

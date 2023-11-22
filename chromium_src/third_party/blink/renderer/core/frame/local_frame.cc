/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/local_frame.h"

#include "brave/components/brave_page_graph/common/buildflags.h"
#include "skia/ext/skia_utils_base.h"
#include "third_party/blink/renderer/core/core_probe_sink.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types_3d.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#define AddInspectorTraceEvents(...)                               \
  AddInspectorTraceEvents(__VA_ARGS__);                            \
  IF_BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH, {                          \
    DCHECK(IsLocalRoot());                                         \
    /* InstallSupplements call is too late, do it here instead. */ \
    PageGraph::ProvideTo(*this);                                   \
  })

#include "src/third_party/blink/renderer/core/frame/local_frame.cc"

#undef AddInspectorTraceEvents

namespace blink {

namespace {

// Copied same method from
// third_party/blink/renderer/core/editing/editing_utilities.cc
scoped_refptr<Image> ImageFromNode(const Node& node) {
  DCHECK(!node.GetDocument().NeedsLayoutTreeUpdate());
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      node.GetDocument().Lifecycle());

  const LayoutObject* const layout_object = node.GetLayoutObject();
  if (!layout_object)
    return nullptr;

  if (layout_object->IsCanvas()) {
    return To<HTMLCanvasElement>(const_cast<Node&>(node))
        .Snapshot(FlushReason::kNon2DCanvas, kFrontBuffer);
  }

  if (!layout_object->IsImage())
    return nullptr;

  const auto& layout_image = To<LayoutImage>(*layout_object);
  const ImageResourceContent* const cached_image = layout_image.CachedImage();
  if (!cached_image || cached_image->ErrorOccurred())
    return nullptr;
  return cached_image->GetImage();
}

}  // namespace

// Referred LocalFrame::CopyImageAtViewportPoint().
SkBitmap LocalFrame::GetImageAtViewportPoint(const gfx::Point& viewport_point) {
  HitTestResult result = HitTestResultForVisualViewportPos(viewport_point);
  if (!IsA<HTMLCanvasElement>(result.InnerNodeOrImageMapImage()) &&
      result.AbsoluteImageURL().IsEmpty()) {
    // There isn't actually an image at these coordinates.  Might be because
    // the window scrolled while the context menu was open or because the page
    // changed itself between when we thought there was an image here and when
    // we actually tried to retrieve the image.
    //
    // FIXME: implement a cache of the most recent HitTestResult to avoid having
    //        to do two hit tests.
    return {};
  }

  const scoped_refptr<Image> image =
      ImageFromNode(*result.InnerNodeOrImageMapImage());
  if (!image.get())
    return {};

  // Referred SystemClipboard::WriteImageWithTag() about how to get bitmap data
  // from Image.
  PaintImage paint_image = image->PaintImageForCurrentFrame();
  // Orient the data.
  if (!image->HasDefaultOrientation()) {
    paint_image = Image::ResizeAndOrientImage(
        paint_image, image->CurrentFrameOrientation(), gfx::Vector2dF(1, 1), 1,
        kInterpolationNone);
  }
  SkBitmap bitmap;
  if (sk_sp<SkImage> sk_image = paint_image.GetSwSkImage())
    sk_image->asLegacyBitmap(&bitmap);

  // The bitmap backing a canvas can be in non-native skia pixel order (aka
  // RGBA when kN32_SkColorType is BGRA-ordered, or higher bit-depth color-types
  // like F16. The IPC to the browser requires the bitmap to be in N32 format
  // so we convert it here if needed.
  SkBitmap n32_bitmap;
  if (skia::SkBitmapToN32OpaqueOrPremul(bitmap, &n32_bitmap) &&
      !n32_bitmap.isNull()) {
    return n32_bitmap;
  }

  return {};
}

}  // namespace blink

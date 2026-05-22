// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

// Make GetTopSeparatorType virtual to override it in
// BraveBrowserViewTabbedLayoutImpl.
#define GetTopSeparatorType()                    \
  GetTopSeparatorType_Unused();                  \
  friend class BraveBrowserViewTabbedLayoutImpl; \
  virtual TopSeparatorType GetTopSeparatorType()

// Make GetHorizontalTabStripLeadingMargin virtual so subclasses can reduce the
// leading inset between leading caption buttons and the first tab in compact
// horizontal tabs mode.
#define GetHorizontalTabStripLeadingMargin(params)   \
  GetHorizontalTabStripLeadingMargin_Unused(params); \
  friend class BraveBrowserViewTabbedLayoutImpl;     \
  virtual int GetHorizontalTabStripLeadingMargin(params)

// Make CalculateTopContainerLayoutImpl virtual to override it in
// BraveBrowserViewTabbedLayoutImpl. CalculateTopContainerLayout is virtual
// but it's only a wrapper around the Impl and internally the Impl is called
// so we need to override the Impl.
#define CalculateTopContainerLayoutImpl virtual CalculateTopContainerLayoutImpl

// Make ShadowOverlayVisible virtual so Brave can suppress the upstream
// toolbar-height side panel's shadow overlay (Brave has its own rounded-
// corners shadow handling around contents and the side panel).
#define ShadowOverlayVisible()                   \
  ShadowOverlayVisible_Unused();                 \
  friend class BraveBrowserViewTabbedLayoutImpl; \
  virtual bool ShadowOverlayVisible()

#include <chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h>  // IWYU pragma: export

#undef ShadowOverlayVisible
#undef CalculateTopContainerLayoutImpl
#undef GetTopSeparatorType
#undef GetHorizontalTabStripLeadingMargin

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_LAYOUT_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

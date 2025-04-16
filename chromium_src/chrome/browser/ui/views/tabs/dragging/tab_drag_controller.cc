/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#include "ui/views/widget/widget.h"

// Prevent unrelated StackAtTop re-define.
#if BUILDFLAG(IS_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

#define TabDragController TabDragControllerChromium

// StackAtTop() is called to bring browser window to the front.
// It's called for TabDragContext()->GetWidget(). In horizontal tab,
// returned widget is browsr window's widget. But it's vertical tab widget in
// vertical tab mode. To bring dragged window up in vertical tab mode,
// StackAtTop() should be called vertical tab widget's top level widget.
// This also works in horizontal tab mode because it's already top level window.
#define StackAtTop GetTopLevelWidget()->StackAtTop
#define GetWindowBoundsInScreen GetTopLevelWidget()->GetWindowBoundsInScreen

#include "src/chrome/browser/ui/views/tabs/dragging/tab_drag_controller.cc"

#undef GetWindowBoundsInScreen
#undef StackAtTop
#undef GetBrowserViewForNativeWindow
#undef TabDragController

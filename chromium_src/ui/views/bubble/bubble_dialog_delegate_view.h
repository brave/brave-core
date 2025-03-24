/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_BUBBLE_BUBBLE_DIALOG_DELEGATE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_BUBBLE_BUBBLE_DIALOG_DELEGATE_VIEW_H_

#include "ui/views/window/dialog_delegate.h"

class BraveNewsBubbleView;
class BraveHelpBubbleDelegateView;
class SplitViewMenuBubble;
class WaybackMachineBubbleView;
class SidebarItemAddedFeedbackBubble;
class SidebarEditItemBubbleDelegateView;
class SidebarAddItemBubbleDelegateView;

namespace playlist {
class PlaylistBubbleView;
}  // namespace playlist

namespace views {
class BraveBubbleDialogDelegateView;
}

#define CreatePassKey                                  \
  CreatePassKey_Unused();                              \
  friend class ::BraveNewsBubbleView;                  \
  friend class ::BraveHelpBubbleDelegateView;          \
  friend class ::WaybackMachineBubbleView;             \
  friend class ::playlist::PlaylistBubbleView;         \
  friend class ::SplitViewMenuBubble;                  \
  friend class ::SidebarItemAddedFeedbackBubble;       \
  friend class ::SidebarEditItemBubbleDelegateView;    \
  friend class ::SidebarAddItemBubbleDelegateView;     \
  friend class ::views::BraveBubbleDialogDelegateView; \
  static BddvPassKey CreatePassKey

#include "src/ui/views/bubble/bubble_dialog_delegate_view.h"  // IWYU pragma: export

#undef CreatePassKey

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_BUBBLE_BUBBLE_DIALOG_DELEGATE_VIEW_H_

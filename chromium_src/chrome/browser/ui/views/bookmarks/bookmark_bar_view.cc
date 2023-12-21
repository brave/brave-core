/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_view_ids.h"
#include "brave/browser/ui/views/bookmarks/bookmark_bar_instructions_view.h"
#include "brave/browser/ui/views/bookmarks/brave_bookmark_context_menu.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace {
constexpr int kBookmarkBarInstructionsPadding = 6;

BookmarkBarInstructionsView* GetInstructionView(
    views::View* bookmark_bar_view) {
  for (views::View* child : bookmark_bar_view->children()) {
    if (child->GetID() == BRAVE_VIEW_ID_BOOKMARK_IMPORT_INSTRUCTION_VIEW)
      return static_cast<BookmarkBarInstructionsView*>(child);
  }
  return nullptr;
}

void LayoutBookmarkBarInstructionsView(views::View* bookmark_bar_view,
                                       bookmarks::BookmarkModel* model,
                                       Browser* browser,
                                       int button_height,
                                       int x,
                                       int max_x,
                                       int y) {
  // Parent view is not ready to layout bookmark bar instruction view.
  if (max_x <= 0)
    return;

  DCHECK(bookmark_bar_view);
  DCHECK(model);
  DCHECK(browser);

  const bool show_instructions =
      model->loaded() && model->bookmark_bar_node()->children().empty();
  views::View* import_instruction_view = GetInstructionView(bookmark_bar_view);
  if (show_instructions) {
    DCHECK_GE(button_height, 0);
    DCHECK_GE(x, 0);
    DCHECK_GE(y, 0);

    if (!import_instruction_view) {
      import_instruction_view = new BookmarkBarInstructionsView(browser);
      bookmark_bar_view->AddChildView(import_instruction_view);
    }
    import_instruction_view->SetVisible(true);
    gfx::Size pref = import_instruction_view->GetPreferredSize();
    import_instruction_view->SetBounds(
        x + kBookmarkBarInstructionsPadding, y,
        std::min(static_cast<int>(pref.width()), max_x - x), button_height);
  } else {
    if (import_instruction_view)
      import_instruction_view->SetVisible(false);
  }
}

}  // namespace

namespace views {
void InstallNoHighlightPathGenerator(View* view) {
  // Do nothing: the default highlight path is what we want.
}
}  // namespace views

#define BRAVE_LAYOUT                                                  \
  LayoutBookmarkBarInstructionsView(this, bookmark_model_, browser(), \
                                    button_height, x, max_x, y);
#define BookmarkContextMenu BraveBookmarkContextMenu
#define InstallPillHighlightPathGenerator InstallNoHighlightPathGenerator
#include "src/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc"
#undef InstallPillHighlightPathGenerator
#undef BookmarkContextMenu
#undef BRAVE_LAYOUT

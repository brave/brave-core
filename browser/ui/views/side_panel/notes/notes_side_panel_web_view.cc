// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/notes/notes_side_panel_web_view.h"

#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "brave/browser/ui/webui/side_panel/notes/notes_ui.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "url/gurl.h"

namespace {
inline constexpr char kNotesPanelURL[] = "chrome://notes-panel/";
}  // namespace

// static
std::unique_ptr<views::View> NotesSidePanelWebView::CreateView(
    Profile* profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);
  auto web_view = std::make_unique<NotesSidePanelWebView>(
      scope, std::make_unique<WebUIContentsWrapperT<NotesUI>>(
                 GURL(kNotesPanelURL), profile, 0,
                 /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

NotesSidePanelWebView::NotesSidePanelWebView(
    SidePanelEntryScope& scope,
    std::unique_ptr<WebUIContentsWrapperT<NotesUI>> contents_wrapper)
    : SidePanelWebUIViewT<NotesUI>(scope,
                                   base::RepeatingClosure(),
                                   base::RepeatingClosure(),
                                   std::move(contents_wrapper)) {}

NotesSidePanelWebView::~NotesSidePanelWebView() = default;

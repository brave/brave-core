// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_NOTES_NOTES_SIDE_PANEL_WEB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_NOTES_NOTES_SIDE_PANEL_WEB_VIEW_H_

#include <memory>

#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"

class NotesUI;
class Profile;

class NotesSidePanelWebView : public SidePanelWebUIViewT<NotesUI> {
 public:
  static std::unique_ptr<views::View> CreateView(
      Profile* profile,
      SidePanelEntryScope& scope);

  NotesSidePanelWebView(
      SidePanelEntryScope& scope,
      std::unique_ptr<WebUIContentsWrapperT<NotesUI>> contents_wrapper);
  ~NotesSidePanelWebView() override;

  NotesSidePanelWebView(const NotesSidePanelWebView&) = delete;
  NotesSidePanelWebView& operator=(const NotesSidePanelWebView&) = delete;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_NOTES_NOTES_SIDE_PANEL_WEB_VIEW_H_

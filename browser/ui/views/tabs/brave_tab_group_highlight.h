/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HIGHLIGHT_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HIGHLIGHT_H_

#include "chrome/browser/ui/views/tabs/tab_group_highlight.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveTabGroupHighlight : public TabGroupHighlight {
  METADATA_HEADER(BraveTabGroupHighlight, TabGroupHighlight)
 public:
  using TabGroupHighlight::TabGroupHighlight;
  ~BraveTabGroupHighlight() override;

 private:
  // TabGroupHighlight:
  SkPath GetPath() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_HIGHLIGHT_H_

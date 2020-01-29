// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_READER_MODE_BRAVE_READER_MODE_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_READER_MODE_BRAVE_READER_MODE_ICON_VIEW_H_

#include "chrome/browser/ui/views/reader_mode/reader_mode_icon_view.h"


class BraveReaderModeIconView : public ReaderModeIconView {
 public:
    using ReaderModeIconView::ReaderModeIconView;
 protected:
    void UpdateImpl() override;

  DISALLOW_COPY_AND_ASSIGN(BraveReaderModeIconView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_READER_MODE_BRAVE_READER_MODE_ICON_VIEW_H_

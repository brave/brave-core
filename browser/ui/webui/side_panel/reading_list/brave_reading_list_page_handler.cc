/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/side_panel/reading_list/brave_reading_list_page_handler.h"

#include <utility>

#include "content/public/browser/web_ui.h"

BraveReadingListPageHandler::BraveReadingListPageHandler(
    mojo::PendingReceiver<reading_list::mojom::PageHandler> receiver,
    mojo::PendingRemote<reading_list::mojom::Page> page,
    ReadingListUI* reading_list_ui,
    content::WebUI* web_ui)
    : ReadingListPageHandler(std::move(receiver),
                             std::move(page),
                             reading_list_ui,
                             web_ui),
      WebContentsObserver(web_ui->GetWebContents()) {}

BraveReadingListPageHandler::~BraveReadingListPageHandler() = default;

void BraveReadingListPageHandler::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::VISIBLE) {
    // As we have our own panel open/closing logic,
    // UpdateCurrentPageActionButton() could be called before web contents is
    // not visible during the initialization. UpdateCurrentPageActionButton()
    // does early return when web contents is not visible. Because of this,
    // panel can't get proper initial button state. Need to make sure that it
    // should be called once when web contents is visible after panel is opened.
    // After that, upstream code will update properly for relavant events such
    // as tab change and etc.
    Observe(nullptr);
    UpdateCurrentPageActionButton();
  }
}

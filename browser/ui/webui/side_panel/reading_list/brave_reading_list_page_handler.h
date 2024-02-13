/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_BRAVE_READING_LIST_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_BRAVE_READING_LIST_PAGE_HANDLER_H_

#include "chrome/browser/ui/webui/side_panel/reading_list/reading_list.mojom.h"
#include "chrome/browser/ui/webui/side_panel/reading_list/reading_list_page_handler.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ReadingListUI;

class BraveReadingListPageHandler : public ReadingListPageHandler,
                                    public content::WebContentsObserver {
 public:
  BraveReadingListPageHandler(
      mojo::PendingReceiver<reading_list::mojom::PageHandler> receiver,
      mojo::PendingRemote<reading_list::mojom::Page> page,
      ReadingListUI* reading_list_ui,
      content::WebUI* web_ui);
  ~BraveReadingListPageHandler() override;

  // content::WebContentsObserver overrides:
  void OnVisibilityChanged(content::Visibility visibility) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SIDE_PANEL_READING_LIST_BRAVE_READING_LIST_PAGE_HANDLER_H_

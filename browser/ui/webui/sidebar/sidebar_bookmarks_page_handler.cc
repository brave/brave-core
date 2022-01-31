/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/sidebar/sidebar_bookmarks_page_handler.h"

#include <memory>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/ui/bookmarks/bookmark_stats.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace {

class BookmarkContextMenu : public ui::SimpleMenuModel,
                            public ui::SimpleMenuModel::Delegate,
                            public BookmarkContextMenuControllerDelegate {
 public:
  explicit BookmarkContextMenu(Browser* browser,
                               sidebar::Sidebar* sidebar,
                               const bookmarks::BookmarkNode* bookmark)
      : ui::SimpleMenuModel(this),
        controller_(base::WrapUnique(new BookmarkContextMenuController(
            browser->window()->GetNativeWindow(),
            this,
            browser,
            browser->profile(),
            base::BindRepeating(
                [](content::PageNavigator* navigator) { return navigator; },
                browser),
            // Do we need our own histogram enum?
            BOOKMARK_LAUNCH_LOCATION_SIDE_PANEL_CONTEXT_MENU,
            bookmark->parent(),
            {bookmark}))),
        sidebar_(sidebar) {
    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL);
    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_NEW_WINDOW);
    AddItem(IDC_BOOKMARK_BAR_OPEN_ALL_INCOGNITO);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(bookmark->is_folder() ? IDC_BOOKMARK_BAR_RENAME_FOLDER
                                  : IDC_BOOKMARK_BAR_EDIT);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_CUT);
    AddItem(IDC_COPY);
    AddItem(IDC_PASTE);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_BAR_REMOVE);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_BAR_ADD_NEW_BOOKMARK);
    AddItem(IDC_BOOKMARK_BAR_NEW_FOLDER);
    AddSeparator(ui::NORMAL_SEPARATOR);

    AddItem(IDC_BOOKMARK_MANAGER);
  }
  ~BookmarkContextMenu() override = default;

  void ExecuteCommand(int command_id, int event_flags) override {
    controller_->ExecuteCommand(command_id, event_flags);
  }

  bool IsCommandIdEnabled(int command_id) const override {
    return controller_->IsCommandIdEnabled(command_id);
  }

  bool IsCommandIdVisible(int command_id) const override {
    return controller_->IsCommandIdVisible(command_id);
  }

  // BookmarkContextMenuControllerDelegate:
  void CloseMenu() override { sidebar_->HideCustomContextMenu(); }

 private:
  void AddItem(int command_id) {
    ui::SimpleMenuModel::AddItem(
        command_id,
        controller_->menu_model()->GetLabelAt(
            controller_->menu_model()->GetIndexOfCommandId(command_id)));
  }

  std::unique_ptr<BookmarkContextMenuController> controller_;
  sidebar::Sidebar* sidebar_ = nullptr;
};

}  // namespace

SidebarBookmarksPageHandler::SidebarBookmarksPageHandler(
    mojo::PendingReceiver<sidebar::mojom::BookmarksPageHandler> receiver)
    : receiver_(this, std::move(receiver)) {}

SidebarBookmarksPageHandler::~SidebarBookmarksPageHandler() = default;

void SidebarBookmarksPageHandler::OpenBookmark(
    const GURL& url,
    int32_t parent_folder_depth,
    ui::mojom::ClickModifiersPtr click_modifiers) {
  Browser* browser = chrome::FindLastActive();
  if (!browser)
    return;

  WindowOpenDisposition open_location = ui::DispositionFromClick(
      click_modifiers->middle_button, click_modifiers->alt_key,
      click_modifiers->ctrl_key, click_modifiers->meta_key,
      click_modifiers->shift_key);
  content::OpenURLParams params(url, content::Referrer(), open_location,
                                ui::PAGE_TRANSITION_AUTO_BOOKMARK, false);
  browser->OpenURL(params);
}

void SidebarBookmarksPageHandler::ShowContextMenu(const std::string& id_string,
                                                  const gfx::Point& point) {
  int64_t id;
  if (!base::StringToInt64(id_string, &id))
    return;

  Browser* browser = chrome::FindLastActive();
  if (!browser)
    return;

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser->profile());
  const bookmarks::BookmarkNode* bookmark =
      bookmarks::GetBookmarkNodeByID(bookmark_model, id);
  if (!bookmark)
    return;

  auto* sidebar =
      static_cast<BraveBrowser*>(browser)->sidebar_controller()->sidebar();
  sidebar->ShowCustomContextMenu(
      point, std::make_unique<BookmarkContextMenu>(browser, sidebar, bookmark));
}

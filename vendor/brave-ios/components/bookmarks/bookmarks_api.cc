/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/brave-ios/components/bookmarks/bookmarks_api.h"
#include "brave/ios/browser/browser_state/browser_state_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/pref_names.h"

#include "base/time/time.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/undo_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace bookmarks {

BookmarksAPI::BookmarksAPI(BookmarkModel* model,
                           BookmarkUndoService* undo_service)
    : model_(model),
      bookmark_undo_service_(undo_service) {
}

BookmarksAPI::~BookmarksAPI() {}

void BookmarksAPI::Create(const int64_t& parent_id,
                          size_t index,
                          const base::string16& title,
                          const GURL& url) {
  const BookmarkNode* parent =
      bookmarks::GetBookmarkNodeByID(model_, parent_id);

  DCHECK(parent);

  const BookmarkNode* node;
  if (url.is_valid()) {
    node = model_->AddURL(parent, index, title, url); //last parameter is nullptr in new code
  } else {
    node = model_->AddFolder(parent, index, title); //last parameter is nullptr in new code
    model_->SetDateFolderModified(parent, base::Time::Now());
  }

  DCHECK(node);
}

void BookmarksAPI::Move(int64_t id,
                   int64_t parent_id,
                   size_t index)
{
    //DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(model_->loaded());

    const BookmarkNode* node =
        bookmarks::GetBookmarkNodeByID(model_, id);
    DCHECK(IsEditable(node));

    const BookmarkNode* new_parent_node =
        bookmarks::GetBookmarkNodeByID(model_, parent_id);

    if (node->parent() != new_parent_node) {
      model_->Move(node, new_parent_node, index);
    }
}

void BookmarksAPI::Update(int64_t id,
                          const base::string16& title,
                          const GURL& url)
{
    const BookmarkNode* node =
        bookmarks::GetBookmarkNodeByID(model_, id);

    DCHECK(IsEditable(node));
    model_->SetTitle(node, title);
    model_->SetURL(node, url);
}

void BookmarksAPI::Remove(int64_t id)
{
    //DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(model_->loaded());

    const BookmarkNode* node =
        bookmarks::GetBookmarkNodeByID(model_, id);

    if (!IsEditable(node)) {
        NOTREACHED();
        return;
    }

    model_->Remove(node);
}

void BookmarksAPI::RemoveAll()
{
    //DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(model_->loaded());
    model_->RemoveAllUserBookmarks();
}

void BookmarksAPI::Search(const base::string16& search_query,
                          size_t max_count,
                          std::vector<const BookmarkNode*>* nodes)
{
    DCHECK(model_->loaded());
    bookmarks::QueryFields query;
    query.word_phrase_query.reset(new base::string16(search_query));
    GetBookmarksMatchingProperties(model_, query, max_count, nodes);
    DCHECK(nodes->size() <= max_count);
}

void BookmarksAPI::Undo()
{
    //DCHECK_CURRENTLY_ON(web::WebThread::UI);
    DCHECK(model_->loaded());
    UndoManager* undo_manager = bookmark_undo_service_->undo_manager();
    undo_manager->Undo();
}

bool BookmarksAPI::IsEditable(const BookmarkNode* node) const
{
    if (!node || (node->type() != BookmarkNode::FOLDER &&
        node->type() != BookmarkNode::URL)) {
      return false;
    }

    //ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
    return true; //Possibly query iOS to see if editing is allowed..
    //return browser_state_->GetPrefs()->GetBoolean(bookmarks::prefs::kEditBookmarksEnabled);
}

const BookmarkNode* BookmarksAPI::GetBookmarksMobileFolder() const {
    const int64_t kLastUsedFolderNone = -1;
    ChromeBrowserState* browser_state_ = BrowserStateManager::GetInstance().GetBrowserState();
    
    BookmarkModel* bookmarks =
        ios::BookmarkModelFactory::GetForBrowserState(browser_state_);
    const BookmarkNode* defaultFolder = bookmarks->mobile_node();

    PrefService* prefs = browser_state_->GetPrefs();
    int64_t node_id = prefs->GetInt64(prefs::kIosBookmarkFolderDefault);
    if (node_id == kLastUsedFolderNone)
      node_id = defaultFolder->id();
    const BookmarkNode* result =
        bookmarks::GetBookmarkNodeByID(bookmarks, node_id);

    if (result)
      return result;

    return defaultFolder;

}

}  // namespace bookmarks

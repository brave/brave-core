/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_
#define BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

class BookmarkUndoService;

namespace bookmarks {

class BookmarkModel;
class BookmarkNode;

class BookmarksAPI {
 public:
  BookmarksAPI(BookmarkModel* model, BookmarkUndoService* undo_service);
  ~BookmarksAPI();

  void Create(const int64_t& parent_id,
              size_t index,
              const base::string16& title,
              const GURL& url);

  void Move(int64_t id,
            int64_t parent_id,
            size_t index);

  void Update(int64_t id,
              const base::string16& title,
              const GURL& url);

  void Remove(int64_t id);
  void RemoveAll();

  void Search(const base::string16& search_query,
                            size_t max_count,
                            std::vector<const BookmarkNode*>* nodes);

  void Undo();
    
  const BookmarkNode* GetBookmarksMobileFolder() const;

 private:
  bool IsEditable(const BookmarkNode* node) const;
  BookmarkModel* model_;  // not owned
  BookmarkUndoService* bookmark_undo_service_;  // not owned

  DISALLOW_COPY_AND_ASSIGN(BookmarksAPI);
};

}  // namespace bookmarks

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_

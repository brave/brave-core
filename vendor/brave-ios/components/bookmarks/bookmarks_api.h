/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_
#define BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "url/gurl.h"
#include "components/bookmarks/browser/bookmark_client.h"

class BookmarkUndoService;
class PrefService;

namespace sync_bookmarks {
class BookmarkSyncService;
}

namespace web {
class URLDataManagerIOSBackend;
}

namespace bookmarks {

class BookmarkClient;
class BookmarkModel;
class BookmarkNode;

class BookmarksAPI {
 public:
  BookmarksAPI(PrefService* prefs,
               const base::FilePath& state_path,
               const scoped_refptr<base::SequencedTaskRunner>& io_task_runner,
               std::unique_ptr<BookmarkClient> client);
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

 private:
  bool IsEditable(const BookmarkNode* node) const;
  std::unique_ptr<BookmarkModel> model_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  std::unique_ptr<BookmarkUndoService> bookmark_undo_service_;
  std::unique_ptr<sync_bookmarks::BookmarkSyncService> bookmark_sync_service_;

  DISALLOW_COPY_AND_ASSIGN(BookmarksAPI);
};

}  // namespace bookmarks

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BOOKMARKS_BOOKMARKS_API_H_

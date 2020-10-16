/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_EXPORTED_BOOKMARK_ENTRY_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_EXPORTED_BOOKMARK_ENTRY_H_

#include <vector>
#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "url/gurl.h"

class ExportedBookmarkEntry;

class ExportedRootBookmarkEntry {
public:
  ExportedRootBookmarkEntry(std::unique_ptr<ExportedBookmarkEntry> root_node,
                            ExportedBookmarkEntry* bookmarks_bar_node,
                            ExportedBookmarkEntry* other_bookmarks_node,
                            ExportedBookmarkEntry* mobile_bookmarks_node);
  
  ExportedBookmarkEntry* bookmarks_bar_node() { return bookmarks_bar_node_; }
  ExportedBookmarkEntry* other_bookmarks_node() { return other_bookmarks_node_; }
  ExportedBookmarkEntry* mobile_bookmarks_node() { return mobile_bookmarks_node_; }
  ~ExportedRootBookmarkEntry();
  
private:
  std::unique_ptr<ExportedBookmarkEntry> root_node_;
  ExportedBookmarkEntry* bookmarks_bar_node_;
  ExportedBookmarkEntry* other_bookmarks_node_;
  ExportedBookmarkEntry* mobile_bookmarks_node_;
  
  DISALLOW_COPY_AND_ASSIGN(ExportedRootBookmarkEntry);
};

class ExportedBookmarkEntry {
public:
  ExportedBookmarkEntry(int64_t id, const std::string& guid, const GURL& url);
  ~ExportedBookmarkEntry();
  
  const base::string16 GetTitle() const { return title_; }
  void SetTitle(const base::string16& title);
  
  int64_t id() const { return id_; }
  void set_id(int64_t id) { id_ = id; }
  
  const std::string& guid() const { return guid_; }

  const GURL& url() const { return url_; }
  void set_url(const GURL& url) { url_ = url; }

  bookmarks::BookmarkNode::Type type() const { return type_; }

  const base::Time& date_added() const { return date_added_; }
  void set_date_added(const base::Time& date) { date_added_ = date; }

  const base::Time& date_folder_modified() const { return date_folder_modified_; }
  void set_date_folder_modified(const base::Time& date) { date_folder_modified_ = date; }

  bool is_folder() const { return type_ != bookmarks::BookmarkNode::Type::URL; }
  bool is_url() const { return type_ == bookmarks::BookmarkNode::Type::URL; }
  
  const std::vector<std::unique_ptr<ExportedBookmarkEntry>>& children() const { return children_; }
  
  ExportedBookmarkEntry* Add(std::unique_ptr<ExportedBookmarkEntry> node);
  
  static std::unique_ptr<ExportedRootBookmarkEntry> get_root_node();
  
private:
  ExportedBookmarkEntry(int64_t id,
                        bookmarks::BookmarkNode::Type type,
                        const std::string& guid,
                        const base::string16& title);
  
  static std::unique_ptr<ExportedBookmarkEntry> CreateBookmarkBar(int64_t id);
  static std::unique_ptr<ExportedBookmarkEntry> CreateOtherBookmarks(int64_t id);
  static std::unique_ptr<ExportedBookmarkEntry> CreateMobileBookmarks(int64_t id);
  
  int64_t id_;
  const std::string guid_;
  GURL url_;
  const bookmarks::BookmarkNode::Type type_;
  base::Time date_added_;
  base::Time date_folder_modified_;
  
  base::string16 title_;
  std::vector<std::unique_ptr<ExportedBookmarkEntry>> children_;
  
  DISALLOW_COPY_AND_ASSIGN(ExportedBookmarkEntry);
};

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_EXPORTED_BOOKMARK_ENTRY_H_

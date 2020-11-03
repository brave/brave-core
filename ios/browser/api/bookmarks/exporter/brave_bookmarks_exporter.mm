/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/exporter/brave_bookmarks_exporter.h"

#include <vector>
#include <functional>
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/base_paths.h"
#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/ios/browser/api/bookmarks/exporter/bookmark_html_writer.h"
#include "brave/ios/browser/api/bookmarks/exporter/exported_bookmark_entry.h"
#include "brave/ios/browser/api/bookmarks/exporter/bookmarks_encoder.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class BraveBookmarksExportObserver: public BookmarksExportObserver {
public:
  BraveBookmarksExportObserver(std::function<void(BraveBookmarksExporterState)> on_export_finished);
  void OnExportFinished(Result result) override;
    
private:
  std::function<void(BraveBookmarksExporterState)> _on_export_finished;
};

BraveBookmarksExportObserver::BraveBookmarksExportObserver(std::function<void(BraveBookmarksExporterState)> on_export_finished):
    _on_export_finished(on_export_finished) {
}

void BraveBookmarksExportObserver::OnExportFinished(Result result) {
  switch(result) {
    case Result::kSuccess:
      return _on_export_finished(BraveBookmarksExporterStateCompleted);
    case Result::kCouldNotCreateFile:
      return _on_export_finished(BraveBookmarksExporterStateErrorCreatingFile);
    case Result::kCouldNotWriteHeader:
      return _on_export_finished(BraveBookmarksExporterStateErrorWritingHeader);
    case Result::kCouldNotWriteNodes:
      return _on_export_finished(BraveBookmarksExporterStateErrorWritingNodes);
    default:
      NOTREACHED();
  }
  delete this;
}

@implementation BraveExportedBookmark
- (instancetype)initWithTitle:(NSString *)title id:(int64_t)id guid:(NSString *)guid
              url:(NSURL * _Nullable)url dateAdded:(NSDate *)dateAdded
                 dateModified:(NSDate *)dateModified
                    children:(NSArray<BraveExportedBookmark *> * _Nullable)children {
  if ((self = [super init])) {
    _title = title;
    _id = id;
    _guid = guid;
    _url = url;
    _dateAdded = dateAdded;
    _dateModified = dateModified;
    _children = children;
  }
  return self;
}

- (instancetype)initFromChromiumExportedBookmark:(const ExportedBookmarkEntry*)entry {
  if ((self = [super init])) {
    DCHECK(entry);
    
    _title = base::SysUTF16ToNSString(entry->GetTitle());
    _id = entry->id();
    _guid = base::SysUTF8ToNSString(entry->guid());
    _url = entry->url().is_empty() ? nil : net::NSURLWithGURL(entry->url());
    _dateAdded = [NSDate dateWithTimeIntervalSince1970:entry->date_added().ToDoubleT()];
    _dateModified = [NSDate dateWithTimeIntervalSince1970:entry->date_folder_modified().ToDoubleT()];
    _isFolder = entry->is_folder();
    _children = [[NSMutableArray alloc] init];
    
    if (!entry->children().empty()) {
      for (const auto& child : entry->children()) {
        [(NSMutableArray *)_children addObject:
         [[BraveExportedBookmark alloc] initFromChromiumExportedBookmark: child.get()]];
      }
    }
  }
  return self;
}

- (std::unique_ptr<ExportedBookmarkEntry>)toChromiumExportedBookmark {
  auto entry = std::make_unique<ExportedBookmarkEntry>(self.id,
                                                       base::SysNSStringToUTF8(self.guid),
                                                       self.url ? net::GURLWithNSURL(self.url) : GURL());
  entry->SetTitle(base::SysNSStringToUTF16(self.title));
  entry->set_date_added(base::Time::FromDoubleT([self.dateAdded timeIntervalSince1970]));
  entry->set_date_folder_modified(base::Time::FromDoubleT([self.dateModified timeIntervalSince1970]));
  
  for (BraveExportedBookmark* child in self.children) {
    entry->Add([child toChromiumExportedBookmark]);
  }
  return entry;
}
@end

@interface BraveBookmarksExporter()
@end

@implementation BraveBookmarksExporter

- (instancetype)init {
  if ((self = [super init])) {
      
  }
  return self;
}

- (void)exportToFile:(NSString *)filePath
       withListener:(void(^)(BraveBookmarksExporterState))listener {
  
  auto start_export = [](BraveBookmarksExporter* weak_exporter,
                         NSString *filePath,
                         std::function<void(BraveBookmarksExporterState)> listener){
    //Export cancelled as the exporter has been deallocated
    __strong BraveBookmarksExporter* exporter = weak_exporter;
    if (!exporter) {
      listener(BraveBookmarksExporterStateStarted);
      listener(BraveBookmarksExporterStateCancelled);
      return;
    }
    
    DCHECK(GetApplicationContext());
    
    base::FilePath destination_file_path =
        base::mac::NSStringToFilePath(filePath);
      
    listener(BraveBookmarksExporterStateStarted);
      
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    DCHECK(browserStateManager);
    
    ChromeBrowserState* chromeBrowserState =
        browserStateManager->GetLastUsedBrowserState();
    DCHECK(chromeBrowserState);
    
    bookmark_html_writer::WriteBookmarks(chromeBrowserState,
        destination_file_path,
        new BraveBookmarksExportObserver(listener)
    );
  };
  
  __weak BraveBookmarksExporter* weakSelf = self;
  base::PostTask(FROM_HERE,
                 {web::WebThread::UI},
                 base::BindOnce(start_export,
                                weakSelf,
                                filePath,
                                listener)
                 );
}

- (void)exportToFile:(NSString *)filePath
           bookmarks:(NSArray<BraveExportedBookmark *> *)bookmarks
              withListener:(void(^)(BraveBookmarksExporterState))listener {
  if ([bookmarks count] == 0) {
    listener(BraveBookmarksExporterStateStarted);
    listener(BraveBookmarksExporterStateCompleted);
    return;
  }
  
  auto start_export = [](BraveBookmarksExporter* weak_exporter,
                         NSString *filePath,
                         NSArray<BraveExportedBookmark *> *bookmarks,
                         std::function<void(BraveBookmarksExporterState)> listener){
    //Export cancelled as the exporter has been deallocated
    __strong BraveBookmarksExporter* exporter = weak_exporter;
    if (!exporter) {
      listener(BraveBookmarksExporterStateStarted);
      listener(BraveBookmarksExporterStateCancelled);
      return;
    }
    
    listener(BraveBookmarksExporterStateStarted);
    base::FilePath destination_file_path =
        base::mac::NSStringToFilePath(filePath);
    std::unique_ptr<ExportedRootBookmarkEntry> root_node = ExportedBookmarkEntry::get_root_node();
    for (auto& bookmark : [exporter convertToChromiumExportedBookmarks: bookmarks]) {
      //We export as the |mobile_bookmarks_node| by default.
      root_node->mobile_bookmarks_node()->Add(std::move(bookmark));
    }
    
    auto encoded_bookmarks = ios::bookmarks_encoder::EncodeBookmarks(std::move(root_node));
    bookmark_html_writer::WriteBookmarks(std::move(encoded_bookmarks),
        destination_file_path,
        new BraveBookmarksExportObserver(listener)
    );
  };
  
  __weak BraveBookmarksExporter* weakSelf = self;
  base::PostTask(FROM_HERE,
                 {base::ThreadPool(), base::MayBlock(),
                  base::TaskPriority::USER_VISIBLE,
                  base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
                 base::BindOnce(start_export,
                                weakSelf,
                                filePath,
                                bookmarks,
                                listener)
                );
}

// Converts an array of Chromium imported bookmarks to iOS exported bookmarks.
- (NSArray<BraveExportedBookmark *> *)convertToIOSExportedBookmarks:(const std::vector<ExportedBookmarkEntry>&)bookmarks {
  NSMutableArray<BraveExportedBookmark *> *results = [[NSMutableArray alloc] init];
  for (const auto& bookmark : bookmarks) {
    BraveExportedBookmark *imported_bookmark = [[BraveExportedBookmark alloc] initFromChromiumExportedBookmark:&bookmark];
    [results addObject:imported_bookmark];
  }
  return results;
}

// Converts an array of iOS exported bookmarks to Chromium imported bookmarks.
- (std::vector<std::unique_ptr<ExportedBookmarkEntry>>)
convertToChromiumExportedBookmarks:(NSArray<BraveExportedBookmark *> *)bookmarks {
  std::vector<std::unique_ptr<ExportedBookmarkEntry>> results;
  for (BraveExportedBookmark* bookmark in bookmarks) {
    results.push_back([bookmark toChromiumExportedBookmark]);
  }
  return results;
}
@end

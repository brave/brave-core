/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/bookmarks/importer/brave_bookmarks_importer.h"

#include <vector>

#include "base/apple/foundation_util.h"
#include "base/base_paths.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/ios/browser/api/bookmarks/importer/bookmark_html_reader.h"
#include "brave/ios/browser/api/bookmarks/importer/bookmarks_importer.h"
#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#import "net/base/apple/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveImportedBookmark
- (instancetype)initFromChromiumImportedBookmark:
    (const ImportedBookmarkEntry&)entry {
  if ((self = [super init])) {
    NSMutableArray<NSString*>* paths = [[NSMutableArray alloc] init];
    for (const auto& path : entry.path) {
      [paths addObject:base::SysUTF16ToNSString(path)];
    }

    _inToolbar = entry.in_toolbar;
    _isFolder = entry.is_folder;
    _url = net::NSURLWithGURL(entry.url);
    _path = paths;
    _title = base::SysUTF16ToNSString(entry.title);
    _creationTime =
        [NSDate dateWithTimeIntervalSince1970:entry.creation_time
                                                  .InSecondsFSinceUnixEpoch()];
  }
  return self;
}

- (ImportedBookmarkEntry)toChromiumImportedBookmark {
  std::vector<std::u16string> paths;
  for (NSString* path in self.path) {
    paths.push_back(base::SysNSStringToUTF16(path));
  }

  ImportedBookmarkEntry entry;
  entry.creation_time = base::Time::FromSecondsSinceUnixEpoch(
      [self.creationTime timeIntervalSince1970]);
  entry.url = net::GURLWithNSURL(self.url);
  entry.title = base::SysNSStringToUTF16(self.title);
  entry.in_toolbar = self.inToolbar;
  entry.path = paths;
  return entry;
}
@end

@interface BraveBookmarksImporter () {
  scoped_refptr<base::SequencedTaskRunner> import_thread_;
}
@property(atomic) bool cancelled;  // atomic
@end

@implementation BraveBookmarksImporter
- (instancetype)init {
  if ((self = [super init])) {
    self.cancelled = false;

    import_thread_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  }
  return self;
}

- (void)dealloc {
  [self cancel];
}

- (void)cancel {
  self.cancelled = true;
}

- (void)importFromFile:(NSString*)filePath
    topLevelFolderName:(NSString*)folderName
       automaticImport:(bool)automaticImport
          withListener:
              (void (^)(BraveBookmarksImporterState,
                        NSArray<BraveImportedBookmark*>* _Nullable))listener {
  base::FilePath source_file_path = base::apple::NSStringToFilePath(filePath);

  // In Chromium, this is IDS_BOOKMARK_GROUP (804)
  std::u16string top_level_folder_name = base::SysNSStringToUTF16(folderName);

  auto start_import = [](BraveBookmarksImporter* weak_importer,
                         const base::FilePath& source_file_path,
                         const std::u16string& top_level_folder_name,
                         bool automaticImport,
                         std::function<void(BraveBookmarksImporterState,
                                            NSArray<BraveImportedBookmark*>*)>
                             listener) {
    // Import cancelled as the importer has been deallocated
    __strong BraveBookmarksImporter* importer = weak_importer;
    if (!importer) {
      listener(BraveBookmarksImporterStateStarted, nullptr);
      listener(BraveBookmarksImporterStateCancelled, nullptr);
      return;
    }

    listener(BraveBookmarksImporterStateStarted, nullptr);
    std::vector<ImportedBookmarkEntry> bookmarks;
    bookmark_html_reader::ImportBookmarksFile(
        base::BindRepeating(
            [](BraveBookmarksImporter* importer) -> bool {
              return [importer isImporterCancelled];
            },
            base::Unretained(importer)),
        base::BindRepeating(
            [](BraveBookmarksImporter* importer, const GURL& url) -> bool {
              return [importer canImportURL:url];
            },
            base::Unretained(importer)),
        source_file_path, &bookmarks, nullptr);

    if (!bookmarks.empty() && ![importer isImporterCancelled]) {
      if (automaticImport) {
        auto complete_import =
            [](std::vector<ImportedBookmarkEntry> bookmarks,
               const std::u16string& top_level_folder_name,
               std::function<void(BraveBookmarksImporterState,
                                  NSArray<BraveImportedBookmark*>*)> listener) {
              BookmarksImporter::AddBookmarks(top_level_folder_name, bookmarks);
              listener(BraveBookmarksImporterStateAutoCompleted, nullptr);
            };

        // Import into the Profile/ProfileIOS on the main-thread.
        web::GetUIThreadTaskRunner({})->PostTask(
            FROM_HERE, base::BindOnce(complete_import, std::move(bookmarks),
                                      top_level_folder_name, listener));
      } else {
        listener(BraveBookmarksImporterStateCompleted,
                 [importer convertToIOSImportedBookmarks:bookmarks]);
      }
    } else {
      listener(BraveBookmarksImporterStateCancelled, nullptr);
    }
  };

  // Run the importer on the sequenced task runner.
  __weak BraveBookmarksImporter* weakSelf = self;
  import_thread_->PostTask(
      FROM_HERE,
      base::BindOnce(start_import, weakSelf, source_file_path,
                     top_level_folder_name, automaticImport, listener));
}

- (void)importFromArray:(NSArray<BraveImportedBookmark*>*)bookmarks
     topLevelFolderName:(NSString*)folderName
           withListener:(void (^)(BraveBookmarksImporterState))listener {
  // In Chromium, this is IDS_BOOKMARK_GROUP (804)
  std::u16string top_level_folder_name = base::SysNSStringToUTF16(folderName);

  auto start_import =
      [](BraveBookmarksImporter* weak_importer,
         NSArray<BraveImportedBookmark*>* bookmarks,
         const std::u16string& top_level_folder_name,
         std::function<void(BraveBookmarksImporterState)> listener) {
        // Import cancelled as the importer has been deallocated
        __strong BraveBookmarksImporter* importer = weak_importer;
        if (!importer) {
          listener(BraveBookmarksImporterStateStarted);
          listener(BraveBookmarksImporterStateCancelled);
          return;
        }

        listener(BraveBookmarksImporterStateStarted);
        BookmarksImporter::AddBookmarks(
            top_level_folder_name,
            [importer convertToChromiumImportedBookmarks:bookmarks]);
        listener(BraveBookmarksImporterStateCompleted);
      };

  // Import into the Profile/ProfileIOS on the main-thread.
  __weak BraveBookmarksImporter* weakSelf = self;
  import_thread_->PostTask(FROM_HERE,
                           base::BindOnce(start_import, weakSelf, bookmarks,
                                          top_level_folder_name, listener));
}

// MARK: - Private

- (bool)isImporterCancelled {
  return self.cancelled;
}

// Returns true if |url| has a valid scheme that we allow to import. We
// filter out the URL with a unsupported scheme.
- (bool)canImportURL:(const GURL&)url {
  // The URL is not valid.
  if (!url.is_valid()) {
    return false;
  }

  // Filter out the URLs with unsupported schemes.
  const char* const kInvalidSchemes[] = {"wyciwyg", "place", "about", "chrome"};
  for (size_t i = 0; i < std::size(kInvalidSchemes); ++i) {
    if (UNSAFE_TODO(url.SchemeIs(kInvalidSchemes[i]))) {
      return false;
    }
  }

  return true;
}

// Converts an array of Chromium imported bookmarks to iOS imported bookmarks.
- (NSArray<BraveImportedBookmark*>*)convertToIOSImportedBookmarks:
    (const std::vector<ImportedBookmarkEntry>&)bookmarks {
  NSMutableArray<BraveImportedBookmark*>* results =
      [[NSMutableArray alloc] init];
  for (const auto& bookmark : bookmarks) {
    BraveImportedBookmark* imported_bookmark = [[BraveImportedBookmark alloc]
        initFromChromiumImportedBookmark:bookmark];
    [results addObject:imported_bookmark];
  }
  return results;
}

// Converts an array of iOS imported bookmarks to Chromium imported bookmarks.
- (std::vector<ImportedBookmarkEntry>)convertToChromiumImportedBookmarks:
    (NSArray<BraveImportedBookmark*>*)bookmarks {
  std::vector<ImportedBookmarkEntry> results;
  for (BraveImportedBookmark* bookmark in bookmarks) {
    results.push_back([bookmark toChromiumImportedBookmark]);
  }
  return results;
}
@end

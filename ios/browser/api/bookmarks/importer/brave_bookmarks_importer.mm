#include "brave/ios/browser/api/bookmarks/importer/brave_bookmarks_importer.h"

#include <vector>
#include "brave/ios/browser/api/bookmarks/importer/bookmarks_importer.h"
#include "brave/ios/browser/api/bookmarks/importer/bookmark_html_reader.h"
#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

@implementation BraveImportedBookmark
- (instancetype)initFromChromiumImportedBookmark:(const ImportedBookmarkEntry&)entry {
  if ((self = [super init])) {
    NSMutableArray<NSString *> *paths = [[NSMutableArray alloc] init];
    for (const auto& path : entry.path) {
      [paths addObject:base::SysUTF16ToNSString(path)];
    }
      
    _inToolbar = entry.in_toolbar;
    _isFolder = entry.is_folder;
    _url = net::NSURLWithGURL(entry.url);
    _path = paths;
    _title = base::SysUTF16ToNSString(entry.title);
    _creationTime = [NSDate dateWithTimeIntervalSince1970:entry.creation_time.ToDoubleT()];
  }
  return self;
}

- (ImportedBookmarkEntry)toChromiumImportedBookmark {
  std::vector<base::string16> paths;
  for (NSString* path in self.path) {
    paths.push_back(base::SysNSStringToUTF16(path));
  }

  ImportedBookmarkEntry entry;
  entry.creation_time = base::Time::FromDoubleT([self.creationTime timeIntervalSince1970]);
  entry.url = net::GURLWithNSURL(self.url);
  entry.title = base::SysNSStringToUTF16(self.title);
  entry.in_toolbar = self.inToolbar;
  entry.path = paths;
  return entry;
}
@end

@interface BraveBookmarksImporter()
@property (atomic) bool cancelled; // atomic
@end

@implementation BraveBookmarksImporter
- (instancetype)init {
    if ((self = [super init])) {
      self.cancelled = false;
    }
    return self;
}

- (void)cancel {
  self.cancelled = true;
}

- (void)importFromFile:(NSString *)filePath
       automaticImport:(bool)automaticImport
          withListener:(void(^)(BraveBookmarksImporterState, NSArray<BraveImportedBookmark *> * _Nullable))listener {
  base::FilePath source_file_path =
      base::FilePath::FromUTF8Unsafe([filePath UTF8String]);
    
  listener(BraveBookmarksImporterStateStarted, nullptr);
    
  std::vector<ImportedBookmarkEntry> bookmarks;
  bookmark_html_reader::ImportBookmarksFile(
      base::BindRepeating([](BraveBookmarksImporter* importer) -> bool {
          return [importer isImporterCancelled];
      }, base::Unretained(self)),
      base::BindRepeating([](BraveBookmarksImporter* importer, const GURL& url) -> bool {
          return [importer canImportURL:url];
      }, base::Unretained(self)), source_file_path,
      &bookmarks, nullptr);
    
  if (!bookmarks.empty() && ![self isImporterCancelled]) {
    if (automaticImport) {
      BookmarksImporter::AddBookmarks(bookmarks);
      listener(BraveBookmarksImporterStateAutoCompleted, nullptr);
    } else {
      listener(BraveBookmarksImporterStateCompleted, [self convertToIOSImportedBookmarks:bookmarks]);
    }
  } else {
    listener(BraveBookmarksImporterStateCancelled, nullptr);
  }
}

- (void)importFromArray:(NSArray<BraveImportedBookmark *> *)bookmarks {
  BookmarksImporter::AddBookmarks([self convertToChromiumImportedBookmarks:bookmarks]);
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
    for (size_t i = 0; i < base::size(kInvalidSchemes); ++i) {
      if (url.SchemeIs(kInvalidSchemes[i])) {
        return false;
      }
    }

    return true;
}

// Converts an array of Chromium imported bookmarks to iOS imported bookmarks.
- (NSArray<BraveImportedBookmark *> *)convertToIOSImportedBookmarks:(const std::vector<ImportedBookmarkEntry>&)bookmarks {
  NSMutableArray<BraveImportedBookmark *> *results = [[NSMutableArray alloc] init];
  for (const auto& bookmark : bookmarks) {
    BraveImportedBookmark *imported_bookmark = [[BraveImportedBookmark alloc] initFromChromiumImportedBookmark:bookmark];
    [results addObject:imported_bookmark];
  }
  return results;
}

// Converts an array of iOS imported bookmarks to Chromium imported bookmarks.
- (std::vector<ImportedBookmarkEntry>)convertToChromiumImportedBookmarks:(NSArray<BraveImportedBookmark *> *)bookmarks {
  std::vector<ImportedBookmarkEntry> results;
  for (BraveImportedBookmark* bookmark in bookmarks) {
    results.push_back([bookmark toChromiumImportedBookmark]);
  }
  return results;
}
@end

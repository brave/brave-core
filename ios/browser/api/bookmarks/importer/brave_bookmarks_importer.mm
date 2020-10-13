#include "brave/ios/browser/api/bookmarks/importer/brave_bookmarks_importer.h"

#include <vector>
#include "brave/ios/browser/api/bookmarks/importer/bookmark_html_reader.h"
#include "brave/ios/browser/api/bookmarks/importer/imported_bookmark_entry.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/base_paths.h"
#include "base/path_service.h"

@interface BraveBookmarksImporter()
@end

@implementation BraveBookmarksImporter
- (instancetype)init {
    if ((self = [super init])) {
        
    }
    return self;
}

- (bool)isImporterCancelled {
    return false;
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

- (void)importFromFile:(NSString *)filePath {
    
//    base::FilePath source_file_path;
//    base::PathService::Get(base::DIR_APP_DATA, &source_file_path);
//    source_file_path = source_file_path.AppendASCII("bookmarks.html");
    
      base::FilePath source_file_path =
          base::FilePath::FromUTF8Unsafe([filePath UTF8String]);
    
//      bridge->NotifyStarted();
//      bridge->NotifyItemStarted(importer::FAVORITES);

      std::vector<ImportedBookmarkEntry> bookmarks;

      bookmark_html_reader::ImportBookmarksFile(
          base::BindRepeating([](BraveBookmarksImporter* importer) -> bool {
              return [importer isImporterCancelled];
          }, base::Unretained(self)),
          base::BindRepeating([](BraveBookmarksImporter* importer, const GURL& url) -> bool {
              return [importer canImportURL:url];
          }, base::Unretained(self)), source_file_path,
          &bookmarks, nullptr);

//      if (!bookmarks.empty() && !cancelled()) {
//        base::string16 first_folder_name =
//            bridge->GetLocalizedString(IDS_BOOKMARK_GROUP);
//        bridge->AddBookmarks(bookmarks, first_folder_name);
//      }

//      bridge->NotifyItemEnded(importer::FAVORITES);
//      bridge->NotifyEnded();
}
@end

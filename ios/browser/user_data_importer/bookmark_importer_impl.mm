// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/user_data_importer/bookmark_importer_impl.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "components/user_data_importer/ios/ios_bookmark_parser.h"
#include "components/user_data_importer/utility/bookmark_util.h"
#include "ui/base/l10n/l10n_util.h"

static const NSErrorDomain kBookmarkParserErrorDomain =
    @"BookmarkParserErrorDomain";

@implementation BookmarkImporterImpl {
  std::unique_ptr<user_data_importer::IOSBookmarkParser> _parser;
  raw_ptr<bookmarks::BookmarkModel> _bookmarkModel;
}

- (instancetype)initWithBookmarkModel:(bookmarks::BookmarkModel*)bookmarkModel {
  if ((self = [super init])) {
    _parser = std::make_unique<user_data_importer::IOSBookmarkParser>();
    _bookmarkModel = bookmarkModel;
  }
  return self;
}

- (void)importBookmarksForFileAtPath:(NSString*)path
                          completion:(void (^)(NSInteger importedBookmarksCount,
                                               NSError* error))completion {
  base::FilePath filePath(base::SysNSStringToUTF8(path));
  __weak BookmarkImporterImpl* weakSelf = self;
  _parser->Parse(
      filePath,
      base::BindOnce(^(base::expected<
                       user_data_importer::BookmarkParser::ParsedBookmarks,
                       user_data_importer::BookmarkParser::BookmarkParsingError>
                           result) {
        BookmarkImporterImpl* strongSelf = weakSelf;
        if (!strongSelf) {
          completion(0, nil);
          return;
        }
        if (!result.has_value()) {
          NSError* error =
              [NSError errorWithDomain:kBookmarkParserErrorDomain
                                  code:static_cast<NSInteger>(result.error())
                              userInfo:nil];
          completion(0, error);
          return;
        }
        const size_t count = user_data_importer::ImportBookmarks(
            strongSelf->_bookmarkModel, std::move(result->bookmarks),
            l10n_util::GetStringUTF16(IDS_IMPORTED_FOLDER));
        completion(count, nil);
      }));
}

@end

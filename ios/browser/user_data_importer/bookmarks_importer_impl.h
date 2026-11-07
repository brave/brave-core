// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARKS_IMPORTER_IMPL_H_
#define BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARKS_IMPORTER_IMPL_H_

#include <Foundation/Foundation.h>

#include "brave/ios/browser/user_data_importer/bookmarks_importer.h"

namespace bookmarks {
class BookmarkModel;
}

NS_ASSUME_NONNULL_BEGIN

/// A specialized BookmarkImporter implementation which parses the bookmarks
/// HTML and imports the bookmarks into the provided BookmarkModel under the
/// standard "Imported" folder name
@interface BookmarksImporterImpl : NSObject <BookmarkImporter>
- (instancetype)initWithBookmarkModel:(bookmarks::BookmarkModel*)bookmarkModel;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARKS_IMPORTER_IMPL_H_

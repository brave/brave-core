/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api.h"

NS_ASSUME_NONNULL_BEGIN

namespace bookmarks {
class BookmarkModel;
}

class BookmarkUndoService;

@interface BraveBookmarksAPI (Private)
- (instancetype)initWithBookmarkModel:(bookmarks::BookmarkModel*)bookmarkModel
                  bookmarkUndoService:(BookmarkUndoService*)bookmarkUndoService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_BRAVE_BOOKMARKS_API_PRIVATE_H_

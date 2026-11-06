// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARK_IMPORTER_H_
#define BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARK_IMPORTER_H_

#include <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol ProfileBridge;

@protocol BookmarkImporter
/// Parse a bookmarks HTML file securely and imports those bookmarks into the
/// users bookmarks list.
///
/// Runs the completion handler with the number of bookmarks imported from the
/// file or an error if the parsing failed or the importer was destroyed before
/// parsing completed
- (void)importBookmarksForFileAtPath:(NSString*)path
                          completion:
                              (void (^)(NSInteger importedBookmarksCount,
                                        NSError* _Nullable error))completion;
@end

/// A lightweight factory that can vend a BookmarkImporter for the given
/// profile
NS_SWIFT_UI_ACTOR
OBJC_EXPORT
@interface BookmarkImporterFactory : NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (id<BookmarkImporter>)getForProfile:(id<ProfileBridge>)profile
    NS_SWIFT_NAME(get(profile:));
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_USER_DATA_IMPORTER_BOOKMARK_IMPORTER_H_

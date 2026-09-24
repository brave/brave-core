/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BRAVE_BOOKMARKS_EXPORTER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BRAVE_BOOKMARKS_EXPORTER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BraveBookmarksExporterState) {
  BraveBookmarksExporterStateCompleted,
  BraveBookmarksExporterStateStarted,
  BraveBookmarksExporterStateCancelled,
  BraveBookmarksExporterStateErrorCreatingFile,
  BraveBookmarksExporterStateErrorWritingHeader,
  BraveBookmarksExporterStateErrorWritingNodes
};

@class IOSBookmarkNode;

OBJC_EXPORT
@interface BraveBookmarksExporter : NSObject
- (instancetype)init;

- (void)exportToFile:(NSString*)filePath
        withListener:(void (^)(BraveBookmarksExporterState))listener;

- (void)exportToFile:(NSString*)filePath
           bookmarks:(NSArray<IOSBookmarkNode*>*)bookmarks
        withListener:(void (^)(BraveBookmarksExporterState))listener;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BRAVE_BOOKMARKS_EXPORTER_H_

/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BRAVE_BOOKMARKS_EXPORTER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BRAVE_BOOKMARKS_EXPORTER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BraveBookmarksExporterState) {
  BraveBookmarksExporterStateCompleted,
  BraveBookmarksExporterStateStarted,
  BraveBookmarksExporterStateErrorCreatingFile,
  BraveBookmarksExporterStateErrorWritingHeader,
  BraveBookmarksExporterStateErrorWritingNodes
};

OBJC_EXPORT
@interface BraveExportedBookmark: NSObject
@property (nonatomic, readonly, copy) NSString *title;
@property (nonatomic, readonly) int64_t id;
@property (nonatomic, readonly, copy) NSString *guid;
@property (nullable, nonatomic, readonly, copy) NSURL *url;
@property (nonatomic, readonly, copy) NSDate *dateAdded;
@property (nonatomic, readonly, copy) NSDate *dateModified;
@property (nonatomic, readonly) bool isFolder;

@property (nullable, nonatomic, readonly, copy) NSArray<BraveExportedBookmark *> *children;
@end

OBJC_EXPORT
@interface BraveBookmarksExporter: NSObject
- (instancetype)init;

- (void)exportToFile:(NSString *)filePath
       withListener:(void(^)(BraveBookmarksExporterState))listener;

- (void)exportToFile:(NSString *)filePath
           bookmarks:(NSArray<BraveExportedBookmark *> *)bookmarks
              withListener:(void(^)(BraveBookmarksExporterState))listener;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_IMPORTER_BRAVE_BOOKMARKS_EXPORTER_H_


/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_BRAVE_HISTORY_IMPORTER_H_
#define BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_BRAVE_HISTORY_IMPORTER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, BraveHistoryImporterState) {
  BraveHistoryImporterStateCompleted,
  BraveHistoryImporterStateAutoCompleted,
  BraveHistoryImporterStateStarted,
  BraveHistoryImporterStateCancelled
};

OBJC_EXPORT
@interface BraveImportedHistory : NSObject
@property(nonatomic, readonly) NSURL* url;
@property(nonatomic, readonly) NSString* title;
@property(nonatomic, readonly) NSInteger visitCount;
@property(nonatomic, readonly) NSDate* lastVisitDate;
@end

OBJC_EXPORT
@interface BraveHistoryImporter : NSObject
- (instancetype)init;

- (void)cancel;

- (void)importFromFile:(NSString*)filePath
       automaticImport:(bool)automaticImport
          withListener:
              (void (^)(BraveHistoryImporterState,
                        NSArray<BraveImportedHistory*>* _Nullable))listener;

- (void)importFromArray:(NSArray<BraveImportedHistory*>*)historyItems
           withListener:(void (^)(BraveHistoryImporterState))listener;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HISTORY_IMPORTER_BRAVE_HISTORY_IMPORTER_H_

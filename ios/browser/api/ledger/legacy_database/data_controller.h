/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LEDGER_LEGACY_DATABASE_DATA_CONTROLLER_H_
#define BRAVE_IOS_BROWSER_API_LEDGER_LEGACY_DATABASE_DATA_CONTROLLER_H_

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface DataController : NSObject

+ (BOOL)defaultStoreExists;

@property(nonatomic, class) DataController* shared;

/// File URL to the folder containing all data files
@property(nonatomic, readonly) NSURL* storeDirectoryURL;
/// File URL to the SQLite store
@property(nonatomic, readonly) NSURL* storeURL;

- (void)addPersistentStoreForContainer:(NSPersistentContainer*)container;

@property(nonatomic, readonly) NSPersistentContainer* container;

/// Context object also allows us access to all persistent container data if
/// needed.
+ (NSManagedObjectContext*)viewContext;

+ (NSManagedObjectContext*)newBackgroundContext;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LEDGER_LEGACY_DATABASE_DATA_CONTROLLER_H_

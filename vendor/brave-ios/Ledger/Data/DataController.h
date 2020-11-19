/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface DataController : NSObject

+ (BOOL)defaultStoreExists;

@property (nonatomic, class) DataController *shared;

/// File URL to the folder containing all data files
@property (nonatomic, readonly) NSURL *storeDirectoryURL;
/// File URL to the SQLite store
@property (nonatomic, readonly) NSURL *storeURL;

- (void)addPersistentStoreForContainer:(NSPersistentContainer *)container;

@property (nonatomic, readonly) NSPersistentContainer *container;

/// Context object also allows us access to all persistent container data if needed.
+ (NSManagedObjectContext *)viewContext;

+ (NSManagedObjectContext *)newBackgroundContext;

@end

NS_ASSUME_NONNULL_END

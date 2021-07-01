/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <CoreData/CoreData.h>

@class ServerPublisherInfo;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface ServerPublisherBanner : NSManagedObject

+ (NSFetchRequest<ServerPublisherBanner*>*)fetchRequest;

@property(nonatomic, copy) NSString* publisherID;
@property(nullable, nonatomic, copy) NSString* title;
@property(nullable, nonatomic, copy) NSString* desc;
@property(nullable, nonatomic, copy) NSString* background;
@property(nullable, nonatomic, copy) NSString* logo;
@property(nullable, nonatomic, retain) ServerPublisherInfo* serverPublisherInfo;

@end

NS_ASSUME_NONNULL_END

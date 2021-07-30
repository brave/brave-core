/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <CoreData/CoreData.h>

@class ServerPublisherBanner, ServerPublisherAmount, ServerPublisherLink;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface ServerPublisherInfo : NSManagedObject

+ (NSFetchRequest<ServerPublisherInfo*>*)fetchRequest;

@property(nonatomic, copy) NSString* publisherID;
@property(nonatomic) int32_t status;
@property(nonatomic) BOOL excluded;
@property(nonatomic, copy) NSString* address;
@property(nullable, nonatomic, retain) ServerPublisherBanner* banner;
@property(nullable, nonatomic, retain) NSSet<ServerPublisherAmount*>* amounts;
@property(nullable, nonatomic, retain) NSSet<ServerPublisherLink*>* links;

@end

@interface ServerPublisherInfo (CoreDataGeneratedAccessors)
- (void)addAmountsObject:(ServerPublisherAmount*)value;
- (void)removeAmountsObject:(ServerPublisherAmount*)value;
- (void)addAmounts:(NSSet<ServerPublisherAmount*>*)values;
- (void)removeAmounts:(NSSet<ServerPublisherAmount*>*)values;

- (void)addLinksObject:(ServerPublisherLink*)value;
- (void)removeLinksObject:(ServerPublisherLink*)value;
- (void)addLinks:(NSSet<ServerPublisherLink*>*)values;
- (void)removeLinks:(NSSet<ServerPublisherLink*>*)values;
@end

NS_ASSUME_NONNULL_END

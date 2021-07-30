/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <CoreData/CoreData.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface MediaPublisherInfo : NSManagedObject

+ (NSFetchRequest<MediaPublisherInfo*>*)fetchRequest;

@property(nonatomic, copy) NSString* mediaKey;
@property(nonatomic, copy) NSString* publisherID;

@end

NS_ASSUME_NONNULL_END

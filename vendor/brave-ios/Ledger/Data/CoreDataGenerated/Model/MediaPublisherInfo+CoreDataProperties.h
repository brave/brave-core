//
//  MediaPublisherInfo+CoreDataProperties.h
//  
//
//  Created by Kyle Hickinson on 2019-05-24.
//
//  This file was automatically generated and should not be edited.
//

#import "MediaPublisherInfo+CoreDataClass.h"


NS_ASSUME_NONNULL_BEGIN

@interface MediaPublisherInfo (CoreDataProperties)

+ (NSFetchRequest<MediaPublisherInfo *> *)fetchRequest;

@property (nullable, nonatomic, copy) NSString *mediaKey;
@property (nullable, nonatomic, copy) NSString *publisherID;

@end

NS_ASSUME_NONNULL_END

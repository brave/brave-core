/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ServerPublisherBanner.h"

@implementation ServerPublisherBanner

+ (NSFetchRequest<ServerPublisherBanner*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ServerPublisherBanner"];
}

@dynamic publisherID;
@dynamic title;
@dynamic desc;
@dynamic background;
@dynamic logo;
@dynamic serverPublisherInfo;

@end

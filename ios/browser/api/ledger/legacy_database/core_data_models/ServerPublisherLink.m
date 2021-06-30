/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ServerPublisherLink.h"

@implementation ServerPublisherLink

+ (NSFetchRequest<ServerPublisherLink*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ServerPublisherLink"];
}

@dynamic publisherID;
@dynamic provider;
@dynamic link;
@dynamic serverPublisherInfo;

@end

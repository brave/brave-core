/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ServerPublisherInfo.h"

@implementation ServerPublisherInfo

+ (NSFetchRequest<ServerPublisherInfo*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ServerPublisherInfo"];
}

@dynamic publisherID;
@dynamic status;
@dynamic excluded;
@dynamic address;
@dynamic banner;
@dynamic amounts;
@dynamic links;

@end

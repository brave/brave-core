/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ActivityInfo.h"

@implementation ActivityInfo

+ (NSFetchRequest<ActivityInfo*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ActivityInfo"];
}

@dynamic duration;
@dynamic percent;
@dynamic publisherID;
@dynamic reconcileStamp;
@dynamic score;
@dynamic visits;
@dynamic weight;
@dynamic publisher;

@end

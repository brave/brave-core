/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ContributionQueue.h"

@implementation ContributionQueue

+ (NSFetchRequest<ContributionQueue*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ContributionQueue"];
}

@dynamic id;
@dynamic type;
@dynamic amount;
@dynamic partial;
@dynamic publishers;

@end

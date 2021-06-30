/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "PendingContribution.h"

@implementation PendingContribution

+ (NSFetchRequest<PendingContribution*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"PendingContribution"];
}

@dynamic addedDate;
@dynamic amount;
@dynamic type;
@dynamic publisherID;
@dynamic viewingID;
@dynamic publisher;

@end

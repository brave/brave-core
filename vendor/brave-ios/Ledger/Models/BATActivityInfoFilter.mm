/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATActivityInfoFilter.h"
#import "BATActivityInfoFilter+Private.h"
#import "CppTransformations.h"

@implementation BATActivityInfoFilterOrderPair

- (instancetype)initWithStringBoolPair:(std::pair<std::string, bool>)obj
{
  if ((self = [super init])) {
    self.propertyName = [NSString stringWithUTF8String:obj.first.c_str()];
    self.ascending = obj.second;
  }
  return self;
}

- (std::pair<std::string, bool>)cppObj
{
  return std::pair<std::string, bool>(std::string(self.propertyName.UTF8String), self.ascending);
}

@end

@implementation BATActivityInfoFilter

- (instancetype)initWithActivityInfoFilter:(const ledger::ActivityInfoFilter&)obj
{
  if ((self = [super init])) {
    self.id = [NSString stringWithUTF8String:obj.id.c_str()];
    self.excluded = (BATExcludeFilter)obj.excluded;
    self.orderBy = NSArrayFromVector(obj.order_by, ^BATActivityInfoFilterOrderPair *(const std::pair<std::string, bool>& o){
      return [[BATActivityInfoFilterOrderPair alloc] initWithStringBoolPair:o];
    });
    self.minDuration = obj.min_duration;
    self.reconcileStamp = obj.reconcile_stamp;
    self.nonVerified = obj.non_verified;
    self.minVisits = obj.min_visits;
  }
  return self;
}

- (ledger::ActivityInfoFilter)cppObj
{
  ledger::ActivityInfoFilter obj;
  obj.id = std::string(self.id.UTF8String);
  obj.excluded = (ledger::EXCLUDE_FILTER)self.excluded;
  obj.order_by = VectorFromNSArray(self.orderBy, ^std::pair<std::string, bool>(BATActivityInfoFilterOrderPair *o){
    return [o cppObj];
  });
  obj.min_duration = self.minDuration;
  obj.reconcile_stamp = self.reconcileStamp;
  obj.non_verified = self.nonVerified;
  obj.min_visits = obj.min_visits;
  return obj;
}

@end

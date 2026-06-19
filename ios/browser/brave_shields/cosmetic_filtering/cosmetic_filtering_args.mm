// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_args.h"

@implementation CosmeticFilteringArgs

+ (BOOL)supportsSecureCoding {
  return YES;
}

- (instancetype)
        initWithHideFirstPartyContent:(BOOL)hideFirstPartyContent
                          genericHide:(BOOL)genericHide
         firstSelectorsPollingDelayMs:
             (nullable NSNumber*)firstSelectorsPollingDelayMs
    switchToSelectorsPollingThreshold:
        (nullable NSNumber*)switchToSelectorsPollingThreshold
     fetchNewClassIdRulesThrottlingMs:
         (nullable NSNumber*)fetchNewClassIdRulesThrottlingMs
                  aggressiveSelectors:(NSSet<NSString*>*)aggressiveSelectors
                    standardSelectors:(NSSet<NSString*>*)standardSelectors
                    proceduralFilters:(NSSet<NSString*>*)proceduralFilters {
  self = [super init];
  if (self) {
    _hideFirstPartyContent = hideFirstPartyContent;
    _genericHide = genericHide;
    _firstSelectorsPollingDelayMs = firstSelectorsPollingDelayMs;
    _switchToSelectorsPollingThreshold = switchToSelectorsPollingThreshold;
    _fetchNewClassIdRulesThrottlingMs = fetchNewClassIdRulesThrottlingMs;
    _aggressiveSelectors = [aggressiveSelectors copy];
    _standardSelectors = [standardSelectors copy];
    _proceduralFilters = [proceduralFilters copy];
  }
  return self;
}

- (id)copyWithZone:(NSZone*)zone {
  return [[CosmeticFilteringArgs alloc]
          initWithHideFirstPartyContent:self.hideFirstPartyContent
                            genericHide:self.genericHide
           firstSelectorsPollingDelayMs:self.firstSelectorsPollingDelayMs
      switchToSelectorsPollingThreshold:self.switchToSelectorsPollingThreshold
       fetchNewClassIdRulesThrottlingMs:self.fetchNewClassIdRulesThrottlingMs
                    aggressiveSelectors:self.aggressiveSelectors
                      standardSelectors:self.standardSelectors
                      proceduralFilters:self.proceduralFilters];
}

- (void)encodeWithCoder:(NSCoder*)coder {
  [coder encodeBool:self.hideFirstPartyContent forKey:@"hideFirstPartyContent"];
  [coder encodeBool:self.genericHide forKey:@"genericHide"];
  [coder encodeObject:self.firstSelectorsPollingDelayMs
               forKey:@"firstSelectorsPollingDelayMs"];
  [coder encodeObject:self.switchToSelectorsPollingThreshold
               forKey:@"switchToSelectorsPollingThreshold"];
  [coder encodeObject:self.fetchNewClassIdRulesThrottlingMs
               forKey:@"fetchNewClassIdRulesThrottlingMs"];
  [coder encodeObject:self.aggressiveSelectors forKey:@"aggressiveSelectors"];
  [coder encodeObject:self.standardSelectors forKey:@"standardSelectors"];
  [coder encodeObject:self.proceduralFilters forKey:@"proceduralFilters"];
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  self = [super init];
  if (self) {
    _hideFirstPartyContent = [coder decodeBoolForKey:@"hideFirstPartyContent"];
    _genericHide = [coder decodeBoolForKey:@"genericHide"];
    _firstSelectorsPollingDelayMs =
        [coder decodeObjectForKey:@"firstSelectorsPollingDelayMs"];
    _switchToSelectorsPollingThreshold =
        [coder decodeObjectForKey:@"switchToSelectorsPollingThreshold"];
    _fetchNewClassIdRulesThrottlingMs =
        [coder decodeObjectForKey:@"fetchNewClassIdRulesThrottlingMs"];
    _aggressiveSelectors =
        [[coder decodeObjectForKey:@"aggressiveSelectors"] copy];
    _standardSelectors = [[coder decodeObjectForKey:@"standardSelectors"] copy];
    _proceduralFilters = [[coder decodeObjectForKey:@"proceduralFilters"] copy];
  }
  return self;
}

- (NSUInteger)hash {
  NSUInteger hash = self.hideFirstPartyContent;
  hash = hash * 31 + self.genericHide;
  hash = hash * 31 + [self.firstSelectorsPollingDelayMs hash];
  hash = hash * 31 + [self.switchToSelectorsPollingThreshold hash];
  hash = hash * 31 + [self.fetchNewClassIdRulesThrottlingMs hash];
  hash = hash * 31 + [self.aggressiveSelectors hash];
  hash = hash * 31 + [self.standardSelectors hash];
  hash = hash * 31 + [self.proceduralFilters hash];
  return hash;
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }
  if (![object isKindOfClass:[CosmeticFilteringArgs class]]) {
    return NO;
  }

  CosmeticFilteringArgs* other = (CosmeticFilteringArgs*)object;
  return self.hideFirstPartyContent == other.hideFirstPartyContent &&
         self.genericHide == other.genericHide &&
         [self.firstSelectorsPollingDelayMs
             isEqual:other.firstSelectorsPollingDelayMs] &&
         [self.switchToSelectorsPollingThreshold
             isEqual:other.switchToSelectorsPollingThreshold] &&
         [self.fetchNewClassIdRulesThrottlingMs
             isEqual:other.fetchNewClassIdRulesThrottlingMs] &&
         [self.aggressiveSelectors isEqual:other.aggressiveSelectors] &&
         [self.standardSelectors isEqual:other.standardSelectors] &&
         [self.proceduralFilters isEqual:other.proceduralFilters];
}

@end

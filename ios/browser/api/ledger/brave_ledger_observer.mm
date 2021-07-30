/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave_ledger_observer.h"
#import "brave_ledger.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BraveLedgerObserver ()
@property(nonatomic, weak) BraveLedger* ledger;
@end

@implementation BraveLedgerObserver

- (instancetype)initWithLedger:(BraveLedger*)ledger {
  if ((self = [super init])) {
    self.ledger = ledger;
  }
  return self;
}

@end

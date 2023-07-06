/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "chrome/browser/app_controller_mac.h"

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

// Calling [self init] seems to invoke BraveAppController:init, which will end
// in a inifinite loop. So we should specify the exact class implmeneation.
#define dealloc                                                                \
  dummy{} - (instancetype)initForBrave {                                       \
    IMP imp =                                                                  \
        class_getMethodImplementation([AppController class], @selector(init)); \
    id (*callableImp)(id, SEL) = (__typeof__(callableImp))imp;                 \
    return callableImp(self, @selector(init));                                 \
  }                                                                            \
  -(void)dealloc

#include "src/chrome/browser/app_controller_mac.mm"
#undef dealloc

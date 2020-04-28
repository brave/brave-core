// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/common/channel_info.h"

#include <dispatch/dispatch.h>
#import <Foundation/Foundation.h>

#import "base/mac/bundle_locations.h"
#import "base/strings/sys_string_conversions.h"
#include "build/branding_buildflags.h"
#include "components/version_info/version_info.h"
#include "components/version_info/version_string.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

std::string GetVersionString() {
  return std::string();
}

std::string GetChannelString() {
  return std::string();
}

version_info::Channel GetChannel() {
  return version_info::Channel::UNKNOWN;
}

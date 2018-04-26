/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/sparkle_glue_mac.h"

#include "base/mac/bundle_locations.h"

namespace {

id GetSUUpdater() {
  NSString* sparkle_path =
      [[base::mac::FrameworkBundle() privateFrameworksPath]
          stringByAppendingPathComponent:@"Sparkle.framework"];

  NSBundle* sparkle_bundle = [NSBundle bundleWithPath:sparkle_path];
  [sparkle_bundle load];

  Class sparkle_class = [sparkle_bundle classNamed:@"SUUpdater"];
  return [sparkle_class performSelector:NSSelectorFromString(@"sharedUpdater")];
}

}

@implementation SparkleGlue
{
  id su_updater_;
}

+ (instancetype)sharedSparkleGlue {
  static SparkleGlue* shared;
  if (shared == nil)
    shared = [[SparkleGlue alloc] init];
  return shared;
}

+ (std::string)descriptionFromAppcastItem:(id)item {
  NSString* description =
      [NSString stringWithFormat:@"AppcastItem(Date: %@, Version: %@)",
          [item performSelector:NSSelectorFromString(@"dateString")],
          [item performSelector:NSSelectorFromString(@"versionString")]];
  return [description UTF8String];
}

- (instancetype)init {
  if (self = [super init]) {
    su_updater_ = GetSUUpdater();
    return self;
  } else {
    return nil;
  }
}

- (void)setDelegate:(id)delegate {
  [su_updater_ setDelegate:delegate];
}

- (void)checkForUpdates:(id)sender {
  [su_updater_ checkForUpdates:sender];
}

@end

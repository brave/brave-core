// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_PRIVATE_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/brave_viewer/brave_viewer_service.h"

namespace brave_viewer {
class BraveViewerService;
} // namespace brave_viewer

NS_ASSUME_NONNULL_BEGIN

@interface BraveViewerService (Private)

- (instancetype)init;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_PRIVATE_H_

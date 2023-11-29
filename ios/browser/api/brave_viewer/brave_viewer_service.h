// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveViewerService : NSObject
/**
 Return a script to be injected to the page given by the provided url

 @param url The URL of the main frame.
 @return A script to inject to the page.
 */
- (void)getTestScriptForURL:(NSURL*)url 
                   callback:(void (^)(NSString* _Nullable script)) callback;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_VIEWER_BRAVE_VIEWER_SERVICE_H_

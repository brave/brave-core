// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_
#define BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_

#include <Foundation/Foundation.h>

OBJC_EXPORT
@interface BravePasswordImporter : NSObject
- (void)importPasswords:(NSString*)fileName completion:(void (^)())completion;
@end

#endif  // BRAVE_IOS_BROWSER_API_PASSWORD_IMPORTER_BRAVE_PASSWORD_IMPORTER_H_

/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_PRIVATE_H_

#include "brave/ios/browser/api/local_data_file_service/local_data_file_service_wrapper.h"

/// A private extension for `LocalDataFileServiceWrapper`
/// which is usable inside the brave core library and not exposed to iOS.
@interface LocalDataFileServiceWrapper (Private)
/// Start the service so that it begins downloading/updating the necessary
/// components.
- (void)start;
@end

#endif  // BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_PRIVATE_H_

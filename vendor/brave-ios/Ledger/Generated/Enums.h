/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, BATLogLevel) {
  BATLogLevelLogError = 1,
  BATLogLevelLogWarning = 2,
  BATLogLevelLogInfo = 3,
  BATLogLevelLogDebug = 4,
  BATLogLevelLogRequest = 5,
  BATLogLevelLogResponse = 6
} NS_SWIFT_NAME(LogLevel);

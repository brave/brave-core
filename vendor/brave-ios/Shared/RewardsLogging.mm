/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RewardsLogging.h"
#import "BATBraveRewards.h"

namespace rewards {

__weak BATBraveRewards* g_rewards_client = nil;

void set_rewards_client_for_logging(BATBraveRewards *rewards) {
  g_rewards_client = rewards;
}

void LogMessage(const char* file,
                int line,
                int verbose_level,
                NSString *message) {
  if (!g_rewards_client) {
    return;
  }
  
  const auto filename = [NSString stringWithUTF8String:file];
  
  [g_rewards_client.delegate logMessageWithFilename:filename
                                         lineNumber:line
                                          verbosity:verbose_level
                                            message:message];
}
  
void Log(const char* file, int line, int verbose_level, NSString *format, ...) {
  if (!g_rewards_client) {
    return;
  }
  
  const auto filename = [NSString stringWithUTF8String:file];
  
  va_list args;
  va_start(args, format);
  NSString *message = [[NSString alloc] initWithFormat:format arguments:args];
  va_end(args);
  
  [g_rewards_client.delegate logMessageWithFilename:filename
                                         lineNumber:line
                                          verbosity:verbose_level
                                            message:message];
}
  
} // namespace rewards

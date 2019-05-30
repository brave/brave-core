/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#import <Foundation/Foundation.h>
#import <iostream>

#import "bat/ledger/ledger_client.h"
#import "bat/ads/ads_client.h"

class RewardsLogStream : public ledger::LogStream, public ads::LogStream {
public:
  RewardsLogStream(const char* file, const int line, const ledger::LogLevel log_level);
  RewardsLogStream(const char* file, const int line, const ads::LogLevel log_level);
  
  std::ostream& stream() override;
  
private:
  std::string log_message_;
  
  void constructLogMessageWithPrefix(const std::string& prefix, const char* file, const int line);
  
  // Not copyable, not assignable
  RewardsLogStream(const RewardsLogStream&) = delete;
  RewardsLogStream& operator=(const RewardsLogStream&) = delete;
};

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#import <Foundation/Foundation.h>
#import <iostream>

#import "bat/ledger/ledger_client.h"
#import "bat/ads/ads_client.h"
#import "Logger.h"

/// A generic unbuffered logger logs messages via iostream
class RewardsLogStream : public ledger::LogStream, public ads::LogStream {
public:
  /// Creates a stream for logging ledger information
  RewardsLogStream(const char* file, const int line, const ledger::LogLevel log_level);
    
  /// Creates a stream for logging ads information
  RewardsLogStream(const char* file, const int line, const ads::LogLevel log_level);
    
  /// Flushes logs immediately upon destruction
  ~RewardsLogStream() override;

  /// A stream used to insert logging data
  /// IE: stream() << "Some information that needs logging"
  std::ostream& stream() override;

private:
  std::string log_message_;
  std::unique_ptr<UnbufferedLogger> log_stream;
    
  void constructLogMessageWithPrefix(const std::string& prefix, const char* file, const int line);

  // Not copyable, not assignable
  RewardsLogStream(const RewardsLogStream&) = delete;
  RewardsLogStream& operator=(const RewardsLogStream&) = delete;
};

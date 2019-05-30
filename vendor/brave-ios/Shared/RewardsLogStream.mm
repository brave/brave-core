// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "RewardsLogStream.h"

RewardsLogStream::RewardsLogStream(const char* file,
                                   const int line,
                                   const ledger::LogLevel log_level) {
  
  std::map<ledger::LogLevel, std::string> map {
    {ledger::LOG_ERROR, "ERROR"},
    {ledger::LOG_WARNING, "WARNING"},
    {ledger::LOG_INFO, "INFO"},
    {ledger::LOG_DEBUG, "DEBUG"},
    {ledger::LOG_RESPONSE, "RESPONSE"}
  };
  constructLogMessageWithPrefix(map[log_level], file, line);
}

RewardsLogStream::RewardsLogStream(const char* file,
                                   const int line,
                                   const ads::LogLevel log_level) {
  std::map<ads::LogLevel, std::string> map {
    {ads::LOG_ERROR, "ERROR"},
    {ads::LOG_WARNING, "WARNING"},
    {ads::LOG_INFO, "INFO"}
  };
  
  constructLogMessageWithPrefix(map[log_level], file, line);
}

std::ostream& RewardsLogStream::stream() {
  std::cout << std::endl << log_message_;
  return std::cout;
}

void RewardsLogStream::constructLogMessageWithPrefix(const std::string& prefix, const char* file, const int line) {
  //    log_message_ = prefix + ": in " + file + " on line " + std::to_string(line) + ": ";
  log_message_ = prefix + ": ";
}

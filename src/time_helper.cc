/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <ctime>

#include "time_helper.h"

namespace helper {

void Time::TimeStamp(std::string& str) {
  time_t rawtime;
  std::time(&rawtime);

  char buffer[24];
  struct tm* timeinfo = std::localtime(&rawtime);
  strftime(buffer, 24, "%FT%TZ", timeinfo);
  str = std::string(buffer);
}

}  // namespace helper

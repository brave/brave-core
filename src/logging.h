/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_REWARDS_LOGGING_H_
#define BAT_REWARDS_LOGGING_H_

#define LOG(client, severity) \
  client->Log(__FILE__, __LINE__, severity)

#endif  // BAT_REWARDS_LOGGING_H_

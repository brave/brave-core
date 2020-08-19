/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EVENT_LOG_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EVENT_LOG_H_

#include <string>

namespace brave_rewards {

struct EventLog {
  EventLog();
  ~EventLog();
  EventLog(const EventLog& properties);

  std::string event_log_id;
  std::string key;
  std::string value;
  uint64_t created_at;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EVENT_LOG_H_

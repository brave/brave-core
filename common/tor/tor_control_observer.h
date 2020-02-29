/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_CONTROL_OBSERVER_H_
#define BRAVE_BROWSER_TOR_TOR_CONTROL_OBSERVER_H_

#include <map>
#include <string>

#include "brave/common/tor/tor_control_event.h"

#include "base/observer_list.h"

namespace tor {

class TorControlObserver : public base::CheckedObserver {
 public:
  virtual void OnTorControlReady();
  virtual void OnTorClosed();

  virtual void OnTorEvent(
      TorControlEvent,
      const std::string& initial,
      const std::map<std::string, std::string>& extra);

  // Debugging options.
  virtual void OnTorRawCmd(const std::string& cmd);
  virtual void OnTorRawAsync(const std::string& status,
                             const std::string& line);
  virtual void OnTorRawMid(const std::string& status,
                           const std::string& line);
  virtual void OnTorRawEnd(const std::string& status,
                           const std::string& line);
};

}

#endif  // BRAVE_BROWSER_TOR_TOR_CONTROL_OBSERVER_H_

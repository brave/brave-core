/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_CAN_SEND_HISTORY_H
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_CAN_SEND_HISTORY_H

namespace history {
  class QueryResults;
}

namespace brave_sync {

class CanSendSyncHistory {
public:
  virtual ~CanSendSyncHistory() = default;
  virtual void HaveInitialHistory(history::QueryResults* results) = 0;

};

} // namespace brave_sync

#endif

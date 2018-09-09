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

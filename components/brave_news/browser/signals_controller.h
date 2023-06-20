#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNALS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNALS_CONTROLLER_H_

#include <unordered_map>
#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/raw_feed_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/prefs/pref_service.h"
namespace brave_news {

using Signals = base::flat_map<std::string, mojom::SignalPtr>;
using SignalsCallback = mojom::BraveNewsController::GetSignalsCallback;

class SignalsController {
 public:
  SignalsController(PublishersController* publishers_controller,
                    ChannelsController* channels_controller,
                    RawFeedController* feed_controller,
                    PrefService* prefs,
                    history::HistoryService* history_service);
  SignalsController(const SignalsController&) = delete;
  SignalsController& operator=(const SignalsController&) = delete;
  ~SignalsController();

  void GetSignals(SignalsCallback callback);

 private:
  void OnGotHistory(std::vector<mojom::FeedItemMetadataPtr> articles,
                    SignalsCallback callback,
                    history::QueryResults results);

  base::CancelableTaskTracker task_tracker_;

  raw_ptr<PublishersController> publishers_controller_;
  raw_ptr<ChannelsController> channels_controller_;
  raw_ptr<RawFeedController> feed_controller_;
  raw_ptr<PrefService> prefs_;
  raw_ptr<history::HistoryService> history_service_;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_SIGNALS_CONTROLLER_H_

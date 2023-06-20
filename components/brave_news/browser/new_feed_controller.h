#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_NEW_FEED_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_NEW_FEED_CONTROLLER_H_

#include "base/functional/callback_forward.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "components/prefs/pref_service.h"

namespace history {
class HistoryService;
}

namespace brave_news {

using FeedItems = std::vector<mojom::FeedItemPtr>;
using GetFeedItemsCallback = base::OnceCallback<void(FeedItems)>;

class NewFeedController : public PublishersController::Observer {
 public:
  NewFeedController(PublishersController* publishers_controller,
                    ChannelsController* channels_controller,
                    history::HistoryService* history_service,
                    api_request_helper::APIRequestHelper* api_request_helper,
                    PrefService* prefs);
  ~NewFeedController() override;
  NewFeedController(const NewFeedController&) = delete;
  NewFeedController& operator=(const NewFeedController&) = delete;

  void GetOrFetchFeed();
  void EnsureFeedIsUpdating();

  void OnPublishersUpdated(PublishersController* publishers) override;

 private:
  void FetchCombinedFeed(Publishers publishers, GetFeedItemsCallback callback);
  void NotifyUpdateDone();

  raw_ptr<PublishersController> publishers_controller_ = nullptr;
  raw_ptr<ChannelsController> channels_controller_ = nullptr;
  raw_ptr<history::HistoryService> history_service_ = nullptr;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;

  base::CancelableTaskTracker task_tracker_;
  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;
  base::ScopedObservation<PublishersController, PublishersController::Observer>
      publishers_observation_;
  base::flat_map<std::string, std::string> locale_feed_etags_;
  bool is_update_in_progress_ = false;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_NEW_FEED_CONTROLLER_H_

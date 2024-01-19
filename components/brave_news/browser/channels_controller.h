// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace brave_news {

namespace p3a {
class NewsMetrics;
}  // namespace p3a

using Channels = base::flat_map<std::string, mojom::ChannelPtr>;
using ChannelsCallback = base::OnceCallback<void(Channels)>;

inline constexpr char kTopSourcesChannel[] = "Top Sources";
inline constexpr char kTopNewsChannel[] = "Top News";

class ChannelsController : public PublishersController::Observer {
 public:
  explicit ChannelsController(PrefService* prefs,
                              PublishersController* publishers_controller,
                              p3a::NewsMetrics* news_metrics);
  ~ChannelsController() override;
  ChannelsController(const ChannelsController&) = delete;
  ChannelsController& operator=(const ChannelsController&) = delete;

  static Channels GetChannelsFromPublishers(const Publishers& publishers,
                                            PrefService* prefs);

  // Get all the Locales the user has subscribed to channels in.
  std::vector<std::string> GetChannelLocales() const;

  // Gets all the locales the user has subscribed to a channel in.
  std::vector<std::string> GetChannelLocales(
      const std::string& channel_id) const;
  void GetAllChannels(ChannelsCallback callback);
  void AddListener(mojo::PendingRemote<mojom::ChannelsListener> listener);
  mojom::ChannelPtr SetChannelSubscribed(const std::string& locale,
                                         const std::string& channel_id,
                                         bool subscribed);
  bool GetChannelSubscribed(const std::string& locale,
                            const std::string& channel_id);

  // PublishersController::Observer:
  void OnPublishersUpdated(PublishersController* controller) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveNewsFeedBuildingTest, BuildFeed);
  FRIEND_TEST_ALL_PREFIXES(BraveNewsFeedBuildingTest,
                           DuplicateItemsAreNotIncluded);
  static void SetChannelSubscribedPref(PrefService* prefs,
                                       const std::string& locale,
                                       const std::string& channel_id,
                                       bool subscribed);
  raw_ptr<PrefService> prefs_;
  raw_ptr<PublishersController> publishers_controller_;
  raw_ptr<p3a::NewsMetrics> news_metrics_;

  mojo::RemoteSet<mojom::ChannelsListener> listeners_;

  size_t enabled_channel_count_ = 0;

  base::ScopedObservation<PublishersController, PublishersController::Observer>
      scoped_observation_{this};
};
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNELS_CONTROLLER_H_

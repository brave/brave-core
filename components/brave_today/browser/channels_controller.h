// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_CHANNELS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_CHANNELS_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "components/prefs/pref_service.h"

namespace brave_news {
using Channels = base::flat_map<std::string, mojom::ChannelPtr>;
using ChannelsCallback = base::OnceCallback<void(Channels)>;

constexpr char kTopSourcesChannel[] = "Top Sources";

class ChannelsController {
 public:
  explicit ChannelsController(PrefService* prefs,
                              PublishersController* publishers_controller);
  ~ChannelsController();
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
  mojom::ChannelPtr SetChannelSubscribed(const std::string& locale,
                                         const std::string& channel_id,
                                         bool subscribed);
  bool GetChannelSubscribed(const std::string& locale,
                            const std::string& channel_id);

 private:
  raw_ptr<PrefService> prefs_;
  raw_ptr<PublishersController> publishers_controller_;
};
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_CHANNELS_CONTROLLER_H_

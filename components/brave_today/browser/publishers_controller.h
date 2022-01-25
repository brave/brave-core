// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/one_shot_event.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

using GetPublishersCallback = mojom::BraveNewsController::GetPublishersCallback;
using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;

class PublishersController {
 public:
  PublishersController(
      PrefService* prefs,
      api_request_helper::APIRequestHelper* api_request_helper);
  ~PublishersController();
  PublishersController(const PublishersController&) = delete;
  PublishersController& operator=(const PublishersController&) = delete;

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPublishersUpdated(PublishersController* controller) = 0;
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void GetOrFetchPublishers(GetPublishersCallback callback,
                            bool wait_for_current_update = false);
  void EnsurePublishersIsUpdating();
  void ClearCache();

 private:
  void GetOrFetchPublishers(base::OnceClosure callback,
                            bool wait_for_current_update);

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<api_request_helper::APIRequestHelper> api_request_helper_ = nullptr;

  std::unique_ptr<base::OneShotEvent> on_current_update_complete_;
  base::ObserverList<Observer> observers_;
  Publishers publishers_;
  bool is_update_in_progress_ = false;
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_PUBLISHERS_CONTROLLER_H_

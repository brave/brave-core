// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_TRACKER_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_TRACKER_SERVICE_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ai_chat {

// Maintains a list of tabs and notifies listeners when tabs are added, removed,
// or updated.
class TabTrackerService : public KeyedService, public mojom::TabTrackerService {
 public:
  TabTrackerService();
  ~TabTrackerService() override;

  // Updates the tab with the given |tab_id|. If |tab| is nullptr the tab will
  // be removed.
  void UpdateTab(int32_t tab_id, mojom::TabDataPtr tab);

  void Bind(mojo::PendingReceiver<mojom::TabTrackerService> receiver);

  // Bridge to tab activation, implemented in code with chrome/browser/ui
  // access.
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Activates the tab matching `tab_id`. Returns true if a tab was
    // activated.
    virtual bool ActivateTab(int32_t tab_id) = 0;
  };

  // Sets the delegate used by ActivateTab(). The delegate must clear itself
  // by passing nullptr before it is destroyed.
  void SetDelegate(Delegate* delegate);

  // Activates the tab matching `tab_id` via the delegate. Returns false if no
  // delegate is installed or the tab cannot be found.
  bool ActivateTab(int32_t tab_id);

  // mojom::TabTrackerService
  void AddObserver(
      mojo::PendingRemote<mojom::TabDataObserver> observer) override;

 private:
  friend class TabInformerBrowserTest;

  void NotifyObservers();
  void NotifyObserver(mojom::TabDataObserver* observer);

  mojo::ReceiverSet<mojom::TabTrackerService> receivers_;
  mojo::RemoteSet<mojom::TabDataObserver> observers_;

  std::vector<mojom::TabDataPtr> tabs_;
  raw_ptr<Delegate> delegate_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_TRACKER_SERVICE_H_

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_INFORMER_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_INFORMER_SERVICE_H_

#include <vector>

#include "brave/components/ai_chat/core/common/mojom/tab_informer.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ai_chat {

// Maintains a list of tabs and notifies listeners when tabs are added, removed,
// or updated.
class TabInformerService : public KeyedService, public mojom::TabInformer {
 public:
  TabInformerService();
  ~TabInformerService() override;

  // Updates the tab with the given |tab_id|. If |tab| is nullptr the tab will
  // be removed.
  void UpdateTab(int32_t tab_id, mojom::TabPtr tab);

  void Bind(mojo::PendingReceiver<mojom::TabInformer> receiver);

  // mojom::TabInformer
  void AddListener(mojo::PendingRemote<mojom::TabListener> listener) override;

 private:
  friend class TabInformerBrowserTest;

  void NotifyListeners();
  void NotifyListener(mojom::TabListener* listener);

  mojo::ReceiverSet<mojom::TabInformer> receivers_;
  mojo::RemoteSet<mojom::TabListener> listeners_;

  std::vector<mojom::TabPtr> tabs_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TAB_INFORMER_SERVICE_H_

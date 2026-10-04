// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import createAIChatAPI from './ai_chat_api'

export default function bindWebUiServices() {
  // Create global mojo connections
  const serviceRemote = Mojom.Service.getRemote()
  const metricsRemote = new Mojom.MetricsRemote()
  serviceRemote.bindMetrics(metricsRemote.$.bindNewPipeAndPassReceiver())
  const uiHandlerRemote = Mojom.AIChatUIHandler.getRemote()
  const tabTrackerServiceRemote = Mojom.TabTrackerService.getRemote()

  const aiChat = createAIChatAPI(
    serviceRemote,
    uiHandlerRemote,
    Mojom.BookmarksPageHandler.getRemote(),
    Mojom.HistoryUIHandler.getRemote(),
    metricsRemote,
  )

  // Bind mojo receivers to the appropriate observers, routing events.
  const serviceObserverReceiver = new Mojom.ServiceObserverReceiver(
    aiChat.serviceObserver,
  )
  serviceRemote
    .bindObserver(serviceObserverReceiver.$.bindNewPipeAndPassRemote())
    .then(({ state }) => {
      aiChat.api.state.update(state)
    })

  const chatUIReceiver = new Mojom.ChatUIReceiver(aiChat.chatUIObserver)
  uiHandlerRemote
    .setChatUI(chatUIReceiver.$.bindNewPipeAndPassRemote())
    .then(({ isStandalone }) => {
      aiChat.api.isStandalone.update(isStandalone)
    })
  const tabDataObserverReceiver = new Mojom.TabDataObserverReceiver(
    aiChat.tabDataObserver,
  )
  tabTrackerServiceRemote.addObserver(
    tabDataObserverReceiver.$.bindNewPipeAndPassRemote(),
  )

  return aiChat
}

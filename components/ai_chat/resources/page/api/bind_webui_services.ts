// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Closable } from '$web-common/api'
import { loadTimeData } from '$web-common/loadTimeData'
import * as Mojom from '../../common/mojom'
import createAIChatAPI, { MCPHostInterface } from './ai_chat_api'

/**
 * Connects to the host of the user's MCP servers. Where connectors aren't
 * managed, the remote is left unbound, so that its requests fail.
 */
function bindMCPHost(serviceRemote: Mojom.ServiceRemote) {
  const remote = new Mojom.MCPHostRemote()
  const isBound = loadTimeData.getBoolean('isMCPEnabled')
  if (isBound) {
    serviceRemote.bindMCPHost(remote.$.bindNewPipeAndPassReceiver())
  }
  return {
    // The `.m.js` bindings mistype `result<>` responses.
    remote: remote as unknown as Closable<MCPHostInterface>,
    isBound,
  }
}

export default function bindWebUiServices() {
  // Create global mojo connections
  const serviceRemote = Mojom.Service.getRemote()
  const metricsRemote = new Mojom.MetricsRemote()
  serviceRemote.bindMetrics(metricsRemote.$.bindNewPipeAndPassReceiver())
  const uiHandlerRemote = Mojom.AIChatUIHandler.getRemote()
  const tabTrackerServiceRemote = Mojom.TabTrackerService.getRemote()
  const mcpHost = bindMCPHost(serviceRemote)

  const aiChat = createAIChatAPI(
    serviceRemote,
    uiHandlerRemote,
    Mojom.BookmarksPageHandler.getRemote(),
    Mojom.HistoryUIHandler.getRemote(),
    metricsRemote,
    mcpHost.remote,
  )

  // Bind mojo receivers to the appropriate observers, routing events.
  const serviceObserverReceiver = new Mojom.ServiceObserverReceiver(
    aiChat.serviceObserver,
  )
  serviceRemote
    .bindObserver(serviceObserverReceiver.$.bindNewPipeAndPassRemote())
    .then(({ state, generationStates }) => {
      aiChat.api.state.update(state)
      aiChat.api.emitEvent('onConversationGenerationStateChanged', [
        generationStates,
      ])
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

  if (mcpHost.isBound) {
    const mcpHostObserverReceiver = new Mojom.MCPHostObserverReceiver(
      aiChat.mcpHostObserver,
    )
    mcpHost.remote.bindObserver(
      mcpHostObserverReceiver.$.bindNewPipeAndPassRemote(),
    )
  }

  return aiChat
}

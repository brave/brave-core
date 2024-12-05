// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import API from '../common/api'
import * as Mojom from '../common/mojom'

// Global state for this UI
export type ConversationEntriesUIState = Mojom.ConversationEntriesState & {
  conversationHistory: Mojom.ConversationTurn[]
  isMobile: boolean
}

// Default state before initial API call
export const defaultConversationEntriesUIState: ConversationEntriesUIState = {
  conversationHistory: [],
  isGenerating: false,
  isLeoModel: true,
  contentUsedPercentage: undefined,
  canSubmitUserEntries: false,
  isMobile: loadTimeData.getBoolean('isMobile')
}

// Define how to get the initial data and update the state from events
export default class APIConversationEntriesUI extends API<ConversationEntriesUIState> {
  public conversationHandler: Mojom.UntrustedConversationHandlerRemote
    = Mojom.UntrustedConversationHandler.getRemote()

  public uiHandler: Mojom.UntrustedUIHandlerRemote
    = Mojom.UntrustedUIHandler.getRemote()

  public parentUIFrame: Mojom.ParentUIFrameRemote
    = new Mojom.ParentUIFrameRemote()

  private conversationObserver: Mojom.UntrustedConversationUICallbackRouter
    = new Mojom.UntrustedConversationUICallbackRouter

  constructor() {
    super(defaultConversationEntriesUIState)
    this.initialize()
  }

  async initialize() {
    const [
      { conversationEntriesState },
      { conversationHistory }
     ] = await Promise.all([
      this.conversationHandler.bindUntrustedConversationUI(
        this.conversationObserver.$.bindNewPipeAndPassRemote()
      ),
      this.conversationHandler.getConversationHistory()
    ])
    this.setPartialState({
      ...conversationEntriesState,
      conversationHistory
    })
    this.conversationObserver.onConversationHistoryUpdate.addListener(
      async () => this.setPartialState(await this.conversationHandler.getConversationHistory())
    )

    // Set up communication with the parent frame
    this.uiHandler.bindParentPage(this.parentUIFrame.$.bindNewPipeAndPassReceiver())

    // Send document height to parent frame
    window.addEventListener('resize', () => this.sendDocumentHeight())
    new ResizeObserver(() => this.sendDocumentHeight()).observe(document.body)
    this.sendDocumentHeight()
  }

  sendDocumentHeight() {
    this.parentUIFrame.childHeightChanged(document.body.clientHeight)
  }

  static getInstance() {
    if (!apiInstance) {
      apiInstance = new APIConversationEntriesUI()
    }
    return apiInstance
  }
}

let apiInstance: APIConversationEntriesUI

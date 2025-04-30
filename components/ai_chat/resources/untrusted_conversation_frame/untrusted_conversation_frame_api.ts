// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import API from '../common/api'
import * as Mojom from '../common/mojom'
import { updateConversationHistory } from '../common/conversation_history_utils'

// Global state for this UI
export type ConversationEntriesUIState = Mojom.ConversationEntriesState & {
  conversationHistory: Mojom.ConversationTurn[]
  isMobile: boolean
  associatedContent: Mojom.AssociatedContent[]
}

// Default state before initial API call
export const defaultConversationEntriesUIState: ConversationEntriesUIState = {
  conversationHistory: [],
  isGenerating: false,
  isLeoModel: true,
  allModels: [],
  currentModelKey: '',
  contentUsedPercentage: undefined,
  trimmedTokens: BigInt(0),
  totalTokens: BigInt(0),
  canSubmitUserEntries: false,
  isMobile: loadTimeData.getBoolean('isMobile'),
  associatedContent: []
}

// Define how to get the initial data and update the state from events
export default class UntrustedConversationFrameAPI extends API<ConversationEntriesUIState> {
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
      async (entry?: Mojom.ConversationTurn) => {
        if (entry) {
          // Use the shared utility function to update the history
          const updatedHistory =
            updateConversationHistory(this.state.conversationHistory, entry)
          this.setPartialState({
            conversationHistory: updatedHistory
          })
        } else {
          // When no entry is provided, fetch the full history
          const { conversationHistory } =
            await this.conversationHandler.getConversationHistory()
          this.setPartialState({ conversationHistory })
        }
      }
    )

    this.conversationObserver.onEntriesUIStateChanged.addListener((state: Mojom.ConversationEntriesState) => {
      this.setPartialState(state)
    })

    this.conversationObserver.associatedContentChanged.addListener((content: Mojom.AssociatedContent[]) => {
      this.setPartialState({ associatedContent: content })
    })

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
      apiInstance = new UntrustedConversationFrameAPI()
    }
    return apiInstance
  }
}

let apiInstance: UntrustedConversationFrameAPI

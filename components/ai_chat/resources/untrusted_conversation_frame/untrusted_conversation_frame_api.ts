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
  // TODO(https://github.com/brave/brave-browser/issues/49258):
  // Store the tab ID of a task on the ToolUseEvent and not for the whole
  // conversation, once multiple agentic tabs and tasks per conversation are
  // supported.
  contentTaskTabId?: number
}

// Default state before initial API call
export const defaultConversationEntriesUIState: ConversationEntriesUIState = {
  conversationHistory: [],
  isGenerating: false,
  isLeoModel: true,
  allModels: [],
  currentModelKey: '',
  contentUsedPercentage: null,
  visualContentUsedPercentage: null,
  trimmedTokens: BigInt(0),
  totalTokens: BigInt(0),
  canSubmitUserEntries: false,
  isMobile: loadTimeData.getBoolean('isMobile'),
  associatedContent: [],
  conversationCapability: Mojom.ConversationCapability.CHAT,
}

// Updates a tool use event for a conversation entry in the history.
// when its data has changed (e.g. output).
export function updateToolUseEventInHistory(
  history: Readonly<Mojom.ConversationTurn[]>,
  entryUuid: string,
  toolUse: Mojom.ToolUseEvent,
) {
  const updatedHistory = [...history]
  const index = history.findIndex((entry) => entry.uuid === entryUuid)
  if (index !== -1) {
    updatedHistory[index] = {
      ...updatedHistory[index],
      events: updatedHistory[index].events?.map((event) => event) || null,
    }
    const eventIndex = updatedHistory[index].events?.findIndex(
      (event) => event.toolUseEvent?.id === toolUse.id,
    )
    if (eventIndex !== undefined && eventIndex !== -1) {
      updatedHistory[index].events![eventIndex] = {
        toolUseEvent: toolUse,
      } as Mojom.ConversationEntryEvent
      return updatedHistory
    }
  }
  return null
}

// Define how to get the initial data and update the state from events
export default class UntrustedConversationFrameAPI extends API<ConversationEntriesUIState> {
  public conversationHandler: Mojom.UntrustedConversationHandlerRemote =
    new Mojom.UntrustedConversationHandlerRemote()

  public uiHandler: Mojom.UntrustedUIHandlerRemote =
    Mojom.UntrustedUIHandler.getRemote()

  public parentUIFrame: Mojom.ParentUIFrameRemote =
    new Mojom.ParentUIFrameRemote()

  public conversationObserver: Mojom.UntrustedConversationUICallbackRouter =
    new Mojom.UntrustedConversationUICallbackRouter()

  public uiObserver: Mojom.UntrustedUICallbackRouter =
    new Mojom.UntrustedUICallbackRouter()

  constructor() {
    super(defaultConversationEntriesUIState)
    this.initialize()
  }

  async initialize() {
    // Bind UntrustedUI for memory notifications
    this.uiHandler.bindUntrustedUI(this.uiObserver.$.bindNewPipeAndPassRemote())
    const conversationId = window.location.pathname.split('/').pop() || ''
    this.uiHandler.bindConversationHandler(
      conversationId,
      this.conversationHandler.$.bindNewPipeAndPassReceiver(),
    )

    const [{ conversationEntriesState }, { conversationHistory }] =
      await Promise.all([
        this.conversationHandler.bindUntrustedConversationUI(
          this.conversationObserver.$.bindNewPipeAndPassRemote(),
        ),
        this.conversationHandler.getConversationHistory(),
      ])

    const allModels =
      conversationEntriesState.conversationCapability
      === Mojom.ConversationCapability.CONTENT_AGENT
        ? conversationEntriesState.allModels.filter(
            (model) => model.supportsTools,
          )
        : conversationEntriesState.allModels

    this.setPartialState({
      ...conversationEntriesState,
      allModels,
      conversationHistory,
    })
    this.conversationObserver.onConversationHistoryUpdate.addListener(
      async (entry?: Mojom.ConversationTurn) => {
        if (entry) {
          // Use the shared utility function to update the history
          const updatedHistory = updateConversationHistory(
            this.state.conversationHistory,
            entry,
          )
          this.setPartialState({
            conversationHistory: updatedHistory,
          })
        } else {
          // When no entry is provided, fetch the full history
          const { conversationHistory } =
            await this.conversationHandler.getConversationHistory()
          this.setPartialState({ conversationHistory })
        }
      },
    )

    this.conversationObserver.onToolUseEventOutput.addListener(
      (entryUuid: string, toolUse: Mojom.ToolUseEvent) => {
        const updatedHistory = updateToolUseEventInHistory(
          this.state.conversationHistory,
          entryUuid,
          toolUse,
        )
        if (updatedHistory) {
          this.setPartialState({ conversationHistory: updatedHistory })
        }
      },
    )

    this.conversationObserver.contentTaskStarted.addListener(
      (tabId: number) => {
        this.setPartialState({ contentTaskTabId: tabId })
      },
    )

    this.conversationObserver.onEntriesUIStateChanged.addListener(
      (state: Mojom.ConversationEntriesState) => {
        this.setPartialState(state)
      },
    )

    this.conversationObserver.associatedContentChanged.addListener(
      (content: Mojom.AssociatedContent[]) => {
        this.setPartialState({ associatedContent: content })
      },
    )

    // Set up communication with the parent frame
    this.uiHandler.bindParentPage(
      this.parentUIFrame.$.bindNewPipeAndPassReceiver(),
    )

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

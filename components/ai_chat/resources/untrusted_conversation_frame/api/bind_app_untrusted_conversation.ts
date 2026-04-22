// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TypeScript service implementations used in place of Mojo/C++ bindings when
// ai_chat_app = true. Receives conversation history and streaming updates from
// the main page via BroadcastChannel (keyed on the conversation UUID from the
// URL path).

import * as Mojom from '../../common/mojom'
import { makeCloseable } from '$web-common/api'
import createUntrustedConversationApi from './untrusted_conversation_api'

// Must match the prefix in bind_app_conversation.ts.
const CONVERSATION_CHANNEL_PREFIX = 'ai-chat-conv-'

const defaultConversationEntriesState: Mojom.ConversationEntriesState = {
  isGenerating: false,
  isToolExecuting: false,
  toolUseTaskState: Mojom.TaskState.kNone,
  isLeoModel: true,
  allModels: [],
  currentModelKey: '',
  contentUsedPercentage: undefined,
  visualContentUsedPercentage: undefined,
  trimmedTokens: BigInt(0),
  totalTokens: BigInt(0),
  canSubmitUserEntries: true,
  conversationCapabilities: [Mojom.ConversationCapability.CHAT],
  suggestedQuestions: [],
  suggestionStatus: Mojom.SuggestionGenerationStatus.None,
  currentError: Mojom.APIError.None,
  isTemporary: false,
}

const defaultServiceState: Mojom.ServiceState = {
  hasAcceptedAgreement: true,
  isStoragePrefEnabled: true,
  isStorageNoticeDismissed: true,
  canShowPremiumPrompt: false,
}

function makeEntriesState(
  isGenerating: boolean,
): Mojom.ConversationEntriesState {
  return {
    ...defaultConversationEntriesState,
    isGenerating,
    canSubmitUserEntries: !isGenerating,
  }
}

// Request the current history from the main-page handler and return it.
// Falls back to [] after a 3-second timeout if the main page isn't ready.
function fetchHistoryFromMainPage(
  channel: BroadcastChannel,
): Promise<Mojom.ConversationTurn[]> {
  return new Promise((resolve) => {
    const timeoutId = setTimeout(() => {
      channel.removeEventListener('message', handler)
      resolve([])
    }, 3000)

    function handler(event: MessageEvent) {
      if (event.data?.type !== 'historyResponse') return
      clearTimeout(timeoutId)
      channel.removeEventListener('message', handler)
      resolve(event.data.history as Mojom.ConversationTurn[])
    }

    channel.addEventListener('message', handler)
    channel.postMessage({ type: 'requestHistory' })
  })
}

export async function bindAppUntrustedConversation() {
  const uuid = window.location.pathname.split('/').pop() || ''
  const channel = new BroadcastChannel(CONVERSATION_CHANNEL_PREFIX + uuid)

  const conversationHandler =
    makeCloseable<Mojom.UntrustedConversationHandlerInterface>({
      getConversationHistory: async () => ({
        conversationHistory: await fetchHistoryFromMainPage(channel),
      }),
      bindUntrustedConversationUI: async () => ({
        conversationEntriesState: defaultConversationEntriesState,
      }),
      modifyConversation: () => {},
      respondToToolUseRequest: () => {},
      processPermissionChallenge: () => {},
      regenerateAnswer: () => {},
      submitSuggestion: () => {},
      generateQuestions: () => {},
      retryAPIRequest: () => {},
      switchToNonPremiumModel: () => {},
    })

  const uiHandler: Mojom.UntrustedUIHandlerInterface = {
    hasMemory: async () => ({ exists: false }),
    bindConversationHandler: () => {},
    bindUntrustedUI: () => {},
    openSearchURL: () => {},
    openLearnMoreAboutBraveSearchWithLeo: () => {},
    openURLFromResponse: () => {},
    openAIChatCustomizationSettings: () => {},
    addTabToThumbnailTracker: () => {},
    removeTabFromThumbnailTracker: () => {},
    bindParentPage: () => {},
    deleteMemory: () => {},
    goPremium: () => {},
    refreshPremiumSession: () => {},
    openModelSupportUrl: () => {},
    openStorageSupportUrl: () => {},
  }

  const parentUIFrame = makeCloseable<Mojom.ParentUIFrameInterface>({
    childHeightChanged: () => {},
    rateMessage: () => {},
    userRequestedOpenGeneratedUrl: () => {},
    dragStart: () => {},
    showSkillDialog: () => {},
    requestNewConversation: () => {},
    handleResetError: () => {},
  })

  const service = makeCloseable<Mojom.UntrustedServiceInterface>({
    bindObserver: async () => ({ state: defaultServiceState }),
    getPremiumStatus: async () => ({
      status: Mojom.PremiumStatus.Inactive,
      info: null,
    }),
    dismissStorageNotice: () => {},
    dismissPremiumPrompt: () => {},
  })

  const conversationAPI = createUntrustedConversationApi(
    conversationHandler,
    uiHandler,
    parentUIFrame,
    service,
  )

  // Forward main-page channel messages to the appropriate UI observers.
  channel.addEventListener('message', (event: MessageEvent) => {
    const msg = event.data
    if (!msg?.type) return

    switch (msg.type as string) {
      case 'historyUpdate':
        conversationAPI.conversationObserver.onConversationHistoryUpdate(
          msg.entry as Mojom.ConversationTurn,
        )
        break
      case 'historyInvalidate':
        // null triggers a full refetch via getConversationHistory()
        conversationAPI.conversationObserver.onConversationHistoryUpdate(null)
        break
      case 'stateChanged':
        conversationAPI.conversationObserver.onEntriesUIStateChanged(
          makeEntriesState(msg.isGenerating as boolean),
        )
        break
    }
  })

  // Push initial state.
  conversationAPI.conversationObserver.onEntriesUIStateChanged(
    defaultConversationEntriesState,
  )
  conversationAPI.serviceObserver.onStateChanged(defaultServiceState)

  return {
    api: conversationAPI.api,
    close: () => {
      conversationAPI.close()
      channel.close()
    },
  }
}

export type BoundAppUntrustedConversation = Awaited<
  ReturnType<typeof bindAppUntrustedConversation>
>

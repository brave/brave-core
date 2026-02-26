// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  createInterfaceApi,
  eventsFor,
  endpointsFor,
  Closable,
  VoidMethodKeys,
  state,
} from '$web-common/api'
import * as Mojom from '../../common/mojom'
import { updateConversationHistory } from '../../common/conversation_history_utils'

// Updates a tool use event for a conversation entry in the history
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
      events: updatedHistory[index].events?.map((event) => event),
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

// Actions exposed from uiHandler
type UIHandlerActions = Pick<
  Mojom.UntrustedUIHandlerInterface,
  | 'openSearchURL'
  | 'openLearnMoreAboutBraveSearchWithLeo'
  | 'addTabToThumbnailTracker'
  | 'removeTabFromThumbnailTracker'
  | 'hasMemory'
  | 'deleteMemory'
  | 'openAIChatCustomizationSettings'
  | 'openURLFromResponse'
  | 'goPremium'
  | 'refreshPremiumSession'
  | 'openModelSupportUrl'
  | 'openStorageSupportUrl'
  | 'openURL'
>

// State that comes from ConversationEntriesState plus additional UI state
export type ConversationEntriesUIState = Mojom.ConversationEntriesState & {
  isMobile: boolean
  associatedContent: Mojom.AssociatedContent[]
  // TODO(https://github.com/brave/brave-browser/issues/49258):
  // Store the tab ID of a task on the ToolUseEvent and not for the whole
  // conversation, once multiple agentic tabs and tasks per conversation are
  // supported.
  contentTaskTabId?: number
}

export default function createUntrustedConversationApi(
  conversationHandler: Closable<Mojom.UntrustedConversationHandlerInterface>,
  uiHandler: Mojom.UntrustedUIHandlerInterface,
  parentUIFrame: Closable<Mojom.ParentUIFrameInterface>,
  service: Closable<Mojom.UntrustedServiceInterface>,
) {
  let conversationObserver: Mojom.UntrustedConversationUIInterface
  let uiObserver: Mojom.UntrustedUIInterface
  let serviceObserver: Mojom.UntrustedServiceObserverInterface

  const api = createInterfaceApi({
    actions: {
      conversationHandler: conversationHandler as Pick<
        Mojom.UntrustedConversationHandlerInterface,
        VoidMethodKeys<Mojom.UntrustedConversationHandlerInterface>
      >,
      uiHandler: uiHandler as UIHandlerActions,
      parentUIFrame: parentUIFrame as Pick<
        Mojom.ParentUIFrameInterface,
        VoidMethodKeys<Mojom.ParentUIFrameInterface>
      >,
      service: service as Pick<
        Mojom.UntrustedServiceInterface,
        VoidMethodKeys<Mojom.UntrustedServiceInterface>
      >,
    },

    endpoints: {
      // General UI data
      ...endpointsFor(uiHandler, {
        hasMemory: {
          response: (result) => result.exists,
        },
      }),

      // Conversation data
      ...endpointsFor(conversationHandler, {
        getConversationHistory: {
          response: (result) => result.conversationHistory,
          prefetchWithArgs: [],
          placeholderData: [] as Mojom.ConversationTurn[],
        },
      }),

      state: state<Mojom.ConversationEntriesState>({
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
        canSubmitUserEntries: false,
        conversationCapabilities: [Mojom.ConversationCapability.CHAT],
        suggestedQuestions: [],
        suggestionStatus: Mojom.SuggestionGenerationStatus.None,
        currentError: Mojom.APIError.None,
        isTemporary: false,
      }),

      // Service state (profile-level) - updated via UntrustedServiceObserver
      serviceState: state<Mojom.ServiceState>({
        hasAcceptedAgreement: false,
        isStoragePrefEnabled: false,
        isStorageNoticeDismissed: false,
        canShowPremiumPrompt: false,
      }),

      // Premium status - fetched from service
      ...endpointsFor(service, {
        getPremiumStatus: {
          response: (result) => ({
            isPremiumUser:
              result.status !== undefined
              && result.status !== Mojom.PremiumStatus.Inactive,

            isPremiumUserDisconnected:
              result.status === Mojom.PremiumStatus.ActiveDisconnected,
          }),
          placeholderData: {
            isPremiumUser: false,
            isPremiumUserDisconnected: false,
          },
          prefetchWithArgs: [],
          refetchOnWindowFocus: 'always',
        },
      }),
    },

    events: {
      // General UI
      ...eventsFor(
        Mojom.UntrustedUIInterface,
        {
          onMemoriesChanged(memories) {},
          thumbnailUpdated(tabId, dataUri) {},
        },
        (observer) => {
          uiObserver = observer
        },
      ),

      // Conversation UI
      ...eventsFor(
        Mojom.UntrustedConversationUIInterface,
        {
          onConversationHistoryUpdate(entry) {
            if (!entry) {
              api.getConversationHistory.invalidate()
            } else {
              api.getConversationHistory.update((old) =>
                updateConversationHistory(old, entry),
              )
            }
          },

          onToolUseEventOutput(entryUuid, toolUse) {
            const currentHistory = api.getConversationHistory.current()
            const updatedHistory = updateToolUseEventInHistory(
              currentHistory,
              entryUuid,
              toolUse,
            )
            if (updatedHistory) {
              api.getConversationHistory.update(updatedHistory)
            }
          },

          contentTaskStarted(tabId) {},

          onEntriesUIStateChanged(state) {
            state.allModels = state.conversationCapabilities.includes(
              Mojom.ConversationCapability.CONTENT_AGENT,
            )
              ? state.allModels.filter((m) =>
                  m.supportedCapabilities.includes(
                    Mojom.ConversationCapability.CONTENT_AGENT,
                  ),
                )
              : state.allModels
            api.state.update(state)
          },

          associatedContentChanged(content) {},
        },
        (observer) => {
          conversationObserver = observer
        },
      ),

      // Service observer (profile-level state changes)
      ...eventsFor(
        Mojom.UntrustedServiceObserverInterface,
        {
          onStateChanged(state) {
            api.serviceState.update(state)
          },
        },
        (observer) => {
          serviceObserver = observer
        },
      ),
    },
  })

  return {
    api,

    conversationObserver: conversationObserver!,
    uiObserver: uiObserver!,
    serviceObserver: serviceObserver!,

    close: () => {
      api.close()
      conversationHandler.$.close()
      parentUIFrame.$.close()
      service.$.close()
    },
  }
}

export type UntrustedConversationAPI = ReturnType<
  typeof createUntrustedConversationApi
>['api']

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

let i = 0

export default function createUntrustedConversationApi(
  conversationHandler: Closable<Mojom.UntrustedConversationHandlerInterface>,
  uiHandler: Mojom.UntrustedUIHandlerInterface,
  parentUIFrame: Closable<Mojom.ParentUIFrameInterface>,
) {
  let conversationObserver: Mojom.UntrustedConversationUIInterface
  let uiObserver: Mojom.UntrustedUIInterface

  const api = createInterfaceApi({
    // Use a different key which acts as dependency key for all the generated
    // hooks.
    key: `${++i}`,
    // Define the mojom actions we will expose to the UI.
    // These are functions that are simply passed through
    // with no caching or deduplicating.
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
    },

    // Define the data-retrieval and mutation functions we'll cache or mutate
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

      // State is a special endpoint that doesn't fetch but is updated via events
      // We'll initialize it via the bindUntrustedConversationUI call
      state: state<Mojom.ConversationEntriesState>({
        isGenerating: false,
        isToolExecuting: false,
        toolUseTaskState: Mojom.TaskState.kNone,
        isPremiumUser: false,
        isLeoModel: true,
        allModels: [],
        currentModelKey: '',
        contentUsedPercentage: undefined,
        visualContentUsedPercentage: undefined,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
        canSubmitUserEntries: false,
        conversationCapability: Mojom.ConversationCapability.CHAT,
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
          // This function is synchronous, so ok to set here even when returning
          // as a result of the whole API-client generating function.
          uiObserver = observer
        },
      ),

      // Conversation UI
      ...eventsFor(
        Mojom.UntrustedConversationUIInterface,
        {
          onConversationHistoryUpdate(entry) {
            if (!entry) {
              // Force full update, getConversationHistory will be re-fetched
              api.getConversationHistory.invalidate()
            } else {
              // Update with the new/modified entry
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
            state.allModels =
              state.conversationCapability
              === Mojom.ConversationCapability.CONTENT_AGENT
                ? state.allModels.filter((model) => model.supportsTools)
                : state.allModels
            api.state.update(state)
          },

          associatedContentChanged(content) {},
        },
        (observer) => {
          conversationObserver = observer
        },
      ),
    },
  })

  return {
    api,

    conversationObserver: conversationObserver!,
    uiObserver: uiObserver!,

    close: () => {
      api.close()
      conversationHandler.$.close()
      parentUIFrame.$.close()
    },
  }
}

export type UntrustedConversationAPI = ReturnType<
  typeof createUntrustedConversationApi
>['api']

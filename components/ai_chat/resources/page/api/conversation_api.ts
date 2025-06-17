// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import {
  createInterfaceApi,
  eventsFor,
  endpointsFor,
  Closable
} from '$web-common/api'
import * as Mojom from '../../common/mojom';
import { updateConversationHistory } from '../../common/conversation_history_utils';

type ExposedInterface = Pick<
  Mojom.ConversationHandlerInterface,
  | 'generateQuestions'
  | 'submitSummarizationRequest'
  | 'submitSuggestion'
  | 'retryAPIRequest'
  | 'changeModel'
  | 'rateMessage'
  | 'sendFeedback'
  | 'submitHumanConversationEntryWithAction'
  | 'submitHumanConversationEntry'
  | 'stopGenerationAndMaybeGetHumanEntry'
  | 'setShouldSendPageContents'
>;

export default function createConversationApi(conversationHandler: Closable<Mojom.ConversationHandlerInterface>) {
  let receiver: Mojom.ConversationUIReceiver

  const api = createInterfaceApi({
    // Define the mojom actions we will expose to the UI.
    // These are functions that are simply passed through
    // with no caching or deduplicating.
    actions: conversationHandler as ExposedInterface,

    // Define the data-retrieval and mutation functions we'll cache or mutate
    endpoints: {
      // Easy generation from interface functions. Caching will automatically be
      // via parameters each function takes (if any).
      // Result types will all be inferred and both
      // non-react endpoints and react use[Key] hooks will be generated.
      ...endpointsFor(conversationHandler, {
        // Queries fetch data whenever they are called,
        // e.g. const { data, inProgress } = api.useGetState()
        getState: {
          response: (result) => result.conversationState,
          prefetchWithArgs: [],
          placeholderData: {
            error: Mojom.APIError.None,
            isRequestInProgress: false,
            currentModelKey: '',
            allModels: [],
            suggestedQuestions: [],
            suggestionStatus: Mojom.SuggestionGenerationStatus.None,
            associatedContent: [],
            shouldSendContent: false,
          }
        },
        getConversationHistory: {
          response: (result) => result.conversationHistory,
          prefetchWithArgs: [],
          placeholderData: [] as Mojom.ConversationTurn[],
        },
        // Mutations are only called when the mutate() function is run, e.g.
        // api.getScreenshots.mutate()
        // or
        // const { data: lastGetScreenshotsMutateResult, mutate, isMutating  } = api.useGetScreenshots()
        getScreenshots: {
          mutationResponse: (result) => result.screenshots
        },
        clearErrorAndGetFailedMessage: {
          // We could do an optimistic update here, but
          // C++ will handle that
          // onMutate: () => {
          //   api.getState.update({ error: Mojom.APIError.None });
          // },
          mutationResponse: (result) => result.turn,
        },
        setTemporary: {
          // Instead of allow setTemporary as a pass through via actions,
          // we intercept it via a mutation so we can handle onMutate
          mutationResponse: () => {},

          // onMutate is provided with the arguments passed to the
          // setTemporary call
          onMutate: ([isTemporary]) => {
            // Optimistic update, could be handled by C++ and send
            // an event for updating the state. Not neccessary
            // though as "temporary" can only be for a single UI
            // view, by the nature of the "temporary" feature.
            api.getState.update({ temporary: !isTemporary })
          }
        }
      }),

      // We can also provide endpoints that are more custom
      // or call something non-mojom - perhaps a message handler,
      // a custom browser API, a fetch or or some DOM function:
      //
      // getTabs: {
      //   query: (active: boolean) => chrome.tabs.query({ active }),
      //   prefetchWithArgs: [true]
      // }

      // We *could* call endpointsFor for other mojom interfaces
      // but it's best to keep a separation.
      // One good reason might be that it's tied to the same
      // lifecycle and UI tree. For example if we have multiple
      // instances of the same API for different parts of the UI.
    },

    events: {
      ...eventsFor(Mojom.ConversationUIInterface, {
        onAPIRequestInProgress: (isRequestInProgress) => {
          // Update can be called passing a partial result for that endpoint
          api.getState.update({ isRequestInProgress })
        },

        onConversationHistoryUpdate(entry) {
          if (!entry) {
            // Force full update, getConversationHistory will be re-fetched if the
            // data is asked for (or if set to prefetch).
            api.getConversationHistory.invalidate();
          } else {
            // Update can also be called with a function that takes the previous state
            api.getConversationHistory.update((old) => updateConversationHistory(old, entry));
          }
        },

        onAPIResponseError: (error) => {
          api.getState.update({ error });
        },

        onModelDataChanged: (currentModelKey, allModels) => {
          api.getState.update({
            currentModelKey,
            allModels,
          });
        },

        onSuggestedQuestionsChanged(suggestedQuestions, suggestionStatus) {
          api.getState.update({
            suggestedQuestions,
            suggestionStatus,
          });
        },

        onAssociatedContentInfoChanged: (associatedContent, shouldSendContent) => {
          api.getState.update({
            associatedContent,
            shouldSendContent,
          });
        },

        // This event is subscribable by the UI
        onConversationDeleted: () => {},
      }, (observer) => {
        receiver = new Mojom.ConversationUIReceiver(observer);
      }),
    },
  });

  return {
    api,
    // The receiver is created in the eventsFor callback
    // which is always called immediately.
    observer: receiver!,
    close: () => {
      api.close()
      receiver.$.close()
      conversationHandler.$.close()
    }
  }
}

export type ConversationAPI = ReturnType<typeof createConversationApi>['api'];

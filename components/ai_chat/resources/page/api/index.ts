/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Mojom from '../../common/mojom'
import {
  Closable,
  createInterfaceApi,
  state,
  eventsFor,
  endpointsFor,
  VoidMethodKeys,
} from '$web-common/api'

// createAIChatAPI uses createInterfaceAPI to configure
// the various mojom interfaces for use
// in the UI. Some function are marked as subscribable,
// some as mutable, some as events, and some as actions -
// which infer differences in data access and observability.
//
// Always provide parameters as mockable so that we can provide an
// implementation of the basic interface. This is achieved with
// Closable<MyMojomServiceInterface> instead of passing the
// MyMojomServiceRemote derived class type, which has a lot of extra
// methods we'd have to mock.
//
// This is a good place for services and ui handlers.
// It may be broken up by service or combined for convenience, depending
// on how big the interfaces are.
export default function createAIChatApi(
  service: Closable<Mojom.ServiceInterface>,
  uiHandler: Closable<Mojom.AIChatUIHandlerInterface>,
  bookmarksService: Closable<Mojom.BookmarksPageHandlerInterface>,
  historyService: Closable<Mojom.HistoryUIHandlerInterface>,
  metrics: Closable<Mojom.MetricsInterface>,
) {
  // Hang on to some receiver references as in some cases we're passed
  // the remote from an event on another receiver.
  let serviceObserver: Mojom.ServiceObserverInterface
  let chatUIObserver: Mojom.ChatUIInterface
  let conversationEntriesFrameObserver: Mojom.ParentUIFrameInterface
  let tabDataObserver: Mojom.TabDataObserverInterface

  // Define the API layout and what to do with the interface data:
  // - which functions should be exposed to the UI as subscribable with hooks
  // - which should be prefetched,
  // - which are mutable and should only be called explicitly
  const api = createInterfaceApi({
    // endpoints are subscribable Queries or Mutations.
    // - Queries are fetched as soon as the hook is present.
    // - Mutations only when the mutate function of the hook is called.
    endpoints: {
      // `endpointsFor` is a helper to generate endpoints from an interface
      // with minimal boilerplate.
      ...endpointsFor(service, {
        getConversations: {
          response: (result) => result.conversations,
          prefetchWithArgs: [],
          placeholderData: [] as Mojom.Conversation[],
        },
        getActionMenuList: {
          response: (result) => result.actionList,
          prefetchWithArgs: [],
          placeholderData: [] as Mojom.ActionGroup[],
        },
        getSkills: {
          response: (result) => result.skills,
          prefetchWithArgs: [],
          placeholderData: [] as Mojom.Skill[],
        },
        getPremiumStatus: {
          response: (result) => ({
            /**
             * @deprecated use api.useGetPremiumState().isPlaceholderData
             * instead to know if this has ever been fetched.
             */
            isPremiumStatusFetching: false,

            isPremiumUser:
              result.status !== undefined
              && result.status !== Mojom.PremiumStatus.Inactive,

            isPremiumUserDisconnected:
              result.status === Mojom.PremiumStatus.ActiveDisconnected,
          }),
          // Should be `placeholderData`?
          initialData: {
            isPremiumStatusFetching: true,
            isPremiumUser: false,
            isPremiumUserDisconnected: false,
          },
          prefetchWithArgs: [],
          refetchOnWindowFocus: 'always',
        },
      }),

      ...endpointsFor(uiHandler, {
        uploadFile: {
          mutationResponse: (result) => result.uploadedFiles,
        },
        processImageFile: {
          mutationResponse: (result) => result.processedFile,
        },
        getPluralString: {
          response: (result) => result.pluralString,
        },
      }),

      ...endpointsFor(bookmarksService, {
        getBookmarks: {
          response: (result) => result.bookmarks,
          placeholderData: [] as Mojom.Bookmark[],
        },
      }),

      ...endpointsFor(historyService, {
        getHistory: {
          response: (result) => result.history,
          placeholderData: [] as Mojom.HistoryEntry[],
        },
      }),

      // An endpoint can also be a non-query: data that will be updated
      // outside (in this case, when binding) and updated by event.
      //
      // We provide initial data here with the `state` helper, so that
      // we don't have to manually define with:
      // `{ query: () => Promise.reject(), enabled: false, initialData: { ... } }`.
      // which state(...) is a wrapper for.
      state: state<Mojom.ServiceState>({
        hasAcceptedAgreement: false,
        isStoragePrefEnabled: false,
        isStorageNoticeDismissed: false,
        canShowPremiumPrompt: false,
      }),

      // no intial data so we know when the value has been received
      // (undefined until the service responds) to avoid flash
      // of different layout.
      isStandalone: state<boolean | undefined>(undefined),

      tabs: state<Mojom.TabData[]>([]),
    },

    // actions are passed through to consumers as-is, no caching or result handling
    actions: {
      service: service as Pick<
        Mojom.ServiceInterface,
        VoidMethodKeys<Mojom.ServiceInterface> | 'conversationExists'
      >,
      uiHandler: uiHandler as Pick<
        Mojom.AIChatUIHandlerInterface,
        VoidMethodKeys<Mojom.AIChatUIHandlerInterface>
      >,
      metrics: metrics as Pick<
        Mojom.MetricsInterface,
        VoidMethodKeys<Mojom.MetricsInterface>
      >,
    },

    // Events are subscribable data and offers an easy hook to wrap the subscription
    events: {
      // Provide a partial implementation of a receiver,
      // and eventsFor will optionally generate events
      // that are create as subscribables in the API.
      ...eventsFor(
        Mojom.ServiceObserverInterface,
        {
          onConversationListChanged: (conversations) => {
            api.getConversations.update(conversations)
          },
          onStateChanged: (state) => {
            api.state.update(state)
          },
          onSkillsChanged: (skills) => {
            api.getSkills.update(skills)
          },
        },
        async (observer) => {
          serviceObserver = observer
        },
      ),

      ...eventsFor(
        Mojom.ChatUIInterface,
        {
          // We don't need to do any handling here, just pass through
          onNewDefaultConversation(contentId: number) {},

          // TODO(petemill): rename in mojom to
          // `onAttachedFilesAreProcessing`.
          onUploadFilesSelected() {},

          onChildFrameBound(parentPagePendingReceiver) {},
        },
        async (observer) => {
          chatUIObserver = observer
        },
      ),

      ...eventsFor(
        Mojom.ParentUIFrameInterface,
        {
          // TODO(petemill): rename in mojom to
          // `OnConversationEntriesFrameHeightChanged`, etc.
          childHeightChanged(height) {},
          rateMessage(turnUuid, isLiked) {},
          userRequestedOpenGeneratedUrl(url) {},
          dragStart() {},
          regenerateAnswerMenuIsOpen(isOpen) {},
          showSkillDialog(prompt) {},
          showPremiumSuggestionForRegenerate(isVisible) {},
        },
        (observer) => {
          conversationEntriesFrameObserver = observer
        },
      ),

      ...eventsFor(
        Mojom.TabDataObserverInterface,
        {
          tabDataChanged: (tabs) => {
            api.tabs.update(tabs)
          },
        },
        (observer) => {
          tabDataObserver = observer
        },
      ),
    },
  })

  return {
    api,

    serviceObserver: serviceObserver!,
    chatUIObserver: chatUIObserver!,
    conversationEntriesFrameObserver: conversationEntriesFrameObserver!,
    tabDataObserver: tabDataObserver!,

    close: () => {
      api.close()
    },
  }
}

export type AIChatAPI = ReturnType<typeof createAIChatApi>

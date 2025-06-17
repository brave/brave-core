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
  VoidMethodKeys
} from '$web-common/api'

// createAIChatAPI uses createInterfaceAPI to configure
// the various mojom interfaces for use
// in the UI. Some function are marked as subscribable,
// some as mutable, some as events, and some as actions.
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
  service: Closable<Mojom.ServiceInterface> = Mojom.Service.getRemote(),
  uiHandler: Closable<Mojom.AIChatUIHandlerInterface> = Mojom.AIChatUIHandler.getRemote(),
  tabTrackerService: Closable<Mojom.TabTrackerServiceInterface> = Mojom.TabTrackerService.getRemote(),
  metrics?: Closable<Mojom.MetricsInterface>
) {
  if (!metrics) {
    // If we're not passing a metrics (e.g. a mocked one), bind a new one
    // to the service
    const metricsRemote = new Mojom.MetricsRemote()
    service.bindMetrics(metricsRemote.$.bindNewPipeAndPassReceiver())
    metrics = metricsRemote
  }
  // Hang on to some receiver references as in some cases we're passed
  // the remote from an event on another receiver.
  let conversationEntriesFrameObserver: Mojom.ParentUIFrameReceiver | undefined
  let tabDataObserver: Mojom.TabDataObserverReceiver | undefined
  // Keep track of all other receivers so that we can close them when asked to
  const receivers: Closable[] = []

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
        getPremiumStatus: {
          response: (result) => ({
            isPremiumStatusFetching: false,
            isPremiumUser: (result.status !== undefined && result.status !== Mojom.PremiumStatus.Inactive),
            isPremiumUserDisconnected: result.status === Mojom.PremiumStatus.ActiveDisconnected
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
        uploadImage: {
          mutationResponse: (result) => result.uploadedImages,
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
      isStandalone: state<boolean>(),

      tabs: state<Mojom.TabData[]>([]),
    },

    // actions are passed through to consumers as-is, no caching or result handling
    actions: {
      service: service as Pick<Mojom.ServiceInterface,
      VoidMethodKeys<Mojom.ServiceInterface>
      | 'conversationExists'
    >,
      uiHandler: uiHandler as Pick<Mojom.AIChatUIHandlerInterface, VoidMethodKeys<Mojom.AIChatUIHandlerInterface>>,
      metrics: metrics as Pick<Mojom.MetricsInterface, VoidMethodKeys<Mojom.MetricsInterface>>
    },

    // Events are subscribable data and offers an easy hook to wrap the subscription
    events: {
      // Provide a partial implementation of a receiver,
      // and eventsFor will optionally generate events
      // that are create as subscribables in the API.
      ...eventsFor(Mojom.ServiceObserverInterface,
        {
          onConversationListChanged: (conversations) => {
            api.getConversations.update(conversations)
          },
          onStateChanged: (state) => {
            api.state.update(state)
          },
        },
        async (serviceObserver) => {
          // We're provided the full handler in this callback so that
          // we can bind it to somewhere.
          const receiver = new Mojom.ServiceObserverReceiver(serviceObserver)
          const { state } = await service.bindObserver(receiver.$.bindNewPipeAndPassRemote())
          receivers.push(receiver)
          api.state.update(state)
        }
      ),

      ...eventsFor(Mojom.ChatUIInterface,
        {
          // We don't need to do any handling here, just pass through
          onNewDefaultConversation () {},

          // TODO(petemill): rename in mojom to
          // `onAttachedFilesAreProcessing`.
          onUploadFilesSelected() {},

          onChildFrameBound(parentPagePendingReceiver) {
            if (!conversationEntriesFrameObserver) {
              console.error('onChildFrameBound called before conversationEntriesFrameObserver was set')
              return
            }
            // Bind to the receiver
            conversationEntriesFrameObserver.$.bindHandle(parentPagePendingReceiver.handle)
          },

        }, async (serviceObserver) => {
          const receiver = new Mojom.ChatUIReceiver(serviceObserver)
          const { isStandalone } = await uiHandler.setChatUI(receiver.$.bindNewPipeAndPassRemote())
          api.isStandalone.update(isStandalone)
          if (!tabDataObserver) {
            console.error('onTabDataObserverBound called before tabDataObserver was set')
            return
          }
          if (isStandalone) {
            tabTrackerService.addObserver(tabDataObserver.$.bindNewPipeAndPassRemote())
          }
        }
      ),

      ...eventsFor(Mojom.ParentUIFrameInterface,
        {
          // TODO(petemill): rename in mojom to
          // `OnConversationEntriesFrameHeightChanged`.
          childHeightChanged(height) {},

          rateMessage(turnUuid, isLiked) {},

          userRequestedOpenGeneratedUrl(url) {},
        },
        (observer) => {
          conversationEntriesFrameObserver = new Mojom.ParentUIFrameReceiver(observer)
          receivers.push(conversationEntriesFrameObserver)
        }
      ),

      ...eventsFor(Mojom.TabDataObserverInterface,
        {
          tabDataChanged: (tabs) => {
            api.tabs.update(tabs)
          }
        },
        (observer) => {
          // We don't bind here as we only want to do it if the
          // page is standalone
          const receiver = new Mojom.TabDataObserverReceiver(observer)
          tabDataObserver = receiver
          receivers.push(receiver)
        }
      )

    },
  })

  return {
    ...api,
    close: () => {
      api.close()
      receivers.forEach((receiver) => {
        receiver.close()
      })
    }
  }
}

export type AIChatAPI = ReturnType<typeof createAIChatApi>

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

/**
 * `Mojom.MCPHostInterface` as its remote behaves. The `.m.js` bindings type a
 * `result<T, E>` response as `{ result: { success?: T, failure?: E } }`, but
 * resolve with the `T`, and reject with the `E`.
 */
// TODO(https://github.com/brave/brave-browser/issues/48960): drop this once
// the JSDoc of `result<>` responses is fixed.
export type MCPHostInterface = Omit<
  Mojom.MCPHostInterface,
  'addServer' | 'updateServer' | 'clearServerToken' | 'getServerTools'
> & {
  addServer: (
    params: Mojom.MCPServerParams,
    token: string | null,
  ) => Promise<Mojom.MCPServer>
  updateServer: (
    id: string,
    params: Mojom.MCPServerParams,
    token: string | null,
  ) => Promise<Mojom.MCPServer>
  clearServerToken: (id: string) => Promise<Mojom.MCPServer>
  getServerTools: (id: string) => Promise<Mojom.MCPServerTool[]>
}

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
  mcpHost: Closable<MCPHostInterface>,
) {
  // Hang on to some receiver references as in some cases we're passed
  // the remote from an event on another receiver.
  let serviceObserver: Mojom.ServiceObserverInterface
  let chatUIObserver: Mojom.ChatUIInterface
  let conversationEntriesFrameObserver: Mojom.ParentUIFrameInterface
  let tabDataObserver: Mojom.TabDataObserverInterface
  let mcpHostObserver: Mojom.MCPHostObserverInterface

  // A server's tools are listed again once its connection changes. A new
  // token isn't visible in `MCPServer`, so the mutations that set one reset
  // the server's tools themselves.
  const resetToolsOfChangedServers = (servers: Mojom.MCPServer[]) => {
    for (const old of api.getServers.current()) {
      const server = servers.find((s) => s.id === old.id)
      if (
        !server
        || server.name !== old.name
        || server.endpoint.url !== old.endpoint.url
        || server.enabled !== old.enabled
        || server.hasToken !== old.hasToken
        || server.tokenUnreadable !== old.tokenUnreadable
      ) {
        api.getServerTools.reset(old.id)
      }
    }
  }

  // A server's tools are listed again once it recovers, as what was listed
  // while it was failing no longer applies.
  const resetToolsOfRecoveredServers = (
    statuses: Record<string, Mojom.MCPServerStatus>,
  ) => {
    const old = api.getServerStatuses.current()
    for (const [id, status] of Object.entries(statuses)) {
      if (
        status === Mojom.MCPServerStatus.kAvailable
        && old[id] !== undefined
        && old[id] !== status
      ) {
        api.getServerTools.reset(id)
      }
    }
  }

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
        // Not prefetched - only the shared conversations dialog needs this, and
        // it fetches when it opens.
        getConversationShares: {
          response: (result) => result.shares,
          placeholderData: [] as Mojom.ConversationShare[],
        },
        deleteConversationShare: {
          mutationResponse: (result) => result.success,
          onSuccess: () => {
            api.getConversationShares.invalidate()
          },
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
          placeholderData: {
            isPremiumStatusFetching: true,
            isPremiumUser: false,
            isPremiumUserDisconnected: false,
          },
          prefetchWithArgs: [],
          refetchOnWindowFocus: 'always',
        },
      }),

      ...endpointsFor(uiHandler, {
        processImageFile: {
          mutationResponse: (result) => result.processedFile,
        },
        processPdfFile: {
          mutationResponse: (result) => result.processedFile,
        },
        processTextFile: {
          mutationResponse: (result) => result.processedFile,
        },
        getPluralString: {
          response: (result) => result.pluralString,
        },
        getFaviconDataURL: {
          response: (result) => result.dataUrl,
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

      // The user's MCP servers. Their failed requests reject with an
      // `MCPServerError`, except `removeServer`, which resolves with it.
      // Nothing is prefetched, so that nothing is asked of the host while the
      // feature is off.
      ...endpointsFor(mcpHost, {
        // Fails only if there's no host, which retrying wouldn't change.
        getServers: {
          response: (result) => result.servers,
          placeholderData: [] as Mojom.MCPServer[],
          retry: false,
        },
        // Whether Leo can use each server's tools, by server id. Kept current
        // by `onServerStatusesChanged`.
        getServerStatuses: {
          response: (result) => result.statuses,
          placeholderData: {} as Record<string, Mojom.MCPServerStatus>,
          retry: false,
        },
        // Connects to the server if it isn't connected. A failure isn't
        // retried on its own, as each attempt is a request to the server.
        getServerTools: {
          response: (tools) => tools,
          retry: false,
          retryOnMount: false,
          refetchOnWindowFocus: false,
          refetchOnReconnect: false,
        },
        addServer: {
          mutationResponse: (server) => server,
        },
        updateServer: {
          mutationResponse: (server) => server,
          onSuccess: (server, [, , token]) => {
            if (token !== null) {
              api.getServerTools.reset(server.id)
            }
          },
        },
        clearServerToken: {
          mutationResponse: (server) => server,
          onSuccess: (server) => {
            api.getServerTools.reset(server.id)
          },
        },
        removeServer: {
          mutationResponse: (result) => result.error,
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
        isContentAgentUserSelectable: false,
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
        | VoidMethodKeys<Mojom.ServiceInterface>
        | 'conversationExists'
        | 'shareConversation'
      >,
      uiHandler: uiHandler as Pick<
        Mojom.AIChatUIHandlerInterface,
        | VoidMethodKeys<Mojom.AIChatUIHandlerInterface>
        | `processTextFile`
        | `processImageFile`
        | `processPdfFile`
        | `showWorkspaceFolderPicker`
      >,
      metrics: metrics as Pick<
        Mojom.MetricsInterface,
        VoidMethodKeys<Mojom.MetricsInterface>
      >,
      mcpHost: mcpHost as Pick<MCPHostInterface, 'reconnectServer'>,
    },

    // Events are subscribable data and offer an easy hook to wrap the subscription
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
          // Every conversation that isn't idle, read with
          // useCurrentOnConversationGenerationStateChanged().
          onConversationGenerationStateChanged(generationStates: {
            [uuid: string]: Mojom.ConversationGenerationState
          }) {},
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

          onChildFrameBound(parentPagePendingReceiver) {},

          onDisplayModeChanged(isStandalone) {
            api.isStandalone.update(isStandalone)
          },
        },
        async (observer) => {
          chatUIObserver = observer
        },
      ),

      ...eventsFor(
        Mojom.ParentUIFrameInterface,
        {
          // TODO(https://github.com/brave/brave-browser/issues/52540): rename
          // in mojom to `OnConversationEntriesFrameHeightChanged`, etc.
          childHeightChanged(height) {},
          rateMessage(turnUuid, isLiked) {},
          dragStart() {},
          // <if expr="is_ios">
          dismissMenus() {},
          // </if>
          showSkillDialog(prompt) {},
          showImageLightbox(file) {},
          showWorkspaceFileLightbox(filePath: string) {},
          requestNewConversation() {},
          handleResetError() {},
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

      ...eventsFor(
        Mojom.MCPHostObserverInterface,
        {
          onServersChanged: (servers) => {
            resetToolsOfChangedServers(servers)
            api.getServers.update(servers)
          },
          onServerStatusesChanged: (statuses) => {
            resetToolsOfRecoveredServers(statuses)
            api.getServerStatuses.update(statuses)
          },
        },
        (observer) => {
          mcpHostObserver = observer
        },
      ),
    },
  })

  // The conversation list reads the generation states only while it's open,
  // and the query cache drops data that nothing has read for a few minutes.
  // They're only sent again when they change, so keep them subscribed for as
  // long as the API lives.
  const unsubscribeGenerationStates =
    api.subscribeToOnConversationGenerationStateChanged(() => {})

  return {
    api,

    serviceObserver: serviceObserver!,
    chatUIObserver: chatUIObserver!,
    conversationEntriesFrameObserver: conversationEntriesFrameObserver!,
    tabDataObserver: tabDataObserver!,
    mcpHostObserver: mcpHostObserver!,

    close: () => {
      unsubscribeGenerationStates()
      api.close()
    },
  }
}

export type AIChatAPI = ReturnType<typeof createAIChatApi>

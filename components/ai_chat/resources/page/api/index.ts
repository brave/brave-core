/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { debounce } from '$web-common/debounce'
import { loadTimeData } from '$web-common/loadTimeData'

// State that is owned by this class because it is global to the UI
// (loadTimeData / Service / UIHandler).
export type State = mojom.ServiceState & {
  initialized: boolean
  isStandalone?: boolean
  visibleConversations: mojom.Conversation[]
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  isMobile: boolean
  isHistoryFeatureEnabled: boolean
  allActions: mojom.ActionGroup[]
}

export const defaultUIState: State = {
  initialized: false,
  visibleConversations: [],
  hasAcceptedAgreement: false,
  isStoragePrefEnabled: false,
  isPremiumStatusFetching: true,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: false,
  isMobile: loadTimeData.getBoolean('isMobile'),
  isHistoryFeatureEnabled: loadTimeData.getBoolean('isHistoryEnabled'),
  allActions: [],
}

export type UIStateChangeEvent = CustomEvent<State>

class API {
  public service: mojom.ServiceRemote
  public observer: mojom.ServiceObserverCallbackRouter
  public uiHandler: mojom.AIChatUIHandlerRemote
  public uiObserver: mojom.ChatUICallbackRouter
  public state: State = { ...defaultUIState }

  private eventTarget = new EventTarget()

  constructor() {
    // Connect to service
    this.service = mojom.Service.getRemote()
    this.observer = new mojom.ServiceObserverCallbackRouter()
    // Connect to platform UI handler
    this.uiHandler = mojom.AIChatUIHandler.getRemote()
    this.uiObserver = new mojom.ChatUICallbackRouter()
    this.initialize()
    this.updateCurrentPremiumStatus()
  }

  async initialize() {
    // Get any global UI state. We can do that here instead of React context
    // to start as early as possible.
    // Premium state separately because it takes longer to fetch and we don't
    // need to wait for it.
    const [
      { state },
      { isStandalone },
      { conversations: visibleConversations },
      { actionList: allActions },
      premiumStatus
    ] = await Promise.all([
      this.service.bindObserver(this.observer.$.bindNewPipeAndPassRemote()),
      this.uiHandler.setChatUI(this.uiObserver.$.bindNewPipeAndPassRemote()),
      this.service.getVisibleConversations(),
      this.service.getActionMenuList(),
      this.getCurrentPremiumStatus()
    ])
    this.setPartialState({
      ...state,
      ...premiumStatus,
      initialized: true,
      isStandalone,
      visibleConversations,
      allActions
    })

    this.observer.onStateChanged.addListener((state: mojom.ServiceState) => {
      this.setPartialState(state)
    })

    this.observer.onConversationListChanged.addListener(
      (conversations: mojom.Conversation[]) => {
        this.setPartialState({
          visibleConversations: conversations
        })
      }
    )

    // Since there is no browser-side event for premium status changing,
    // we should check often. And since purchase or login is performed in
    // a separate WebContents, we can check when focus is returned here.
    window.addEventListener('focus', () => {
      this.updateCurrentPremiumStatus()
    })

    document.addEventListener('visibilitychange', (e) => {
      if (document.visibilityState === 'visible') {
        this.updateCurrentPremiumStatus()
      }
    })
  }

  addStateChangeListener(callback: (event: UIStateChangeEvent) => void) {
    this.eventTarget.addEventListener('uistatechange', callback)
  }

  removeStateChangeListener(callback: (event: UIStateChangeEvent) => void) {
    this.eventTarget.removeEventListener('uistatechange', callback)
  }

  private dispatchDebouncedStateChange = debounce(() => {
    console.debug('dispatching uistatechange event', {...this.state})
    this.eventTarget.dispatchEvent(
      new Event('uistatechange')
    )
  }, 0)

  private setPartialState(partialState: Partial<State>) {
    this.state = { ...this.state, ...partialState }
    this.dispatchDebouncedStateChange()
  }

  private async getCurrentPremiumStatus() {
    const { status } = await this.service.getPremiumStatus()
    return {
      isPremiumStatusFetching: false,
      isPremiumUser: (status !== undefined && status !== mojom.PremiumStatus.Inactive),
      isPremiumUserDisconnected: status === mojom.PremiumStatus.ActiveDisconnected
    }
  }

  private async updateCurrentPremiumStatus() {
    this.setPartialState(await this.getCurrentPremiumStatus())
  }
}

let apiInstance: API

export default function getAPI() {
  if (!apiInstance) {
    apiInstance = new API()
  }
  return apiInstance
}

export function bindConversation(id: string | undefined) {
  let conversationHandler: mojom.ConversationHandlerRemote =
    new mojom.ConversationHandlerRemote()
  let callbackRouter = new mojom.ConversationUICallbackRouter()

  if (id !== undefined) {
    getAPI().service.bindConversation(
      id,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  } else {
    getAPI().uiHandler.bindRelatedConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  }
  return {
    conversationHandler,
    callbackRouter
  }
}

export function newConversation() {
  let conversationHandler: mojom.ConversationHandlerRemote =
    new mojom.ConversationHandlerRemote()
  let callbackRouter = new mojom.ConversationUICallbackRouter()
  getAPI().uiHandler.newConversation(
    conversationHandler.$.bindNewPipeAndPassReceiver(),
    callbackRouter.$.bindNewPipeAndPassRemote()
  )
  return {
    conversationHandler,
    callbackRouter
  }
}

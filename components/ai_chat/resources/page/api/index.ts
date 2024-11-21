/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
export * from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { debounce } from '$web-common/debounce'
import { loadTimeData } from '$web-common/loadTimeData'

// State that is owned by this class because
export interface UIState {
  initialized: boolean
  isStandalone?: boolean
  visibleConversations: mojom.Conversation[]
  hasAcceptedAgreement: boolean
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  isStorageNoticeDismissed: boolean
  canShowPremiumPrompt?: boolean
  isMobile: boolean
  isHistoryEnabled: boolean
  allActions: mojom.ActionGroup[]
}

export const defaultUIState: UIState = {
  initialized: false,
  visibleConversations: [],
  hasAcceptedAgreement: false,
  isPremiumStatusFetching: true,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: undefined,
  isMobile: Boolean(loadTimeData.getBoolean('isMobile')),
  isHistoryEnabled: Boolean(loadTimeData.getBoolean('isHistoryEnabled')),
  allActions: [],
}


export type UIStateChangeEvent = CustomEvent<UIState>

class API {
  public Service: mojom.ServiceRemote
  public Observer: mojom.ServiceObserverCallbackRouter
  public UIHandler: mojom.AIChatUIHandlerRemote
  public UIObserver: mojom.ChatUICallbackRouter
  public UIState: UIState = { ...defaultUIState }

  private eventTarget = new EventTarget()

  constructor() {
    // Connect to service
    this.Service = mojom.Service.getRemote()
    this.Observer = new mojom.ServiceObserverCallbackRouter()
    this.Service.bindObserver(this.Observer.$.bindNewPipeAndPassRemote())
    // Connect to platform UI handler
    this.UIHandler = mojom.AIChatUIHandler.getRemote()
    this.UIObserver = new mojom.ChatUICallbackRouter()

    // Get any global UI state. We can do that here instead of React context
    // to start as early as possible.
    this.UIHandler.setChatUI(this.UIObserver.$.bindNewPipeAndPassRemote()).then(
      ({ isStandalone }) => {
        this.setPartialUIState({ isStandalone })
      }
    )

    this.getInitialState()
    this.updateCurrentPremiumStatus()

    if (this.UIState.isHistoryEnabled) {
      this.Observer.onConversationListChanged.addListener(
        (conversations: mojom.Conversation[]) => {
          this.setPartialUIState({
            visibleConversations: conversations
          })
        }
      )
    }

    this.Observer.onAgreementAccepted.addListener(() =>
      this.setPartialUIState({
        hasAcceptedAgreement: true
      })
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

  addUIStateChangeListener(callback: (event: UIStateChangeEvent) => void) {
    this.eventTarget.addEventListener('uistatechange', callback)
  }

  removeUIStateChangeListener(callback: (event: UIStateChangeEvent) => void) {
    this.eventTarget.removeEventListener('uistatechange', callback)
  }

  private dispatchDebouncedUIStateChange = debounce(() => {
    console.debug('dispatching uistatechange event', {...this.UIState})
    this.eventTarget.dispatchEvent(
      new CustomEvent<UIState>('uistatechange', { detail: this.UIState })
    )
  }, 0)

  private setPartialUIState(partialState: Partial<UIState>) {
    this.UIState = { ...this.UIState, ...partialState }
    this.dispatchDebouncedUIStateChange()
  }

  private async getInitialState() {
    const [
      { conversations: visibleConversations },
      { actionList: allActions },
      { canShowPremiumPrompt, isStorageNoticeDismissed, hasAcceptedAgreement }
    ] = await Promise.all([
      this.Service.getVisibleConversations(),
      this.Service.getActionMenuList(),
      this.Service.getNoticesState()
    ])
    this.setPartialUIState({
      initialized: true,
      hasAcceptedAgreement,
      isStorageNoticeDismissed,
      visibleConversations,
      allActions,
      canShowPremiumPrompt
    })
  }

  private async updateCurrentPremiumStatus() {
    const { status } = await this.Service.getPremiumStatus()
    this.setPartialUIState({
      isPremiumStatusFetching: false,
      isPremiumUser: (status !== undefined && status !== mojom.PremiumStatus.Inactive),
      isPremiumUserDisconnected: status === mojom.PremiumStatus.ActiveDisconnected
    })
  }
}

let apiInstance: API

export function setAPIForTesting(instance: API) {
  apiInstance = instance
}

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
    getAPI().Service.bindConversation(
      id,
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote()
    )
  } else {
    getAPI().UIHandler.bindRelatedConversation(
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
  getAPI().UIHandler.newConversation(
    conversationHandler.$.bindNewPipeAndPassReceiver(),
    callbackRouter.$.bindNewPipeAndPassRemote()
  )
  return {
    conversationHandler,
    callbackRouter
  }
}

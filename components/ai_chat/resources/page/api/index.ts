/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import { loadTimeData } from '$web-common/loadTimeData'
 import API from '../../common/api'
 import * as Mojom from '../../common/mojom'

// State that is owned by this class because it is global to the UI
// (loadTimeData / Service / UIHandler).
export type State = Mojom.ServiceState & {
  initialized: boolean
  isStandalone?: boolean
  visibleConversations: Mojom.Conversation[]
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  isMobile: boolean
  isHistoryFeatureEnabled: boolean
  allActions: Mojom.ActionGroup[]
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

// Owns connections to the browser via mojom as well as global state
class PageAPI extends API<State> {
  public service: Mojom.ServiceRemote
    = Mojom.Service.getRemote()

  public observer: Mojom.ServiceObserverCallbackRouter
    = new Mojom.ServiceObserverCallbackRouter()

  public uiHandler: Mojom.AIChatUIHandlerRemote
    = Mojom.AIChatUIHandler.getRemote()

  public uiObserver: Mojom.ChatUICallbackRouter
    = new Mojom.ChatUICallbackRouter()

  public conversationEntriesFrameObserver: Mojom.ParentUIFrameCallbackRouter
    = new Mojom.ParentUIFrameCallbackRouter()

  constructor() {
    super(defaultUIState)
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

    this.observer.onStateChanged.addListener((state: Mojom.ServiceState) => {
      this.setPartialState(state)
    })

    this.observer.onConversationListChanged.addListener(
      (conversations: Mojom.Conversation[]) => {
        this.setPartialState({
          visibleConversations: conversations
        })
      }
    )

    this.uiObserver.onChildFrameBound.addListener((parentPagePendingReceiver: Mojom.ParentUIFramePendingReceiver) => {
      this.conversationEntriesFrameObserver.$.bindHandle(parentPagePendingReceiver.handle)
    })

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

  private async getCurrentPremiumStatus() {
    const { status } = await this.service.getPremiumStatus()
    return {
      isPremiumStatusFetching: false,
      isPremiumUser: (status !== undefined && status !== Mojom.PremiumStatus.Inactive),
      isPremiumUserDisconnected: status === Mojom.PremiumStatus.ActiveDisconnected
    }
  }

  private async updateCurrentPremiumStatus() {
    this.setPartialState(await this.getCurrentPremiumStatus())
  }
}

let apiInstance: PageAPI

export default function getAPI() {
  if (!apiInstance) {
    apiInstance = new PageAPI()
  }
  return apiInstance
}

export function bindConversation(id: string | undefined) {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()

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
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()
  getAPI().uiHandler.newConversation(
    conversationHandler.$.bindNewPipeAndPassReceiver(),
    callbackRouter.$.bindNewPipeAndPassRemote()
  )
  return {
    conversationHandler,
    callbackRouter
  }
}

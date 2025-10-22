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
  conversations: Mojom.Conversation[]
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  isMobile: boolean
  isHistoryFeatureEnabled: boolean
  isAIChatAgentProfileFeatureEnabled: boolean
  isAIChatAgentProfile: boolean
  actionList: Mojom.ActionGroup[]
  skills: Mojom.Skill[]
  tabs: Mojom.TabData[]

  // This is the content of the tab that this conversation is shown next to (if
  // any). If the user creates a new conversation this will be used as the
  // default tab content.
  defaultTabContentId?: number
}

export const defaultUIState: State = {
  initialized: false,
  conversations: [],
  hasAcceptedAgreement: false,
  isStoragePrefEnabled: false,
  isPremiumStatusFetching: true,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: false,
  isMobile: loadTimeData.getBoolean('isMobile'),
  isHistoryFeatureEnabled: loadTimeData.getBoolean('isHistoryEnabled'),
  isAIChatAgentProfileFeatureEnabled: loadTimeData.getBoolean(
    'isAIChatAgentProfileFeatureEnabled',
  ),
  isAIChatAgentProfile: loadTimeData.getBoolean('isAIChatAgentProfile'),
  actionList: [],
  skills: [],
  tabs: [],
}

// Owns connections to the browser via mojom as well as global state
class PageAPI extends API<State> {
  public service: Mojom.ServiceRemote = Mojom.Service.getRemote()

  public metrics: Mojom.MetricsRemote = new Mojom.MetricsRemote()

  public observer: Mojom.ServiceObserverCallbackRouter =
    new Mojom.ServiceObserverCallbackRouter()

  public uiHandler: Mojom.AIChatUIHandlerRemote =
    Mojom.AIChatUIHandler.getRemote()

  public uiObserver: Mojom.ChatUICallbackRouter =
    new Mojom.ChatUICallbackRouter()

  public conversationEntriesFrameObserver: Mojom.ParentUIFrameCallbackRouter =
    new Mojom.ParentUIFrameCallbackRouter()

  public tabObserver: Mojom.TabDataObserverCallbackRouter =
    new Mojom.TabDataObserverCallbackRouter()

  public bookmarksService = Mojom.BookmarksPageHandler.getRemote()

  public historyService = Mojom.HistoryUIHandler.getRemote()

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

    this.uiObserver.onNewDefaultConversation.addListener(
      (contentId?: number) => {
        this.setPartialState({
          defaultTabContentId: contentId,
        })
      },
    )

    const [
      { state },
      { isStandalone },
      { conversations },
      { actionList },
      { skills },
      premiumStatus,
    ] = await Promise.all([
      this.service.bindObserver(this.observer.$.bindNewPipeAndPassRemote()),
      this.uiHandler.setChatUI(this.uiObserver.$.bindNewPipeAndPassRemote()),
      this.service.getConversations(),
      this.service.getActionMenuList(),
      this.service.getSkills(),
      this.getCurrentPremiumStatus(),
    ])
    this.setPartialState({
      ...state,
      ...premiumStatus,
      initialized: true,
      isStandalone,
      conversations,
      actionList,
      skills,
    })

    this.service.bindMetrics(this.metrics.$.bindNewPipeAndPassReceiver())

    // If we're in standalone mode, listen for tab changes so we can show a picker.
    Mojom.TabTrackerService.getRemote().addObserver(
      this.tabObserver.$.bindNewPipeAndPassRemote(),
    )
    this.tabObserver.tabDataChanged.addListener((tabs: Mojom.TabData[]) => {
      this.setPartialState({
        tabs,
      })
    })

    this.observer.onStateChanged.addListener((state: Mojom.ServiceState) => {
      this.setPartialState(state)
    })

    this.observer.onConversationListChanged.addListener(
      (conversations: Mojom.Conversation[]) => {
        this.setPartialState({
          conversations,
        })
      },
    )

    this.observer.onSkillsChanged.addListener((skills: Mojom.Skill[]) => {
      this.setPartialState({
        skills,
      })
    })

    this.uiObserver.onChildFrameBound.addListener(
      (parentPagePendingReceiver: Mojom.ParentUIFramePendingReceiver) => {
        this.conversationEntriesFrameObserver.$.bindHandle(
          parentPagePendingReceiver.handle,
        )
      },
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

  private async getCurrentPremiumStatus() {
    const { status } = await this.service.getPremiumStatus()
    return {
      isPremiumStatusFetching: false,
      isPremiumUser:
        status !== undefined && status !== Mojom.PremiumStatus.Inactive,
      isPremiumUserDisconnected:
        status === Mojom.PremiumStatus.ActiveDisconnected,
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
      callbackRouter.$.bindNewPipeAndPassRemote(),
    )
  } else {
    getAPI().uiHandler.bindRelatedConversation(
      conversationHandler.$.bindNewPipeAndPassReceiver(),
      callbackRouter.$.bindNewPipeAndPassRemote(),
    )
  }
  return {
    conversationHandler,
    callbackRouter,
  }
}

export function newConversation() {
  let conversationHandler: Mojom.ConversationHandlerRemote =
    new Mojom.ConversationHandlerRemote()
  let callbackRouter = new Mojom.ConversationUICallbackRouter()
  getAPI().uiHandler.newConversation(
    conversationHandler.$.bindNewPipeAndPassReceiver(),
    callbackRouter.$.bindNewPipeAndPassRemote(),
  )
  return {
    conversationHandler,
    callbackRouter,
  }
}

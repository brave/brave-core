// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as mojom from '../api'
import { loadTimeData } from '$web-common/loadTimeData'

export interface AIChatContext {
  initialized: boolean
  visibleConversations: mojom.Conversation[]
  hasAcceptedAgreement: boolean
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  isStorageNoticeDismissed: boolean
  canShowPremiumPrompt?: boolean
  isMobile: boolean
  isStandalone?: boolean
  isHistoryEnabled: boolean
  allActions: mojom.ActionGroup[]
  initialized: boolean
  goPremium: () => void
  managePremium: () => void
  handleAgreeClick: () => void
  dismissStorageNotice: () => void
  dismissPremiumPrompt: () => void
  userRefreshPremiumSession: () => void
  uiHandler?: mojom.AIChatUIHandlerRemote

  editingConversationId: string | null
  setEditingConversationId: (uuid: string | null) => void
}

const defaultContext: AIChatContext = {
  initialized: false,
  visibleConversations: [],
  hasAcceptedAgreement: false,
  isPremiumStatusFetching: true,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  isStandalone: getAPI().isStandalone,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: undefined,
  isMobile: Boolean(loadTimeData.getBoolean('isMobile')),
  isHistoryEnabled: Boolean(loadTimeData.getBoolean('isHistoryEnabled')),
  allActions: [],
  initialized: false,
  goPremium: () => { },
  managePremium: () => { },
  handleAgreeClick: () => { },
  dismissStorageNotice: () => { },
  dismissPremiumPrompt: () => { },
  userRefreshPremiumSession: () => { },

  editingConversationId: null,
  setEditingConversationId: () => { }
}

export const AIChatReactContext =
  React.createContext<AIChatContext>(defaultContext)

export function AIChatContextProvider(props: React.PropsWithChildren) {
  const [context, setContext] = React.useState<AIChatContext>(defaultContext)
  const [editingConversationId, setEditingConversationId] = React.useState<string | null>(null)

  const setPartialContext = (partialContext: Partial<AIChatContext>) => {
    setContext((value) => ({
      ...value,
      ...partialContext
    }))
  }

  React.useEffect(() => {
    const { Service, Observer, UIObserver } = getAPI()
    async function initialize() {
      const [
        { conversations: visibleConversations },
        { actionList: allActions },
        { canShowPremiumPrompt, isStorageNoticeDismissed, hasAcceptedAgreement }
      ] = await Promise.all([
        Service.getVisibleConversations(),
        Service.getActionMenuList(),
        Service.getNoticesState()
      ])
      setPartialContext({
        initialized: true,
        hasAcceptedAgreement,
        isStorageNoticeDismissed,
        visibleConversations,
        allActions,
        canShowPremiumPrompt,
        initialized: true
      })
    }

    async function updateCurrentPremiumStatus() {
      const { status } = await getAPI().Service.getPremiumStatus()
      setPartialContext({
        isPremiumStatusFetching: false,
        isPremiumUser: (status !== undefined && status !== mojom.PremiumStatus.Inactive),
        isPremiumUserDisconnected: status === mojom.PremiumStatus.ActiveDisconnected
      })
    }

    initialize()
    updateCurrentPremiumStatus()

    if (context.isHistoryEnabled) {
      Observer.onConversationListChanged.addListener(
        (conversations: mojom.Conversation[]) => {
          setPartialContext({
            visibleConversations: conversations
          })
        }
      )
    }

    Observer.onAgreementAccepted.addListener(() =>
      setPartialContext({
        hasAcceptedAgreement: true
      })
    )

    UIObserver.setInitialData.addListener((isStandalone: boolean) => {
      setPartialContext({
        isStandalone
      })
    })

    // Since there is no server-side event for premium status changing,
    // we should check often. And since purchase or login is performed in
    // a separate WebContents, we can check when focus is returned here.
    window.addEventListener('focus', () => {
      updateCurrentPremiumStatus()
    })

    document.addEventListener('visibilitychange', (e) => {
      if (document.visibilityState === 'visible') {
        updateCurrentPremiumStatus()
      }
    })
  }, [])

  const { Service, UIHandler } = getAPI()

  const store: AIChatContext = {
    ...context,
    ...props,
    goPremium: () => UIHandler.goPremium(),
    managePremium: () => UIHandler.managePremium(),
    dismissStorageNotice: () => Service.dismissStorageNotice(),
    dismissPremiumPrompt: () => Service.dismissPremiumPrompt(),
    userRefreshPremiumSession: () => UIHandler.refreshPremiumSession(),
    handleAgreeClick: () => Service.markAgreementAccepted(),
    uiHandler: UIHandler,
    editingConversationId,
    setEditingConversationId
  }

  return (
    <AIChatReactContext.Provider value={store}>
      {props.children}
    </AIChatReactContext.Provider>
  )
}

export function useAIChat() {
  return React.useContext(AIChatReactContext)
}

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as mojom from '../api'
import { loadTimeData } from '$web-common/loadTimeData'


interface Props {
  // Whether there is a specific conversation selected
  isDefaultConversation: boolean
  // Create a new conversation and use it
  onNewConversation: () => unknown
  onNewMultiTabConversation: () => void;
  // Select a new conversation
  onSelectConversationUuid: (id: string | undefined) => unknown
}

export interface AIChatContext extends Props {
  visibleConversations: mojom.Conversation[]
  availableAssociatedContent: mojom.WebSiteInfoDetail[]
  hasAcceptedAgreement: boolean
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  canShowPremiumPrompt?: boolean
  isMobile: boolean
  isStandalone?: boolean
  isHistoryEnabled: boolean
  allActions: mojom.ActionGroup[]
  goPremium: () => void
  managePremium: () => void
  handleAgreeClick: () => void
  dismissPremiumPrompt: () => void
  userRefreshPremiumSession: () => void,
  uiHandler?: mojom.AIChatUIHandlerRemote
}

const defaultContext: AIChatContext = {
  isDefaultConversation: true,
  visibleConversations: [],
  availableAssociatedContent: [],
  hasAcceptedAgreement: Boolean(loadTimeData.getBoolean('hasAcceptedAgreement')),
  isPremiumStatusFetching: true,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  canShowPremiumPrompt: undefined,
  isMobile: Boolean(loadTimeData.getBoolean('isMobile')),
  isHistoryEnabled: Boolean(loadTimeData.getBoolean('isHistoryEnabled')),
  allActions: [],
  goPremium: () => {},
  managePremium: () => {},
  handleAgreeClick: () => {},
  dismissPremiumPrompt: () => {},
  userRefreshPremiumSession: () => {},
  onNewConversation: () => {},
  onNewMultiTabConversation: () => {},
  onSelectConversationUuid: () => {}
}

export const AIChatReactContext =
  React.createContext<AIChatContext>(defaultContext)

export function AIChatContextProvider(props: React.PropsWithChildren<Props>) {
  const [context, setContext] = React.useState<AIChatContext>(defaultContext)

  const setPartialContext = (partialContext: Partial<AIChatContext>) => {
    setContext((value) => ({
      ...value,
      ...partialContext
    }))
  }

  React.useEffect(() => {
    const { Service, Observer } = getAPI()
    async function initialize() {
      const [
        { conversations: visibleConversations },
        { actionList: allActions },
        { availableContent: availableAssociatedContent },
        { canShow: canShowPremiumPrompt }
      ] = await Promise.all([
        Service.getVisibleConversations(),
        Service.getActionMenuList(),
        Service.getAvailableContent(),
        Service.getCanShowPremiumPrompt()
      ])
      setPartialContext({
        visibleConversations,
        availableAssociatedContent,
        allActions,
        canShowPremiumPrompt
      })
    }

    async function updateCurrentPremiumStatus () {
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

    Observer.onAvailableContentChanged.addListener((availableAssociatedContent: mojom.WebSiteInfoDetail[]) => {
      setPartialContext({
        availableAssociatedContent
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
    isStandalone: getAPI().isStandalone,
    goPremium: () => UIHandler.goPremium(),
    managePremium: () => UIHandler.managePremium(),
    dismissPremiumPrompt: () => Service.dismissPremiumPrompt(),
    userRefreshPremiumSession: () => UIHandler.refreshPremiumSession(),
    handleAgreeClick: () => Service.markAgreementAccepted(),
    uiHandler: UIHandler
  }

  console.log('content', store.availableAssociatedContent)

  return (
    <AIChatReactContext.Provider value={store}>
      {props.children}
    </AIChatReactContext.Provider>
  )
}

export function useAIChat() {
  return React.useContext(AIChatReactContext)
}

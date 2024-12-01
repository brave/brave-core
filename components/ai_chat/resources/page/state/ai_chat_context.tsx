// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import getAPI, * as AIChat from '../api'
import useMediaQuery from '$web-common/useMediaQuery'

type AIChatContextInternal = {
  initialized: boolean
  goPremium: () => void
  managePremium: () => void
  handleAgreeClick: () => void
  enableStoragePref: () => void
  markStorageNoticeViewed: () => void
  dismissStorageNotice: () => void
  dismissPremiumPrompt: () => void
  userRefreshPremiumSession: () => void
  uiHandler?: AIChat.AIChatUIHandlerRemote

  editingConversationId: string | null
  setEditingConversationId: (uuid: string | null) => void,

  showSidebar: boolean,
  toggleSidebar: () => void
}

export type AIChatContext = AIChat.State & AIChatContextInternal

const defaultContext: AIChatContext = {
  ...AIChat.defaultUIState,
  initialized: false,
  goPremium: () => { },
  managePremium: () => { },
  handleAgreeClick: () => { },
  enableStoragePref: () => { },
  markStorageNoticeViewed: () => { },
  dismissStorageNotice: () => { },
  dismissPremiumPrompt: () => { },
  userRefreshPremiumSession: () => { },

  editingConversationId: null,
  setEditingConversationId: () => { },

  showSidebar: false,
  toggleSidebar: () => { }
}

export const AIChatReactContext =
  React.createContext<AIChatContext>(defaultContext)

export function useIsSmall() {
  return useMediaQuery('(max-width: 1024px)') || defaultContext.isMobile
}

export function AIChatContextProvider(props: React.PropsWithChildren) {
  // Intialize with global state that may have been set between module-load
  // time and the first React render.
  const [context, setContext] = React.useState<AIChatContext>({
    ...defaultContext,
    ...getAPI().state
  })
  const [editingConversationId, setEditingConversationId] = React.useState<string | null>(null)
  const isSmall = useIsSmall()
  const [showSidebar, setShowSidebar] = React.useState(isSmall)

  const updateFromAPIState = (state: AIChat.State) => {
    setContext((value) => ({
      ...value,
      ...state
    }))
  }

  React.useEffect(() => {
    // Update with any global state change that may have occurred between
    // first React render and first useEffect run.
    updateFromAPIState(getAPI().state)

    // Listen for global state changes that occur after now
    const onGlobalStateChange = () => {
      updateFromAPIState(getAPI().state)
    }
    getAPI().addStateChangeListener(onGlobalStateChange)

    return () => {
      getAPI().removeStateChangeListener(onGlobalStateChange)
    }
  }, [])

  const { service, uiHandler } = getAPI()

  const store: AIChatContext = {
    ...context,
    ...props,
    goPremium: () => uiHandler.goPremium(),
    managePremium: () => uiHandler.managePremium(),
    markStorageNoticeViewed: () => service.dismissStorageNotice(),
    dismissStorageNotice: () => {
      getAPI().setPartialState({
        isStorageNoticeDismissed: true
      })
      service.dismissStorageNotice()
    },
    enableStoragePref: () => service.enableStoragePref(),
    dismissPremiumPrompt: () => service.dismissPremiumPrompt(),
    userRefreshPremiumSession: () => uiHandler.refreshPremiumSession(),
    handleAgreeClick: () => service.markAgreementAccepted(),
    uiHandler,
    editingConversationId,
    setEditingConversationId,
    showSidebar,
    toggleSidebar: () => setShowSidebar(s => !s)
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

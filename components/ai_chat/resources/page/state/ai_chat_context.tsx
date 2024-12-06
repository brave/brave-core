// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import useAPIState from '../../common/useAPIState'
import * as Mojom from '../../common/mojom'
import getAPI, * as AIChat from '../api'
import useMediaQuery from '$web-common/useMediaQuery'

export interface ConversationEntriesProps {
  onLoad: () => unknown
  onHeightChanged: () => unknown
}

type AIChatContextProps = {
  conversationEntriesComponent: (props: ConversationEntriesProps) => React.ReactElement
}

type AIChatContextInternal = AIChatContextProps & {
  initialized: boolean
  goPremium: () => void
  managePremium: () => void
  handleAgreeClick: () => void
  enableStoragePref: () => void
  markStorageNoticeViewed: () => void
  dismissStorageNotice: () => void
  dismissPremiumPrompt: () => void
  userRefreshPremiumSession: () => void
  uiHandler?: Mojom.AIChatUIHandlerRemote
  service?: Mojom.ServiceRemote

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
  toggleSidebar: () => { },

  conversationEntriesComponent: () => <></>
}

export const AIChatReactContext =
  React.createContext<AIChatContext>(defaultContext)

export function useIsSmall() {
  return useMediaQuery('(max-width: 1024px)')
}

export function AIChatContextProvider(props: React.PropsWithChildren<AIChatContextProps>) {
  const api = getAPI()
  const context = useAPIState(api, defaultContext)
  const [editingConversationId, setEditingConversationId] =
    React.useState<string | null>(null)
  const isSmall = useIsSmall()
  const [showSidebar, setShowSidebar] = React.useState(isSmall)

  const store: AIChatContext = {
    ...context,
    goPremium: () => api.uiHandler.goPremium(),
    managePremium: () => api.uiHandler.managePremium(),
    markStorageNoticeViewed: () => api.service.dismissStorageNotice(),
    dismissStorageNotice: () => {
      api.setPartialState({
        isStorageNoticeDismissed: true
      })
      api.service.dismissStorageNotice()
    },
    enableStoragePref: () => api.service.enableStoragePref(),
    dismissPremiumPrompt: () => api.service.dismissPremiumPrompt(),
    userRefreshPremiumSession: () => api.uiHandler.refreshPremiumSession(),
    handleAgreeClick: () => api.service.markAgreementAccepted(),
    uiHandler: api.uiHandler,
    service: api.service,
    editingConversationId,
    setEditingConversationId,
    showSidebar,
    toggleSidebar: () => setShowSidebar(s => !s),
    conversationEntriesComponent: props.conversationEntriesComponent
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

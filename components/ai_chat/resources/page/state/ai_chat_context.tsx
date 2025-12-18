// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import generateReactContextForAPI from '$web-common/api/react_api'
import { loadTimeData } from '$web-common/loadTimeData'
import useMediaQuery from '$web-common/useMediaQuery'
import * as Mojom from '../../common/mojom'
import { AIChatAPI } from '../api'

export interface ConversationEntriesProps {
  onIsContentReady: (isContentReady: boolean) => void
}

type AIChatContextProps = {
  api: AIChatAPI['api']
  conversationEntriesComponent: (
    props: ConversationEntriesProps,
  ) => React.ReactElement
}

export function useIsSmall() {
  return useMediaQuery('(max-width: 1024px)')
}

export default function useProvideAIChatContext(props: AIChatContextProps) {
  // This hook should only have any state or anything
  // worth memoizing across the app.
  // Anything that changes will cause the entire tree underneath
  // to re-render.
  const { api } = props

  const [editingConversationId, setEditingConversationId] = React.useState<
    string | null
  >(null)
  const [deletingConversationId, setDeletingConversationId] = React.useState<
    string | null
  >(null)
  const isSmall = useIsSmall()
  const [showSidebar, setShowSidebar] = React.useState(isSmall)
  const [skillDialog, setSkillDialog] = React.useState<Mojom.Skill | null>(null)

  const { getConversationsData, isPlaceholderData: isConversationsLoading } =
    api.useGetConversations()

  const [defaultTabContentId, setDefaultTabContentId] = React.useState<number>()

  api.useOnNewDefaultConversation((contentId) => {
    setDefaultTabContentId(contentId)
  })

  const store = {
    api: props.api,
    initialized:
      api.isStandalone.current !== undefined && !isConversationsLoading,
    isMobile: loadTimeData.getBoolean('isMobile'),
    isHistoryFeatureEnabled: loadTimeData.getBoolean('isHistoryEnabled'),
    isAIChatAgentProfileFeatureEnabled: loadTimeData.getBoolean(
      'isAIChatAgentProfileFeatureEnabled',
    ),
    isAIChatAgentProfile: loadTimeData.getBoolean('isAIChatAgentProfile'),

    // TODO(petemill): consumers should consume directly from
    // api's hooks for better performance, so that every component
    // is not re-rendered when the state of every endpoint changes.

    /**
     * @deprecated use api.useTabs() instead
     */
    tabs: api.useTabs().data!,

    /**
     * @deprecated use api.useSkills() instead
     */
    skills: api.useGetSkills().data!,

    /**
     * @deprecated use api.useState() instead
     */
    ...api.useState().data!,

    /**
     * @deprecated use api.useGetPremiumStatus() instead
     */
    ...api.useGetPremiumStatus().data!,

    /**
     * @deprecated use api.useGetPremiumStatus() instead
     */
    actionList: api.useGetActionMenuList().data,

    isStandalone: api.useIsStandalone().data,

    /**
     * @deprecated use api.useGetConversations() instead
     */
    conversations: getConversationsData,

    /**
     * @deprecated use api.[action] directly instead
     */
    goPremium: () => api.actions.uiHandler.goPremium(),

    /**
     * @deprecated use api.[action] directly instead
     */
    managePremium: () => api.actions.uiHandler.managePremium(),

    /**
     * @deprecated use api.[action] directly instead
     */
    dismissStorageNotice: () => api.actions.service.dismissStorageNotice(),

    /**
     * @deprecated use api.[action] directly instead
     */
    enableStoragePref: () => api.actions.service.enableStoragePref(),

    /**
     * @deprecated use api.[action] directly instead
     */
    dismissPremiumPrompt: () => api.actions.service.dismissPremiumPrompt(),

    /**
     * @deprecated use api.[action] directly instead
     */
    userRefreshPremiumSession: () =>
      api.actions.uiHandler.refreshPremiumSession(),

    /**
     * @deprecated use api.[action] directly instead
     */
    handleAgreeClick: () => api.actions.service.markAgreementAccepted(),

    /**
     *
     * @deprecated use api.endpoints.useGetPluralString directly instead
     */
    getPluralString(key: string, amount: number) {
      return api.endpoints.getPluralString.fetch(key, amount)
    },

    // Note: we might want to show progress during image processing,
    // and we can do that via monitoring the mutation in the provided hook.
    processImageFile: api.endpoints.processImageFile.mutate,

    /**
     * @deprecated use api.actions.uiHandler.openAIChatAgentProfile directly instead
     */
    openAIChatAgentProfile: () =>
      api.actions.uiHandler.openAIChatAgentProfile(),

    /**
     * @deprecated use api.actions.uiHandler.openURL directly instead
     */
    openURL: api.actions.uiHandler.openURL,

    defaultTabContentId,
    editingConversationId,
    setEditingConversationId,
    deletingConversationId,
    setDeletingConversationId,
    showSidebar,
    toggleSidebar: () => setShowSidebar((s) => !s),
    skillDialog,
    setSkillDialog,
    conversationEntriesComponent: props.conversationEntriesComponent,
  }

  return store
}

export type AIChatContext = ReturnType<typeof useProvideAIChatContext>

export const { useAPI: useAIChat, Provider: AIChatProvider } =
  generateReactContextForAPI<AIChatContextProps, AIChatContext>(
    useProvideAIChatContext,
  )

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import generateReactContext from '$web-common/api/react_api'
import { UntrustedConversationAPI } from './api/untrusted_conversation_api'
import { loadTimeData } from '$web-common/loadTimeData'

// Props required to provide the context
export interface UntrustedConversationContextProps {
  api: UntrustedConversationAPI
}

const IS_MOBILE = loadTimeData.getBoolean('isMobile')

/**
 * Provides the untrusted conversation context with API hooks
 */
export function useProvideUntrustedConversationContext(
  props: UntrustedConversationContextProps,
) {
  const { api } = props

  // Use API hooks for state
  const state = api.useState().data
  const conversationHistory = api.useGetConversationHistoryData()

  const associatedContent = api.useCurrentAssociatedContentChanged().data?.[0]
  const contentTaskTabId = api.useCurrentContentTaskStarted().data?.[0]

  React.useEffect(
    () => console.log('event data', { associatedContent, contentTaskTabId }),
    [associatedContent, contentTaskTabId],
  )

  return {
    api,

    // Expose state properties directly for backwards compatibility
    // These are marked as deprecated - components should use api.useState() instead
    /**
     * @deprecated Use `api.useGetConversationHistory().data` instead
     */
    conversationHistory,

    /**
     * @deprecated Use `api.useState().data.isGenerating` instead
     */
    isGenerating: state.isGenerating,

    /**
     * @deprecated Use `api.useState().data.isToolExecuting` instead
     */
    isToolExecuting: state.isToolExecuting,

    /**
     * @deprecated Use `api.useState().data.toolUseTaskState` instead
     */
    toolUseTaskState: state.toolUseTaskState,

    /**
     * @deprecated Use `api.useState().data.isPremiumUser` instead
     */
    isPremiumUser: state.isPremiumUser,

    /**
     * @deprecated Use `api.useState().data.isLeoModel` instead
     */
    isLeoModel: state.isLeoModel,

    /**
     * @deprecated Use `api.useState().data.allModels` instead
     */
    allModels: state.allModels,

    /**
     * @deprecated Use `api.useState().data.currentModelKey` instead
     */
    currentModelKey: state.currentModelKey,

    /**
     * @deprecated Use `api.useState().data.contentUsedPercentage` instead
     */
    contentUsedPercentage: state.contentUsedPercentage,

    /**
     * @deprecated Use `api.useState().data.visualContentUsedPercentage` instead
     */
    visualContentUsedPercentage: state.visualContentUsedPercentage,

    /**
     * @deprecated Use `api.useState().data.trimmedTokens` instead
     */
    trimmedTokens: state.trimmedTokens,

    /**
     * @deprecated Use `api.useState().data.totalTokens` instead
     */
    totalTokens: state.totalTokens,

    /**
     * @deprecated Use `api.useState().data.canSubmitUserEntries` instead
     */
    canSubmitUserEntries: state.canSubmitUserEntries,

    /**
     * @deprecated Use `api.useState().data.isMobile` instead
     */
    isMobile: IS_MOBILE,

    /**
     * @deprecated Use `api.useCurrentAssociatedContentChanged().data` or
     * subscribe to `api.useAssociatedContentChanged()` directly.
     */
    associatedContent: associatedContent,

    /**
     * @deprecated Use `api.useState().data.conversationCapability` instead
     */
    conversationCapability: state.conversationCapability,

    /**
     * @deprecated Use `api.useCurrentContentTaskStarted().data` or subscribe to
     * `api.useContentTaskStarted()` directly instead
     */
    contentTaskTabId: contentTaskTabId,

    // Expose action handlers via convenient aliases
    // These are deprecated - use api.actions directly instead

    /**
     * @deprecated Use `api.actions.conversationHandler` instead
     */
    conversationHandler: api.actions.conversationHandler,

    /**
     * @deprecated Use `api.actions.uiHandler` instead
     */
    uiHandler: api.actions.uiHandler,

    /**
     * @deprecated Use `api.actions.parentUIFrame` instead
     */
    parentUiFrame: api.actions.parentUIFrame,
  }
}

export const {
  useAPI: useUntrustedConversationContext,
  Provider: UntrustedConversationContextProvider,
} = generateReactContext(useProvideUntrustedConversationContext)

export type UntrustedConversationContext = ReturnType<
  typeof useProvideUntrustedConversationContext
>

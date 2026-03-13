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

  const [
    showPremiumSuggestionForRegenerate,
    setShowPremiumSuggestionForRegenerate,
  ] = React.useState(false)

  // Use API hooks for state
  const state = api.useState().data
  const serviceState = api.useServiceState().data
  const conversationHistory = api.useGetConversationHistoryData()

  const associatedContent = api.useCurrentAssociatedContentChanged().data?.[0]
  const contentTaskTabId = api.useCurrentContentTaskStarted().data?.[0]

  return {
    api,

    showPremiumSuggestionForRegenerate,
    setShowPremiumSuggestionForRegenerate,

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
     * @deprecated Use `api.useGetPremiumStatus().data.isPremiumUser` instead
     */
    isPremiumUser: api.useGetPremiumStatusData().isPremiumUser,

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
    associatedContent,

    /**
     * @deprecated Use `api.useState().data.conversationCapabilities` instead
     */
    conversationCapabilities: state.conversationCapabilities,

    /**
     * @deprecated Use `api.useCurrentContentTaskStarted().data` or subscribe to
     * `api.useContentTaskStarted()` directly instead
     */
    contentTaskTabId,

    /**
     * @deprecated Use `api.useState().data.suggestedQuestions` instead
     */
    suggestedQuestions: state.suggestedQuestions,

    /**
     * @deprecated Use `api.useState().data.suggestionStatus` instead
     */
    suggestionStatus: state.suggestionStatus,

    /**
     * @deprecated Use `api.useState().data.currentError` instead
     */
    currentError: state.currentError,

    /**
     * @deprecated Use `api.useState().data.isTemporary` instead
     */
    isTemporary: state.isTemporary,

    // Expose action handlers via convenient aliases
    // These are deprecated - use api.actions directly instead

    /**
     * @deprecated Use `api.actions.conversationHandler` instead
     */
    conversationHandler: api.conversationHandler,

    /**
     * @deprecated Use `api.actions.uiHandler` instead
     */
    uiHandler: api.uiHandler,

    /**
     * @deprecated Use `api.actions.parentUIFrame` instead
     */
    parentUiFrame: api.parentUIFrame,

    /**
     * @deprecated Use `api.actions.service` instead
     */
    service: api.service,

    // Service state (profile-level)
    /**
     * @deprecated Use `api.useServiceState().data.hasAcceptedAgreement` instead
     */
    hasAcceptedAgreement: serviceState.hasAcceptedAgreement,

    /**
     * @deprecated Use `api.useServiceState().data.isStoragePrefEnabled` instead
     */
    isStoragePrefEnabled: serviceState.isStoragePrefEnabled,

    /**
     * @deprecated Use `api.useServiceState().data.isStorageNoticeDismissed` instead
     */
    isStorageNoticeDismissed: serviceState.isStorageNoticeDismissed,

    /**
     * @deprecated Use `api.useServiceState().data.canShowPremiumPrompt` instead
     */
    canShowPremiumPrompt: serviceState.canShowPremiumPrompt,

    /**
     * @deprecated Use `api.useGetPremiumStatus().data.isPremiumUserDisconnected` instead
     */
    isPremiumUserDisconnected:
      api.useGetPremiumStatusData().isPremiumUserDisconnected,
  }
}

export const {
  useAPI: useUntrustedConversationContext,
  Provider: UntrustedConversationContextProvider,
} = generateReactContext(useProvideUntrustedConversationContext)

export type UntrustedConversationContext = ReturnType<
  typeof useProvideUntrustedConversationContext
>

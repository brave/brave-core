// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as mojom from '../api/page_handler'

export interface AIChatContext {
  allModels: mojom.Model[]
  currentModel?: mojom.Model
  conversationHistory: mojom.ConversationTurn[]
  suggestedQuestions: string[]
  isGenerating: boolean
  suggestionStatus: mojom.SuggestionGenerationStatus
  hasAcceptedAgreement: boolean
  siteInfo: mojom.SiteInfo
  favIconUrl: string | undefined
  currentError: mojom.APIError | undefined
  apiHasError: boolean
  shouldDisableUserInput: boolean
  isPremiumStatusFetching: boolean
  isPremiumUser: boolean
  isPremiumUserDisconnected: boolean
  canShowPremiumPrompt?: boolean
  shouldShowLongPageWarning: boolean
  shouldShowLongConversationInfo: boolean
  showAgreementModal: boolean
  shouldSendPageContents: boolean
  isMobile: boolean
  inputText: string
  isCharLimitExceeded: boolean
  isCharLimitApproaching: boolean
  inputTextCharCountDisplay: string
  selectedActionType: mojom.ActionType | undefined
  isToolsMenuOpen: boolean
  actionList: mojom.ActionGroup[]
  setCurrentModel: (model: mojom.Model) => void,
  switchToBasicModel: () => void,
  generateSuggestedQuestions: () => void
  goPremium: () => void
  managePremium: () => void
  handleAgreeClick: () => void
  dismissPremiumPrompt: () => void
  getCanShowPremiumPrompt: () => void
  userRefreshPremiumSession: () => void
  dismissLongConversationInfo: () => void
  updateShouldSendPageContents: (shouldSend: boolean) => void
  setInputText: (text: string) => void
  handleMaybeLater: () => void
  handleSwitchToBasicModelAndRetry: () => void
  submitInputTextToAPI: () => void
  resetSelectedActionType: () => void
  handleActionTypeClick: (actionType: mojom.ActionType) => void
  setIsToolsMenuOpen: (isOpen: boolean) => void
}

export const defaultContext: AIChatContext = {
  allModels: [],
  conversationHistory: [],
  suggestedQuestions: [],
  isGenerating: false,
  suggestionStatus: mojom.SuggestionGenerationStatus.None,
  hasAcceptedAgreement: false,
  apiHasError: false,
  shouldDisableUserInput: false,
  isPremiumStatusFetching: false,
  isPremiumUser: false,
  isPremiumUserDisconnected: false,
  siteInfo: new mojom.SiteInfo(),
  favIconUrl: undefined,
  currentError: mojom.APIError.None,
  canShowPremiumPrompt: undefined,
  shouldShowLongPageWarning: false,
  shouldShowLongConversationInfo: false,
  showAgreementModal: false,
  shouldSendPageContents: true,
  isMobile: false,
  inputText: '',
  isCharLimitExceeded: false,
  isCharLimitApproaching: false,
  inputTextCharCountDisplay: '',
  selectedActionType: undefined,
  isToolsMenuOpen: false,
  actionList: [],
  setCurrentModel: () => {},
  switchToBasicModel: () => {},
  generateSuggestedQuestions: () => {},
  goPremium: () => {},
  managePremium: () => {},
  handleAgreeClick: () => {},
  dismissPremiumPrompt: () => {},
  getCanShowPremiumPrompt: () => {},
  userRefreshPremiumSession: () => {},
  dismissLongConversationInfo: () => {},
  updateShouldSendPageContents: () => {},
  setInputText: () => {},
  handleMaybeLater: () => {},
  handleSwitchToBasicModelAndRetry: () => {},
  submitInputTextToAPI: () => {},
  resetSelectedActionType: () => {},
  handleActionTypeClick: () => {},
  setIsToolsMenuOpen: () => {},
}

export default React.createContext<AIChatContext>(defaultContext)

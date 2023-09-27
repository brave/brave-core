// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as mojom from '../api/page_handler'

export interface AIChatContext {
  allModels: mojom.Model[]
  currentModel?: mojom.Model
  hasChangedModel: boolean
  conversationHistory: mojom.ConversationTurn[]
  suggestedQuestions: string[]
  isGenerating: boolean
  canGenerateQuestions: boolean
  hasSeenAgreement: boolean
  userAutoGeneratePref: mojom.AutoGenerateQuestionsPref | undefined
  siteInfo: mojom.SiteInfo | null
  favIconUrl: string | undefined
  currentError: mojom.APIError | undefined
  apiHasError: boolean
  shouldDisableUserInput: boolean
  setCurrentModel: (model: mojom.Model) => void,
  generateSuggestedQuestions: () => void
  setUserAllowsAutoGenerating: (value: boolean) => void
  handleAgreeClick: () => void
}

export const defaultContext = {
  allModels: [],
  hasChangedModel: false,
  conversationHistory: [],
  suggestedQuestions: [],
  isGenerating: false,
  canGenerateQuestions: false,
  hasSeenAgreement: false,
  apiHasError: false,
  shouldDisableUserInput: false,
  userAutoGeneratePref: undefined,
  siteInfo: null,
  favIconUrl: undefined,
  currentError: mojom.APIError.None,
  setCurrentModel: () => {},
  generateSuggestedQuestions: () => {},
  setUserAllowsAutoGenerating: () => {},
  handleAgreeClick: () => {}
}

export default React.createContext<AIChatContext>(defaultContext)

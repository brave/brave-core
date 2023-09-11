// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ConversationTurn, AutoGenerateQuestionsPref, SiteInfo, APIError } from '../api/page_handler'

interface Store {
  conversationHistory: ConversationTurn[]
  suggestedQuestions: string[]
  isGenerating: boolean
  canGenerateQuestions: boolean
  hasSeenAgreement: boolean
  userAutoGeneratePref: AutoGenerateQuestionsPref | undefined
  siteInfo: SiteInfo | null
  favIconUrl: string | undefined
  currentError: APIError | undefined
  generateSuggestedQuestions: () => void
  setUserAllowsAutoGenerating: (value: boolean) => void
  handleAgreeClick: () => void
}

const defaultStore = {
  conversationHistory: [],
  suggestedQuestions: [],
  isGenerating: false,
  canGenerateQuestions: false,
  hasSeenAgreement: false,
  userAutoGeneratePref: undefined,
  siteInfo: null,
  favIconUrl: undefined,
  currentError: undefined,
  generateSuggestedQuestions: () => {},
  setUserAllowsAutoGenerating: () => {},
  handleAgreeClick: () => {}
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext

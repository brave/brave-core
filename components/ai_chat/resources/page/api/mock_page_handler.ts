/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

// Implementing from the raw class (mojom.PageHandlerRemote) gives an error: "Types have separate declarations of a private property 'proxy'".
// General guidance is to use the public interface only as the underlying type hides private fields: https://github.com/microsoft/TypeScript/issues/7755
// We use a mapped type to extract only the public fields of a class.
// Also, we don't care about private/protected fields here as this is just a mock.

type Public<T> = { [P in keyof T]: T[P] }

const ACTIONS_LIST: mojom.ActionGroup[] = [
  {
    category: 'Quick actions',
    entries: [
      {
        subheading: undefined,
        details: { label: 'Explain', type: mojom.ActionType.EXPLAIN }
      }
    ]
  },
  {
    category: 'Rewrite',
    entries: [
      {
        subheading: undefined,
        details: { label: 'Paraphrase', type: mojom.ActionType.PARAPHRASE }
      },
      { subheading: 'Change tone', details: undefined },
      {
        subheading: undefined,
        details: {
          label: 'Change tone / Academic',
          type: mojom.ActionType.ACADEMICIZE
        }
      },
      {
        subheading: undefined,
        details: {
          label: 'Change tone / Professional',
          type: mojom.ActionType.PROFESSIONALIZE
        }
      }
    ]
  }
]
export class MockPageHandlerRemote implements Public<mojom.PageHandlerRemote> {
  constructor() {}

  $() {}

  getModels() {
    return Promise.resolve({ models: [], currentModelKey: 'chat-basic' })
  }

  getConversationHistory() {
    return Promise.resolve({ conversationHistory: [] })
  }

  getSiteInfo(): Promise<{ siteInfo: mojom.SiteInfo }> {
    return Promise.resolve({ siteInfo: new mojom.SiteInfo() })
  }

  getFaviconImageData(): Promise<{ faviconImageData: number[] | null }> {
    return Promise.resolve({ faviconImageData: null })
  }

  getAPIResponseError(): Promise<{ error: number }> {
    return Promise.resolve({ error: 0 })
  }

  getCanShowPremiumPrompt(): Promise<{ canShow: boolean }> {
    return Promise.resolve({ canShow: true })
  }

  getPremiumStatus(): Promise<{
    status: number
    info: mojom.PremiumInfo | null
  }> {
    return Promise.resolve({
      status: 0,
      info: { remainingCredentialCount: 2, nextActiveAt: undefined }
    })
  }

  getShouldSendPageContents(): Promise<{ shouldSend: boolean }> {
    return Promise.resolve({ shouldSend: true })
  }

  getSuggestedQuestions() {
    return Promise.resolve({ questions: [], suggestionStatus: 1 })
  }

  clearErrorAndGetFailedMessage() {
    return Promise.resolve({ turn: new mojom.ConversationTurn() })
  }

  rateMessage() {
    return Promise.resolve({ ratingId: null })
  }

  sendFeedback() {
    return Promise.resolve({ isSuccess: true })
  }

  getActionMenuList(): Promise<{ actionList: mojom.ActionGroup[] }> {
    return Promise.resolve({ actionList: ACTIONS_LIST })
  }

  openURL(url: Url) {
    window.open(url.url, '_blank')
  }

  onConnectionError() {}
  setClientPage() {}
  changeModel() {}
  getConversationhistory() {}
  submitHumanConversationEntry() {}
  submitHumanConversationEntryWithAction() {}
  handleVoiceRecognition() {}
  submitSummarizationRequest() {}
  markAgreementAccepted() {}
  openBraveLeoSettings() {}
  generateQuestions() {}
  setShouldSendPageContents() {}
  goPremium() {}
  refreshPremiumSession() {}
  clearConversationHistory() {}
  managePremium() {}
  retryAPIRequest() {}
  dismissPremiumPrompt() {}
  closePanel() {}
  openModelSupportUrl() {}
}

const router = {
  addListener: () => {}
}

export class MockPageHandlerAPI {
  pageHandler: Public<mojom.PageHandlerRemote>
  callbackRouter: any

  constructor() {
    this.pageHandler = new MockPageHandlerRemote()
    this.callbackRouter = {}
    this.callbackRouter.onConversationHistoryUpdate = router
    this.callbackRouter.onAPIRequestInProgress = router
    this.callbackRouter.onFaviconImageDataChanged = router
    this.callbackRouter.onSiteInfoChanged = router
    this.callbackRouter.onAPIResponseError = router
    this.callbackRouter.onModelChanged = router
    this.callbackRouter.onSuggestedQuestionsChanged = router
  }
}

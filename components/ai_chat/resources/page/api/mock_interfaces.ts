// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Mock Mojo interfaces for Storybook stories and unit tests.
 *
 * These mocks provide configurable default implementations for all interface
 * methods.
 *
 * @example
 * // Basic usage with defaults
 * const mockService = createMockService()
 *
 * // With overrides for specific behavior
 * const mockService = createMockService({
 *   getHistory: (query, maxResults) => Promise.resolve({
 *     history: query ? filteredHistory : allHistory
 *   }),
 *   getPremiumStatus: () => Promise.resolve({
 *     status: Mojom.PremiumStatus.Active,
 *     info: null,
 *   }),
 * })
 */

import { Closable, makeCloseable } from '$web-common/api'
import * as Mojom from '../../common/mojom'

export const defaultServiceState: Mojom.ServiceState = {
  hasAcceptedAgreement: false,
  isStoragePrefEnabled: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: false,
}

export const defaultConversationState: Mojom.ConversationState = {
  conversationUuid: 'test-conversation',
  isRequestInProgress: false,
  currentModelKey: 'test-model',
  defaultModelKey: 'test-model',
  allModels: [],
  suggestedQuestions: [],
  suggestionStatus: Mojom.SuggestionGenerationStatus.None,
  associatedContent: [],
  error: Mojom.APIError.None,
  temporary: false,
  toolUseTaskState: Mojom.TaskState.kNone,
}

const emptyTurn: Mojom.ConversationTurn = {
  uuid: '',
  text: '',
  characterType: Mojom.CharacterType.HUMAN,
  actionType: Mojom.ActionType.UNSPECIFIED,
  prompt: undefined,
  selectedText: undefined,
  edits: [],
  createdTime: { internalValue: BigInt(0) },
  events: [],
  uploadedFiles: [],
  fromBraveSearchSERP: false,
  skill: undefined,
  modelKey: '',
  nearVerificationStatus: undefined,
}

/**
 * Creates a mock ConversationHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockConversationHandler(
  overrides: Partial<Mojom.ConversationHandlerInterface> = {},
  initialState: Partial<Mojom.ConversationState> = {},
): Closable<Mojom.ConversationHandlerInterface> {
  return makeCloseable({
    // Query methods - return empty/default results
    getState: () =>
      Promise.resolve({
        conversationState: {
          ...defaultConversationState,
          ...initialState,
        },
      }),
    getConversationHistory: () => Promise.resolve({ conversationHistory: [] }),
    getConversationUuid: () => Promise.resolve({ conversationUuid: '' }),
    getModels: () =>
      Promise.resolve({
        models: [] as Mojom.Model[],
        currentModelKey: '',
      }),
    getIsRequestInProgress: () =>
      Promise.resolve({ isRequestInProgress: false }),
    getAssociatedContentInfo: () => Promise.resolve({ associatedContent: [] }),
    getAPIResponseError: () => Promise.resolve({ error: Mojom.APIError.None }),

    // Mutation methods - return empty/default results
    getScreenshots: () => Promise.resolve({ screenshots: [] }),
    clearErrorAndGetFailedMessage: () => Promise.resolve({ turn: emptyTurn }),
    setTemporary: () => Promise.resolve({}),
    stopGenerationAndMaybeGetHumanEntry: () =>
      Promise.resolve({ humanEntry: null }),
    rateMessage: () => Promise.resolve({ ratingId: null }),
    sendFeedback: () => Promise.resolve({ isSuccess: true }),

    // Action methods - fire and forget stubs
    generateQuestions: () => { },
    submitSummarizationRequest: () => { },
    submitSuggestion: () => { },
    retryAPIRequest: () => { },
    changeModel: () => { },
    submitHumanConversationEntryWithAction: () => { },
    submitHumanConversationEntryWithSkill: () => { },
    submitHumanConversationEntry: () => { },
    pauseTask: () => { },
    resumeTask: () => { },
    stopTask: () => { },

    // Apply overrides - these will replace defaults
    ...overrides,
  })
}

/**
 * Creates a mock ServiceInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockService(
  overrides: Partial<Mojom.ServiceInterface> = {},
  initialState: Partial<Mojom.ServiceState> = {},
): Closable<Mojom.ServiceInterface> {
  return makeCloseable({
    // Query methods - return empty/default results
    getConversations: () => Promise.resolve({ conversations: [] }),
    getConversationData: () => Promise.resolve({ conversation: null, entries: [] }),
    getActionMenuList: () => Promise.resolve({ actionList: [] }),
    getSkills: () => Promise.resolve({ skills: [] }),
    getPremiumStatus: () =>
      Promise.resolve({ status: Mojom.PremiumStatus.Inactive, info: null }),

    // Action methods - fire and forget stubs
    markAgreementAccepted: () => { },
    enableStoragePref: () => { },
    dismissStorageNotice: () => { },
    dismissPremiumPrompt: () => { },
    bindConversation: () => { },
    deleteConversation: () => { },
    renameConversation: () => { },
    conversationExists: () => Promise.resolve({ exists: true }),
    createSkill: () => { },
    updateSkill: () => { },
    deleteSkill: () => { },
    bindObserver: () =>
      Promise.resolve({
        state: {
          ...defaultServiceState,
          ...initialState,
        },
      }),
    bindMetrics: () => { },

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock AIChatUIHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockUIHandler(
  overrides: Partial<Mojom.AIChatUIHandlerInterface> = {},
): Closable<Mojom.AIChatUIHandlerInterface> {
  return makeCloseable({
    // Mutation methods - return empty/default results
    uploadFile: () => Promise.resolve({ uploadedFiles: [] }),
    processImageFile: () =>
      Promise.resolve({
        processedFile: {
          filename: '',
          filesize: 0,
          data: [],
          type: Mojom.UploadedFileType.kImage,
        },
      }),
    getPluralString: () => Promise.resolve({ pluralString: '' }),
    setChatUI: () => Promise.resolve({ isStandalone: false }),

    // Action methods - fire and forget stubs
    newConversation: () => { },
    bindRelatedConversation: () => { },
    openURL: () => { },
    closeUI: () => { },
    openModelSupportUrl: () => { },
    openStorageSupportUrl: () => { },
    goPremium: () => { },
    managePremium: () => { },
    refreshPremiumSession: () => { },
    handleVoiceRecognition: () => { },
    openAIChatSettings: () => { },
    openMemorySettings: () => { },
    openConversationFullPage: () => { },
    associateTab: () => { },
    associateUrlContent: () => { },
    disassociateContent: () => { },
    openAIChatAgentProfile: () => { },
    showSoftKeyboard: () => { },

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock BookmarksPageHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockBookmarksService(
  overrides: Partial<Mojom.BookmarksPageHandlerInterface> = {},
): Closable<Mojom.BookmarksPageHandlerInterface> {
  return makeCloseable({
    getBookmarks: () => Promise.resolve({ bookmarks: [] }),

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock HistoryUIHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockHistoryService(
  overrides: Partial<Mojom.HistoryUIHandlerInterface> = {},
): Closable<Mojom.HistoryUIHandlerInterface> {
  return makeCloseable({
    getHistory: () => Promise.resolve({ history: [] }),

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock MetricsInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockMetrics(
  overrides: Partial<Mojom.MetricsInterface> = {},
): Closable<Mojom.MetricsInterface> {
  return makeCloseable({
    onSendingPromptWithNTP: () => { },
    onQuickActionStatusChange: () => { },
    onSendingPromptWithFullPage: () => { },
    recordSkillClick: () => { },

    // Apply overrides
    ...overrides,
  })
}

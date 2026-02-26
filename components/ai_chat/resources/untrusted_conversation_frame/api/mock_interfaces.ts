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
 * const mockHandler = createMockUntrustedConversationHandler()
 *
 * // With overrides for specific behavior
 * const mockUIHandler = createMockUntrustedUIHandler({
 *   hasMemory: (memory) => Promise.resolve({ exists: true }),
 * })
 */

import { Closable, makeCloseable } from '$web-common/api'
import * as Mojom from '../../common/mojom'

export const defaultConversationEntriesState: Mojom.ConversationEntriesState = {
  isGenerating: false,
  isToolExecuting: false,
  toolUseTaskState: Mojom.TaskState.kNone,
  isLeoModel: true,
  allModels: [],
  currentModelKey: '',
  contentUsedPercentage: undefined,
  visualContentUsedPercentage: undefined,
  trimmedTokens: BigInt(0),
  totalTokens: BigInt(0),
  canSubmitUserEntries: false,
  conversationCapabilities: [Mojom.ConversationCapability.CHAT],
  suggestedQuestions: [],
  suggestionStatus: Mojom.SuggestionGenerationStatus.None,
  currentError: Mojom.APIError.None,
  isTemporary: false,
}

export const defaultServiceState: Mojom.ServiceState = {
  hasAcceptedAgreement: false,
  isStoragePrefEnabled: false,
  isStorageNoticeDismissed: false,
  canShowPremiumPrompt: false,
}

/**
 * Creates a mock UntrustedConversationHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockUntrustedConversationHandler(
  overrides: Partial<Mojom.UntrustedConversationHandlerInterface> = {},
): Closable<Mojom.UntrustedConversationHandlerInterface> {
  return makeCloseable({
    // Query methods - return empty/default results
    getConversationHistory: () => Promise.resolve({ conversationHistory: [] }),
    bindUntrustedConversationUI: () =>
      Promise.resolve({
        conversationEntriesState: defaultConversationEntriesState,
      }),

    // Action methods - fire and forget stubs
    modifyConversation: () => {},
    respondToToolUseRequest: () => {},
    processPermissionChallenge: () => {},
    regenerateAnswer: () => {},
    submitSuggestion: () => {},
    generateQuestions: () => {},
    retryAPIRequest: () => {},

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock UntrustedUIHandlerInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * Note: This interface is not Closable (no $.close() method).
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockUntrustedUIHandler(
  overrides: Partial<Mojom.UntrustedUIHandlerInterface> = {},
): Mojom.UntrustedUIHandlerInterface {
  return {
    // Query methods - return empty/default results
    hasMemory: () => Promise.resolve({ exists: false }),

    // Action methods - fire and forget stubs
    bindConversationHandler: () => {},
    bindUntrustedUI: () => {},
    openSearchURL: () => {},
    openLearnMoreAboutBraveSearchWithLeo: () => {},
    openURLFromResponse: () => {},
    openAIChatCustomizationSettings: () => {},
    addTabToThumbnailTracker: () => {},
    removeTabFromThumbnailTracker: () => {},
    bindParentPage: () => {},
    deleteMemory: () => {},
    goPremium: () => {},
    refreshPremiumSession: () => {},
    openModelSupportUrl: () => {},
    openStorageSupportUrl: () => {},
    openURL: () => {},

    // Apply overrides
    ...overrides,
  }
}

/**
 * Creates a mock ParentUIFrameInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockParentUIFrame(
  overrides: Partial<Mojom.ParentUIFrameInterface> = {},
): Closable<Mojom.ParentUIFrameInterface> {
  return makeCloseable({
    // Action methods - fire and forget stubs
    childHeightChanged: () => {},
    rateMessage: () => {},
    userRequestedOpenGeneratedUrl: () => {},
    dragStart: () => {},
    regenerateAnswerMenuIsOpen: () => {},
    showSkillDialog: () => {},
    showPremiumSuggestionForRegenerate: () => {},
    requestNewConversation: () => {},
    handleResetError: () => {},

    // Apply overrides
    ...overrides,
  })
}

/**
 * Creates a mock UntrustedServiceInterface for Storybook/tests.
 * All methods have sensible defaults that can be overridden.
 *
 * @param overrides - Partial implementation to override default behavior
 */
export function createMockUntrustedService(
  overrides: Partial<Mojom.UntrustedServiceInterface> = {},
): Closable<Mojom.UntrustedServiceInterface> {
  return makeCloseable({
    // Query methods - return default results
    bindObserver: () => Promise.resolve({ state: defaultServiceState }),
    getPremiumStatus: () =>
      Promise.resolve({ status: Mojom.PremiumStatus.Inactive, info: null }),

    // Action methods - fire and forget stubs
    dismissStorageNotice: () => {},
    dismissPremiumPrompt: () => {},

    // Apply overrides
    ...overrides,
  })
}

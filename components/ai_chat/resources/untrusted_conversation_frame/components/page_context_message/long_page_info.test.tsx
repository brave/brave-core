// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from '@testing-library/react'
import { UntrustedConversationReactContext } from '../../untrusted_conversation_context'
import LongPageInfo from './long_page_info'

// Mock the formatLocale function from $web-common/locale
jest.mock('$web-common/locale', () => ({
  formatLocale: (key: string, params?: Record<string, string>) => {
    if (key === 'CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING') {
      return `Only ${params?.$1} of visual content could be used`
    }
    if (key === 'CHAT_UI_TRIMMED_TOKENS_WARNING') {
      return `${params?.$1} of the content was used`
    }
    if (key === 'CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING') {
      return `${params?.$1} of the page content was used`
    }
    return key
  }
}))

const mockContext = {
  conversationHistory: [],
  conversationUuid: '',
  allModels: [],
  currentModelKey: '',
  contentUsedPercentage: undefined,
  visualContentUsedPercentage: undefined,
  trimmedTokens: BigInt(0),
  totalTokens: BigInt(0),
  canSubmitUserEntries: false,
  hasGeneratedAssistantResponse: false,
  generatePageContent: jest.fn(),
  generateSuggestions: jest.fn(),
  rateMessage: jest.fn(),
  sendFeedback: jest.fn(),
  modifyConversation: jest.fn(),
  regenerateAnswer: jest.fn(),
  setCurrentModel: jest.fn(),
  submitHumanConversationEntry: jest.fn(),
  submitSelectedTextWithAction: jest.fn(),
  retryAPIRequest: jest.fn(),
  clearAPIError: jest.fn(),
  editMessageText: jest.fn(),
  submitUserEntryWithAction: jest.fn(),
  submitSummarizationRequest: jest.fn(),
  stopGeneratingResponse: jest.fn(),
  switchToModel: jest.fn(),
  clearConversation: jest.fn(),
  closeConversation: jest.fn(),
  resetAssistantChatHistory: jest.fn(),
  getPageContent: jest.fn(),
  updateShouldDisableUserInput: jest.fn(),
}

const renderWithContext = (contextOverrides = {}) => {
  const context = { ...mockContext, ...contextOverrides }
  return render(
    <UntrustedConversationReactContext.Provider value={context}>
      <LongPageInfo />
    </UntrustedConversationReactContext.Provider>
  )
}

describe('LongPageInfo', () => {
  beforeEach(() => {
    // Mock the global S object
    global.S = {
      CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING: 'CHAT_UI_VISUAL_CONTENT_TOO_MUCH_WARNING',
      CHAT_UI_TRIMMED_TOKENS_WARNING: 'CHAT_UI_TRIMMED_TOKENS_WARNING',
      CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING: 'CHAT_UI_PAGE_CONTENT_TOO_LONG_WARNING'
    }
  })

  afterEach(() => {
    jest.restoreAllMocks()
  })

  describe('visualContentUsedPercentage handling', () => {
    test('should show visual content warning when visualContentUsedPercentage is less than 100', () => {
      const { getByText } = renderWithContext({
        visualContentUsedPercentage: 75,
        contentUsedPercentage: 85,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      expect(getByText('Only 75% of visual content could be used')).toBeInTheDocument()
    })

    test('should show visual content warning when visualContentUsedPercentage is 0', () => {
      const { getByText } = renderWithContext({
        visualContentUsedPercentage: 0,
        contentUsedPercentage: 100,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      expect(getByText('Only 0% of visual content could be used')).toBeInTheDocument()
    })

    test('should not show visual content warning when visualContentUsedPercentage is 100', () => {
      const { queryByText, getByText } = renderWithContext({
        visualContentUsedPercentage: 100,
        contentUsedPercentage: 80,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // Should show page content warning instead
      expect(queryByText(/visual content could be used/)).not.toBeInTheDocument()
      expect(getByText('80% of the page content was used')).toBeInTheDocument()
    })

    test('should not show visual content warning when visualContentUsedPercentage is undefined', () => {
      const { queryByText, getByText } = renderWithContext({
        visualContentUsedPercentage: undefined,
        contentUsedPercentage: 90,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // Should show page content warning instead (undefined defaults to 100)
      expect(queryByText(/visual content could be used/)).not.toBeInTheDocument()
      expect(getByText('90% of the page content was used')).toBeInTheDocument()
    })
  })

  describe('warning priority order', () => {
    test('should prioritize trimmed tokens warning over visual content warning', () => {
      const { queryByText, getByText } = renderWithContext({
        visualContentUsedPercentage: 50,
        contentUsedPercentage: 80,
        trimmedTokens: BigInt(100),
        totalTokens: BigInt(1000),
      })

      // Should show trimmed tokens warning (90% used)
      expect(getByText('90% of the content was used')).toBeInTheDocument()
      expect(queryByText(/visual content could be used/)).not.toBeInTheDocument()
    })

    test('should prioritize visual content warning over page content warning', () => {
      const { queryByText, getByText } = renderWithContext({
        visualContentUsedPercentage: 60,
        contentUsedPercentage: 70,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // Should show visual content warning
      expect(getByText('Only 60% of visual content could be used')).toBeInTheDocument()
      expect(queryByText(/page content was used/)).not.toBeInTheDocument()
    })

    test('should show page content warning when no other warnings apply', () => {
      const { getByText } = renderWithContext({
        visualContentUsedPercentage: 100,
        contentUsedPercentage: 85,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      expect(getByText('85% of the page content was used')).toBeInTheDocument()
    })
  })

  describe('edge cases', () => {
    test('should handle null visualContentUsedPercentage (defaults to 100)', () => {
      const { queryByText, getByText } = renderWithContext({
        visualContentUsedPercentage: null,
        contentUsedPercentage: 75,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // null should be treated as 100% (no visual content warning)
      expect(queryByText(/visual content could be used/)).not.toBeInTheDocument()
      expect(getByText('75% of the page content was used')).toBeInTheDocument()
    })

    test('should handle very low visual content percentage', () => {
      const { getByText } = renderWithContext({
        visualContentUsedPercentage: 1,
        contentUsedPercentage: 100,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      expect(getByText('Only 1% of visual content could be used')).toBeInTheDocument()
    })

    test('should handle very high visual content percentage (just under 100)', () => {
      const { getByText } = renderWithContext({
        visualContentUsedPercentage: 99,
        contentUsedPercentage: 100,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      expect(getByText('Only 99% of visual content could be used')).toBeInTheDocument()
    })
  })

  describe('component structure', () => {
    test('should render with info icon', () => {
      const { container } = renderWithContext({
        visualContentUsedPercentage: 80,
        contentUsedPercentage: 100,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // Check for icon (leo-icon with name 'info-outline')
      const icon = container.querySelector('leo-icon[name="info-outline"]')
      expect(icon).toBeInTheDocument()
    })

    test('should have correct CSS class structure', () => {
      const { container } = renderWithContext({
        visualContentUsedPercentage: 80,
        contentUsedPercentage: 100,
        trimmedTokens: BigInt(0),
        totalTokens: BigInt(0),
      })

      // Check for info container with CSS class
      const infoDiv = container.querySelector('div[class*="info"]')
      expect(infoDiv).toBeInTheDocument()
    })
  })
})

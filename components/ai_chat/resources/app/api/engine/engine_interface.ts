// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../common/mojom'

// Page content with metadata, used for associated content in conversations
export interface PageContent {
  content: string
  isVideo: boolean
}

// Array of page content references
export type PageContents = PageContent[]

// Map from conversation entry UUID to its associated page contents
export type PageContentsMap = Map<string, PageContents>

// Conversation history type
export type ConversationHistory = Mojom.ConversationTurn[]

// Result data from generation operations
export interface GenerationResultData {
  event: Mojom.ConversationEntryEvent | null
  modelKey?: string
  isNearVerified?: boolean
}

// Generation result - either success with data or error
export type GenerationResult =
  | { ok: true; value: GenerationResultData }
  | { ok: false; error: Mojom.APIError }

// Suggested questions result
export type SuggestedQuestionResult =
  | { ok: true; value: string[] }
  | { ok: false; error: Mojom.APIError }

// Callback types
export type GenerationDataCallback = (data: GenerationResultData) => void
export type GenerationCompletedCallback = (result: GenerationResult) => void
export type SuggestedQuestionsCallback = (
  result: SuggestedQuestionResult,
) => void

// Tool interface (simplified for TypeScript)
export interface Tool {
  name: string
  definition: unknown // OAI tool definition
}

/**
 * Abstract interface for AI completion engines.
 * Engines could be local (invoked directly) or remote (invoked via network requests).
 */
export interface EngineConsumer {
  /**
   * Generate suggested questions based on page content.
   * @param pageContents The page contents to generate questions from
   * @param selectedLanguage The user's selected language
   * @param callback Called when questions are generated or an error occurs
   */
  generateQuestionSuggestions: (
    pageContents: PageContents,
    selectedLanguage: string,
    callback: SuggestedQuestionsCallback,
  ) => void

  /**
   * Generate an assistant response based on conversation history and context.
   * @param pageContents Map of page contents keyed by conversation entry UUID
   * @param conversationHistory The conversation history
   * @param selectedLanguage The user's selected language
   * @param isTemporaryChat Whether this is a temporary (non-persistent) chat
   * @param tools Available tools for the assistant
   * @param preferredToolName Optional preferred tool to use
   * @param conversationCapability The capability of the conversation
   * @param dataReceivedCallback Called when streaming data is received
   * @param completedCallback Called when generation is complete
   */
  generateAssistantResponse: (
    pageContents: PageContentsMap,
    conversationHistory: ConversationHistory,
    selectedLanguage: string,
    isTemporaryChat: boolean,
    tools: Tool[],
    preferredToolName: string | undefined,
    conversationCapability: Mojom.ConversationCapability,
    dataReceivedCallback: GenerationDataCallback,
    completedCallback: GenerationCompletedCallback,
  ) => void

  /**
   * Stop any in-progress operations.
   */
  clearAllQueries: () => void

  /**
   * For streaming responses, whether the engine provides delta text responses
   * (true) or full completion each time (false).
   */
  supportsDeltaTextResponses: () => boolean
}

/**
 * Helper to get the prompt text from a conversation entry.
 * Uses the most recent edit if available, otherwise falls back to prompt or text.
 */
export function getPromptForEntry(entry: Mojom.ConversationTurn): string {
  // Check for edits - use the most recent one
  if (entry.edits && entry.edits.length > 0) {
    const lastEdit = entry.edits[entry.edits.length - 1]
    return lastEdit.prompt ?? lastEdit.text
  }

  // Use prompt if available, otherwise text
  return entry.prompt ?? entry.text
}

/**
 * Check if we can perform a completion request based on the conversation history.
 */
export function canPerformCompletionRequest(
  conversationHistory: ConversationHistory,
): boolean {
  if (conversationHistory.length === 0) {
    return false
  }

  // The last entry should be a human message
  const lastEntry = conversationHistory[conversationHistory.length - 1]
  return lastEntry.characterType === Mojom.CharacterType.HUMAN
}

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../../common/mojom'
import {
  type EngineConsumer,
  type PageContent,
  type PageContents,
  type PageContentsMap,
  type ConversationHistory,
  type GenerationDataCallback,
  type GenerationCompletedCallback,
  type SuggestedQuestionsCallback,
  type Tool,
  getPromptForEntry,
  canPerformCompletionRequest,
} from '../engine_interface'
import {
  ConversationAPIClient,
  ConversationEventRole,
  ConversationEventType,
  type ConversationEvent,
  type ConversationAPIClientConfig,
} from './v1_api_client'
import type ModelStore from '../../stores/model_store'
import type MemoryStore from '../../stores/memory_store'

// Feature flags (can be configured via constructor)
const DEFAULT_CONTENT_SIZE_LARGE_TOOL_USE_EVENT = 10000
const DEFAULT_MAX_COUNT_LARGE_TOOL_USE_EVENTS = 3

// Configuration for the V1 engine
export interface V1EngineConfig extends ConversationAPIClientConfig {
  modelStore: ModelStore
  memoryStore?: MemoryStore
  contentSizeLargeToolUseEvent?: number
  maxCountLargeToolUseEvents?: number
}

/**
 * Convert image data to a data URL.
 */
function getImageDataURL(data: Uint8Array): string {
  const base64 = btoa(String.fromCharCode(...data))
  return `data:image/png;base64,${base64}`
}

/**
 * Convert PDF data to a data URL.
 */
function getPdfDataURL(data: Uint8Array): string {
  const base64 = btoa(String.fromCharCode(...data))
  return `data:application/pdf;base64,${base64}`
}

/**
 * Build a skill definition message from a SkillEntry.
 */
function buildSkillDefinitionMessage(skill: Mojom.SkillEntry): string {
  return `<user_defined_skill><shortcut>${skill.shortcut}</shortcut><prompt>${skill.prompt}</prompt></user_defined_skill>`
}

/**
 * Engine consumer implementation for the Brave Conversation API v1.
 */
export class V1Engine implements EngineConsumer {
  private api: ConversationAPIClient
  private config: V1EngineConfig

  constructor(config: V1EngineConfig) {
    this.config = config
    this.api = new ConversationAPIClient(config)
  }

  /**
   * Stop any in-progress operations.
   */
  clearAllQueries(): void {
    this.api.clearAllQueries()
  }

  /**
   * For streaming responses, returns true because this engine provides
   * delta text responses.
   */
  supportsDeltaTextResponses(): boolean {
    return true
  }

  /**
   * Create a conversation event for associated page content.
   */
  private getAssociatedContentConversationEvent(
    content: PageContent,
    remainingLength: number,
  ): ConversationEvent {
    const truncatedContent = content.content.substring(0, remainingLength)

    return {
      role: ConversationEventRole.User,
      type: content.isVideo
        ? ConversationEventType.VideoTranscript
        : ConversationEventType.PageText,
      content: [truncatedContent],
    }
  }

  /**
   * Get user memory event if available and not a temporary chat.
   */
  private async getUserMemoryEvent(
    isTemporaryChat: boolean,
  ): Promise<ConversationEvent | null> {
    if (isTemporaryChat || !this.config.memoryStore) {
      return null
    }

    const userMemory = await this.config.memoryStore.getMemoryForEngine()
    if (!userMemory) {
      return null
    }

    return {
      role: ConversationEventRole.User,
      type: ConversationEventType.UserMemory,
      content: [],
      userMemory,
    }
  }

  /**
   * Generate suggested questions based on page content.
   */
  generateQuestionSuggestions(
    pageContents: PageContents,
    selectedLanguage: string,
    callback: SuggestedQuestionsCallback,
  ): void {
    const conversation: ConversationEvent[] = []
    let remainingLength = this.config.modelStore.getMaxAssociatedContentLength()

    // Iterate in reverse so we prefer more recent page content
    for (let i = pageContents.length - 1; i >= 0 && remainingLength > 0; i--) {
      const content = pageContents[i]
      conversation.push(
        this.getAssociatedContentConversationEvent(content, remainingLength),
      )
      if (content.content.length > remainingLength) {
        break
      }
      remainingLength -= content.content.length
    }

    // Add request for suggested actions
    conversation.push({
      role: ConversationEventRole.User,
      type: ConversationEventType.RequestSuggestedActions,
      content: [''],
    })

    // Perform request without streaming
    this.api.performRequest(
      conversation,
      selectedLanguage,
      undefined, // no tools
      undefined, // no preferred tool
      Mojom.ConversationCapability.CHAT,
      null, // no streaming callback
      (result) => {
        if (!result.ok) {
          callback({ ok: false, error: result.error })
          return
        }

        // Parse the completion as pipe-separated questions
        if (
          !result.value.event
          || !result.value.event.completionEvent
          || !result.value.event.completionEvent.completion
        ) {
          callback({ ok: false, error: Mojom.APIError.InternalError })
          return
        }

        const completion = result.value.event.completionEvent.completion
        const questions = completion
          .split('|')
          .map((q) => q.trim())
          .filter((q) => q.length > 0)

        callback({ ok: true, value: questions })
      },
      undefined, // no model override
      this.config.modelStore.getModelKeyByName,
    )
  }

  /**
   * Generate an assistant response based on conversation history and context.
   */
  async generateAssistantResponse(
    pageContents: PageContentsMap,
    conversationHistory: ConversationHistory,
    selectedLanguage: string,
    isTemporaryChat: boolean,
    tools: Tool[],
    preferredToolName: string | undefined,
    conversationCapability: Mojom.ConversationCapability,
    dataReceivedCallback: GenerationDataCallback,
    completedCallback: GenerationCompletedCallback,
  ): Promise<void> {
    if (!canPerformCompletionRequest(conversationHistory)) {
      completedCallback({ ok: false, error: Mojom.APIError.None })
      return
    }

    const conversation: ConversationEvent[] = []

    // Add user memory event if available
    const userMemoryEvent = await this.getUserMemoryEvent(isTemporaryChat)
    if (userMemoryEvent) {
      conversation.push(userMemoryEvent)
    }

    let remainingLength = this.config.modelStore.getMaxAssociatedContentLength()

    // Build page content events for each conversation entry
    // We iterate in reverse to prefer most recent content
    const pageContentsMessages = new Map<string, ConversationEvent[]>()

    // Track large tool results to remove oldest ones
    const largeToolResultRemoveSet = new Set<string>() // key: `${messageIndex}-${eventIndex}`
    let largeToolCount = 0

    const contentSizeLargeToolUseEvent =
      this.config.contentSizeLargeToolUseEvent
      ?? DEFAULT_CONTENT_SIZE_LARGE_TOOL_USE_EVENT
    const maxCountLargeToolUseEvents =
      this.config.maxCountLargeToolUseEvents
      ?? DEFAULT_MAX_COUNT_LARGE_TOOL_USE_EVENTS

    // First pass: identify which content to keep and which large tool results to remove
    for (
      let messageIndex = conversationHistory.length - 1;
      messageIndex >= 0;
      messageIndex--
    ) {
      const message = conversationHistory[messageIndex]
      if (!message.uuid) continue

      // Process page contents for this turn
      const turnPageContents = pageContents.get(message.uuid)
      if (turnPageContents && remainingLength > 0) {
        const events: ConversationEvent[] = []
        for (
          let i = turnPageContents.length - 1;
          i >= 0 && remainingLength > 0;
          i--
        ) {
          const content = turnPageContents[i]
          events.push(
            this.getAssociatedContentConversationEvent(
              content,
              remainingLength,
            ),
          )
          if (content.content.length > remainingLength) {
            remainingLength = 0
          } else {
            remainingLength -= content.content.length
          }
        }
        pageContentsMessages.set(message.uuid, events)
      }

      // Track large tool results
      if (
        message.characterType === Mojom.CharacterType.ASSISTANT
        && message.events
        && message.events.length > 0
      ) {
        for (
          let eventIndex = message.events.length - 1;
          eventIndex >= 0;
          eventIndex--
        ) {
          const messageEvent = message.events[eventIndex]
          if (!messageEvent.toolUseEvent) continue

          const toolEvent = messageEvent.toolUseEvent
          if (!toolEvent.output || toolEvent.output.length === 0) continue

          // Check if this tool result is large
          let isLarge = false
          let contentSize = 0

          for (const contentBlock of toolEvent.output) {
            if (contentBlock.imageContentBlock) {
              isLarge = true
              break
            } else if (contentBlock.textContentBlock) {
              contentSize += contentBlock.textContentBlock.text.length
              if (contentSize >= contentSizeLargeToolUseEvent) {
                isLarge = true
                break
              }
            }
          }

          if (isLarge) {
            largeToolCount++
            if (largeToolCount > maxCountLargeToolUseEvents) {
              largeToolResultRemoveSet.add(`${messageIndex}-${eventIndex}`)
            }
          }
        }
      }
    }

    // Second pass: build conversation in chronological order
    for (
      let messageIndex = 0;
      messageIndex < conversationHistory.length;
      messageIndex++
    ) {
      const message = conversationHistory[messageIndex]
      if (!message.uuid) continue

      // Add page content events for this turn
      const pageContentEvents = pageContentsMessages.get(message.uuid)
      if (pageContentEvents) {
        conversation.push(...pageContentEvents)
      }

      // Handle uploaded files
      if (message.uploadedFiles && message.uploadedFiles.length > 0) {
        const uploadedImages: string[] = []
        const screenshotImages: string[] = []
        const uploadedPdfs: string[] = []

        for (const file of message.uploadedFiles) {
          if (file.type === Mojom.UploadedFileType.kScreenshot) {
            screenshotImages.push(getImageDataURL(new Uint8Array(file.data)))
          } else if (file.type === Mojom.UploadedFileType.kImage) {
            uploadedImages.push(getImageDataURL(new Uint8Array(file.data)))
          } else if (file.type === Mojom.UploadedFileType.kPdf) {
            uploadedPdfs.push(getPdfDataURL(new Uint8Array(file.data)))
          }
        }

        if (uploadedImages.length > 0) {
          conversation.push({
            role: ConversationEventRole.User,
            type: ConversationEventType.UploadImage,
            content: uploadedImages,
          })
        }

        if (screenshotImages.length > 0) {
          conversation.push({
            role: ConversationEventRole.User,
            type: ConversationEventType.PageScreenshot,
            content: screenshotImages,
          })
        }

        if (uploadedPdfs.length > 0) {
          conversation.push({
            role: ConversationEventRole.User,
            type: ConversationEventType.UploadPdf,
            content: uploadedPdfs,
          })
        }
      }

      // Handle selected text (page excerpt)
      if (message.selectedText && message.selectedText.length > 0) {
        conversation.push({
          role: ConversationEventRole.User,
          type: ConversationEventType.PageExcerpt,
          content: [message.selectedText],
        })
      }

      // Add skill definition message for human turns
      if (
        message.characterType === Mojom.CharacterType.HUMAN
        && message.skill
      ) {
        conversation.push({
          role: ConversationEventRole.User,
          type: ConversationEventType.ChatMessage,
          content: [buildSkillDefinitionMessage(message.skill)],
        })
      }

      // Build the main conversation event
      const event: ConversationEvent = {
        role:
          message.characterType === Mojom.CharacterType.HUMAN
            ? ConversationEventRole.User
            : ConversationEventRole.Assistant,
        type:
          message.actionType === Mojom.ActionType.SUMMARIZE_PAGE
            ? ConversationEventType.RequestSummary
            : ConversationEventType.ChatMessage,
        content:
          message.actionType === Mojom.ActionType.SUMMARIZE_PAGE
            ? ['']
            : [getPromptForEntry(message)],
        toolCalls: [],
      }

      // Add tool calls from assistant messages
      if (
        message.characterType === Mojom.CharacterType.ASSISTANT
        && message.events
        && message.events.length > 0
      ) {
        for (const messageEvent of message.events) {
          if (!messageEvent.toolUseEvent) continue
          const toolEvent = messageEvent.toolUseEvent
          if (toolEvent.output) {
            event.toolCalls!.push(toolEvent)
          }
        }
      }

      // Only include tool_calls if there are any
      if (event.toolCalls!.length === 0) {
        delete event.toolCalls
      }

      conversation.push(event)

      // Add tool results after the main message
      if (
        message.characterType === Mojom.CharacterType.ASSISTANT
        && message.events
        && message.events.length > 0
      ) {
        for (
          let eventIndex = 0;
          eventIndex < message.events.length;
          eventIndex++
        ) {
          const messageEvent = message.events[eventIndex]
          if (!messageEvent.toolUseEvent) continue

          const toolEvent = messageEvent.toolUseEvent
          if (!toolEvent.output) continue

          const shouldKeepFullContent = !largeToolResultRemoveSet.has(
            `${messageIndex}-${eventIndex}`,
          )

          const toolResult: ConversationEvent = {
            role: ConversationEventRole.Tool,
            type: ConversationEventType.ToolUse,
            content: shouldKeepFullContent
              ? toolEvent.output
              : ['[Large result removed to save space for subsequent results]'],
            toolCallId: toolEvent.id,
          }

          conversation.push(toolResult)
        }
      }
    }

    // Get model name override if the last message has a model key
    let modelName: string | undefined
    const lastMessage = conversationHistory[conversationHistory.length - 1]
    if (lastMessage.modelKey) {
      modelName = this.config.modelStore.getModelNameByKey(lastMessage.modelKey)
    }

    // Convert tools to OAI format
    const oaiToolDefinitions =
      tools.length > 0 ? tools.map((t) => t.definition) : undefined

    // Perform the request
    await this.api.performRequest(
      conversation,
      selectedLanguage,
      oaiToolDefinitions,
      preferredToolName,
      conversationCapability,
      dataReceivedCallback,
      completedCallback,
      modelName,
      this.config.modelStore.getModelKeyByName,
    )
  }
}

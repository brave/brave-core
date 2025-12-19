// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../../../common/mojom'
import type {
  GenerationResultData,
  GenerationDataCallback,
  GenerationCompletedCallback,
} from '../engine_interface'

// SSE event structure
interface SSEEvent {
  event: string
  data: string
}

// Role of the conversation event
export enum ConversationEventRole {
  User = 'user',
  Assistant = 'assistant',
  Tool = 'tool',
}

// Type of conversation event - maps to server API types
export enum ConversationEventType {
  System = 'system',
  ContextURL = 'contextURL',
  UserText = 'userText',
  PageText = 'pageText',
  PageExcerpt = 'pageExcerpt',
  VideoTranscript = 'videoTranscript',
  VideoTranscriptXML = 'videoTranscriptXML',
  VideoTranscriptVTT = 'videoTranscriptVTT',
  BraveSearchResults = 'braveSearchResults',
  ChatMessage = 'chatMessage',
  RequestSuggestedActions = 'requestSuggestedActions',
  RequestSummary = 'requestSummary',
  RequestRewrite = 'requestRewrite',
  SuggestedActions = 'suggestedActions',
  UploadImage = 'uploadImage',
  UploadPdf = 'uploadPdf',
  GetSuggestedTopicsForFocusTabs = 'suggestFocusTopics',
  DedupeTopics = 'dedupeFocusTopics',
  GetSuggestedAndDedupeTopicsForFocusTabs = 'suggestAndDedupeFocusTopics',
  GetFocusTabsForTopic = 'classifyTabs',
  PageScreenshot = 'pageScreenshot',
  ToolUse = 'toolUse',
  UserMemory = 'userMemory',
  ChangeTone = 'requestChangeTone',
  Paraphrase = 'requestParaphrase',
  Improve = 'requestImprove',
  Shorten = 'requestShorten',
  Expand = 'requestExpand',
}

// Content can be strings or content blocks
export type Content = string[] | Mojom.ContentBlock[]

// Conversation event that will be sent to the API
export interface ConversationEvent {
  role: ConversationEventRole
  type: ConversationEventType
  content: Content

  // Optional structured properties
  topic?: string // For GetFocusTabsForTopic events
  userMemory?: Record<string, unknown> // For UserMemory events
  toolCalls?: Mojom.ToolUseEvent[] // For Assistant ChatMessage events with tool calls
  toolCallId?: string // For Tool ToolUse events (result of tool execution)
  tone?: string // For ChangeTone events
}

// Configuration for the API client
export interface ConversationAPIClientConfig {
  modelName: string
  apiEndpoint: string
  getCredential?: () => Promise<string | null>
  systemLanguage?: string
}

// Allowed web source favicon host
const ALLOWED_WEB_SOURCE_FAVICON_HOST = 'imgs.search.brave.com'

/**
 * Parse SSE stream from a Response object.
 */
async function readSSE(
  response: Response,
  onEvent: (event: SSEEvent) => void,
): Promise<void> {
  const reader = response.body!.getReader()
  const decoder = new TextDecoder()

  let buffer = ''
  let currentEvent = 'message'
  let currentData = ''

  while (true) {
    const { value, done } = await reader.read()
    if (done) break

    buffer += decoder.decode(value, { stream: true })

    // Split on newlines (handle both \r\n and \n)
    const lines = buffer.split(/\r?\n/)

    // Keep the last part as it may be incomplete
    buffer = lines.pop() ?? ''

    for (const line of lines) {
      if (line === '') {
        // Empty line means end of event
        if (currentData) {
          onEvent({ event: currentEvent, data: currentData.trimEnd() })
          currentData = ''
          currentEvent = 'message'
        }
      } else if (line.startsWith('event:')) {
        currentEvent = line.slice(6).trim()
      } else if (line.startsWith('data:')) {
        currentData += line.slice(5) + '\n'
      }
      // Ignore other lines (comments starting with :, etc.)
    }
  }

  // Process any remaining data
  if (currentData) {
    onEvent({ event: currentEvent, data: currentData.trimEnd() })
  }
}

/**
 * Convert Content to JSON representation for the API.
 */
function contentToJson(content: Content): unknown {
  // Handle string array content
  if (content.length === 0) {
    return ''
  }

  if (typeof content[0] === 'string') {
    const strings = content as string[]
    if (strings.length === 1) {
      return strings[0]
    }
    return strings
  }

  // Handle ContentBlock array
  const blocks = content as Mojom.ContentBlock[]
  return blocks
    .map((block) => {
      if (block.imageContentBlock) {
        return {
          type: 'image_url',
          image_url: { url: block.imageContentBlock.imageUrl.url },
        }
      } else if (block.textContentBlock) {
        return {
          type: 'text',
          text: block.textContentBlock.text,
        }
      }
      // Skip unknown content block types
      return null
    })
    .filter(Boolean)
}

/**
 * Convert conversation events to the API request format.
 */
function conversationEventsToList(
  conversation: ConversationEvent[],
): unknown[] {
  return conversation.map((event) => {
    const eventDict: Record<string, unknown> = {
      role: event.role,
      type: event.type,
      content: contentToJson(event.content),
    }

    // Handle tool calls - server expects type='toolCalls' when tool_calls present
    if (event.toolCalls && event.toolCalls.length > 0) {
      eventDict.type = 'toolCalls'
      eventDict.tool_calls = event.toolCalls.map((toolEvent) => ({
        id: toolEvent.id,
        type: 'function',
        function: {
          name: toolEvent.toolName,
          arguments: toolEvent.argumentsJson,
        },
      }))
    }

    if (event.toolCallId) {
      eventDict.tool_call_id = event.toolCallId
    }

    if (
      event.type === ConversationEventType.GetFocusTabsForTopic
      && event.topic
    ) {
      eventDict.topic = event.topic
    }

    if (event.type === ConversationEventType.UserMemory && event.userMemory) {
      eventDict.memory = event.userMemory
    }

    if (event.type === ConversationEventType.ChangeTone && event.tone) {
      eventDict.tone = event.tone
    }

    return eventDict
  })
}

/**
 * Parse tool calls from API response into ToolUseEvent objects.
 */
function parseToolCalls(toolCalls: unknown[]): Mojom.ToolUseEvent[] {
  const events: Mojom.ToolUseEvent[] = []

  for (const call of toolCalls) {
    if (typeof call !== 'object' || call === null) continue

    const toolCall = call as Record<string, unknown>
    const id = toolCall.id as string | undefined
    const func = toolCall.function as Record<string, unknown> | undefined

    if (!id || !func) continue

    const name = func.name as string | undefined
    const args = func.arguments as string | undefined

    if (!name) continue

    events.push({
      id,
      toolName: name,
      argumentsJson: args ?? '',
      output: undefined,
      permissionChallenge: undefined,
    })
  }

  return events
}

/**
 * Parse a response event from the API into GenerationResultData.
 */
function parseResponseEvent(
  responseEvent: Record<string, unknown>,
  modelKeyLookup?: (modelName: string) => string | undefined,
): GenerationResultData | null {
  const model = responseEvent.model as string | undefined
  const type = responseEvent.type as string | undefined

  if (!model || !type) {
    return null
  }

  let event: Mojom.ConversationEntryEvent | null = null

  switch (type) {
    case 'completion': {
      const completion = responseEvent.completion as string | undefined
      if (!completion) return null
      event = {
        completionEvent: { completion },
        searchQueriesEvent: undefined,
        searchStatusEvent: undefined,
        selectedLanguageEvent: undefined,
        conversationTitleEvent: undefined,
        sourcesEvent: undefined,
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'isSearching': {
      event = {
        completionEvent: undefined,
        searchQueriesEvent: undefined,
        searchStatusEvent: { isSearching: true },
        selectedLanguageEvent: undefined,
        conversationTitleEvent: undefined,
        sourcesEvent: undefined,
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'searchQueries': {
      const queries = responseEvent.queries as unknown[] | undefined
      if (!queries) return null
      event = {
        completionEvent: undefined,
        searchQueriesEvent: {
          searchQueries: queries.filter((q) => typeof q === 'string'),
        },
        searchStatusEvent: undefined,
        selectedLanguageEvent: undefined,
        conversationTitleEvent: undefined,
        sourcesEvent: undefined,
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'webSources': {
      const sources = responseEvent.sources as unknown[] | undefined
      if (!sources) return null

      const webSources: Mojom.WebSource[] = []
      for (const item of sources) {
        if (typeof item !== 'object' || item === null) continue
        const source = item as Record<string, unknown>
        const title = source.title as string | undefined
        const url = source.url as string | undefined
        const faviconUrl = source.favicon as string | undefined

        if (!title || !url) continue

        // Validate URLs
        try {
          const itemUrl = new URL(url)
          let itemFaviconUrl: URL

          if (faviconUrl) {
            itemFaviconUrl = new URL(faviconUrl)
            // Validate favicon is from allowed source
            if (
              itemFaviconUrl.protocol !== 'https:'
              || itemFaviconUrl.hostname.toLowerCase()
                !== ALLOWED_WEB_SOURCE_FAVICON_HOST
            ) {
              continue
            }
          } else {
            itemFaviconUrl = new URL('/nala-icons/globe.svg')
          }

          webSources.push({
            title,
            url: { url: itemUrl.href },
            faviconUrl: { url: itemFaviconUrl.href },
          })
        } catch {
          continue
        }
      }

      if (webSources.length === 0) return null

      // Parse rich results
      const richResults: string[] = []
      const richResultsRaw = responseEvent.rich_results as unknown[] | undefined
      if (richResultsRaw) {
        for (const item of richResultsRaw) {
          if (typeof item !== 'object' || item === null) continue
          const results = (item as Record<string, unknown>).results as
            | unknown[]
            | undefined
          if (!results) continue
          for (const result of results) {
            if (typeof result === 'object' && result !== null) {
              richResults.push(JSON.stringify(result))
            }
          }
        }
      }

      event = {
        completionEvent: undefined,
        searchQueriesEvent: undefined,
        searchStatusEvent: undefined,
        selectedLanguageEvent: undefined,
        conversationTitleEvent: undefined,
        sourcesEvent: {
          sources: webSources,
          richResults,
        },
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'conversationTitle': {
      const title = responseEvent.title as string | undefined
      if (!title) return null
      event = {
        completionEvent: undefined,
        searchQueriesEvent: undefined,
        searchStatusEvent: undefined,
        selectedLanguageEvent: undefined,
        conversationTitleEvent: { title },
        sourcesEvent: undefined,
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'selectedLanguage': {
      const language = responseEvent.language as string | undefined
      if (!language) return null
      event = {
        completionEvent: undefined,
        searchQueriesEvent: undefined,
        searchStatusEvent: undefined,
        selectedLanguageEvent: { selectedLanguage: language },
        conversationTitleEvent: undefined,
        sourcesEvent: undefined,
        contentReceiptEvent: undefined,
        toolUseEvent: undefined,
      }
      break
    }

    case 'contentReceipt': {
      const totalTokens = responseEvent.total_tokens as number | undefined
      const trimmedTokens = responseEvent.trimmed_tokens as number | undefined
      event = {
        completionEvent: undefined,
        searchQueriesEvent: undefined,
        searchStatusEvent: undefined,
        selectedLanguageEvent: undefined,
        conversationTitleEvent: undefined,
        sourcesEvent: undefined,
        contentReceiptEvent: {
          totalTokens: BigInt(Math.max(0, totalTokens ?? 0)),
          trimmedTokens: BigInt(Math.max(0, trimmedTokens ?? 0)),
        },
        toolUseEvent: undefined,
      }
      break
    }

    default:
      // Unknown event type, ignore
      return null
  }

  return {
    event,
    modelKey: modelKeyLookup?.(model),
  }
}

/**
 * Client for the Brave Conversation API.
 */
export class ConversationAPIClient {
  private config: ConversationAPIClientConfig
  private abortController: AbortController | null = null

  constructor(config: ConversationAPIClientConfig) {
    this.config = config
  }

  /**
   * Cancel all in-progress requests.
   */
  clearAllQueries(): void {
    if (this.abortController) {
      this.abortController.abort()
      this.abortController = null
    }
  }

  /**
   * Create the JSON request body for the API.
   */
  createJSONRequestBody(
    conversation: ConversationEvent[],
    selectedLanguage: string,
    oaiToolDefinitions: unknown[] | undefined,
    preferredToolName: string | undefined,
    conversationCapability: Mojom.ConversationCapability,
    modelName: string | undefined,
    isSSEEnabled: boolean,
  ): string {
    const capabilityMap: Record<Mojom.ConversationCapability, string> = {
      [Mojom.ConversationCapability.CHAT]: 'chat',
      [Mojom.ConversationCapability.CONTENT_AGENT]: 'content_agent',
    }

    const body: Record<string, unknown> = {
      events: conversationEventsToList(conversation),
      capability: capabilityMap[conversationCapability],
      model: modelName ?? this.config.modelName,
      selected_language: selectedLanguage,
      system_language: this.config.systemLanguage ?? 'en_US',
      stream: isSSEEnabled,
      use_citations: true,
    }

    if (oaiToolDefinitions && oaiToolDefinitions.length > 0) {
      body.tools = oaiToolDefinitions
    }

    return JSON.stringify(body)
  }

  /**
   * Perform a request to the Conversation API.
   */
  async performRequest(
    conversation: ConversationEvent[],
    selectedLanguage: string,
    oaiToolDefinitions: unknown[] | undefined,
    preferredToolName: string | undefined,
    conversationCapability: Mojom.ConversationCapability,
    dataReceivedCallback: GenerationDataCallback | null,
    completedCallback: GenerationCompletedCallback,
    modelName?: string,
    modelKeyLookup?: (modelName: string) => string | undefined,
  ): Promise<void> {
    if (conversation.length === 0) {
      completedCallback({ ok: false, error: Mojom.APIError.None })
      return
    }

    // Cancel any existing request
    this.clearAllQueries()
    this.abortController = new AbortController()

    const isSSEEnabled = dataReceivedCallback !== null
    const requestBody = this.createJSONRequestBody(
      conversation,
      selectedLanguage,
      oaiToolDefinitions,
      preferredToolName,
      conversationCapability,
      modelName,
      isSSEEnabled,
    )

    const headers: Record<string, string> = {
      Accept: 'text/event-stream',
      'Content-Type': 'application/json',
    }

    // Get credential if available
    if (this.config.getCredential) {
      const credential = await this.config.getCredential()
      if (credential) {
        headers.Cookie = `__Secure-sku#brave-leo-premium=${credential}`
      }
    }

    console.log(
      'ConversationAPIClient: Sending request to',
      this.config.apiEndpoint,
      requestBody,
    )

    try {
      const response = await fetch(this.config.apiEndpoint, {
        method: 'POST',
        headers,
        body: requestBody,
        signal: this.abortController.signal,
      })

      if (!response.ok) {
        // Handle error responses
        let error: Mojom.APIError

        switch (response.status) {
          case 429:
            error = Mojom.APIError.RateLimitReached
            break
          case 413:
            error = Mojom.APIError.ContextLimitReached
            break
          default:
            error = Mojom.APIError.ConnectionIssue
        }

        completedCallback({ ok: false, error })
        return
      }

      // Handle near-verified header
      let isNearVerified: boolean | undefined
      const nearVerifiedHeader = response.headers.get('brave-near-verified')
      if (nearVerifiedHeader !== null) {
        isNearVerified = nearVerifiedHeader === 'true'
      }

      if (isSSEEnabled && response.body) {
        // Streaming response
        await readSSE(response, ({ event, data }) => {
          if (data.startsWith('[DONE]')) return

          try {
            const json = JSON.parse(data) as Record<string, unknown>

            // Parse main response event
            const resultData = parseResponseEvent(json, modelKeyLookup)
            if (resultData) {
              resultData.isNearVerified = isNearVerified
              dataReceivedCallback(resultData)
            }

            // Parse tool calls (may happen with or without a response event)
            const toolCalls = json.tool_calls as unknown[] | undefined
            if (toolCalls) {
              // Check for alignment_check that applies to tool calls
              let permissionChallenge: Mojom.PermissionChallenge | undefined
              const alignmentDict = json.alignment_check as
                | Record<string, unknown>
                | undefined
              if (alignmentDict && alignmentDict.allowed === false) {
                const assessment = alignmentDict.reasoning as string | undefined
                permissionChallenge = {
                  assessment: assessment ?? undefined,
                  plan: undefined,
                }
              }

              const toolUseEvents = parseToolCalls(toolCalls)
              for (let i = 0; i < toolUseEvents.length; i++) {
                const toolEvent = toolUseEvents[i]
                // Apply permission challenge to first tool call
                if (i === 0 && permissionChallenge) {
                  toolEvent.permissionChallenge = permissionChallenge
                }

                dataReceivedCallback({
                  event: {
                    completionEvent: undefined,
                    searchQueriesEvent: undefined,
                    searchStatusEvent: undefined,
                    selectedLanguageEvent: undefined,
                    conversationTitleEvent: undefined,
                    sourcesEvent: undefined,
                    contentReceiptEvent: undefined,
                    toolUseEvent: toolEvent,
                  },
                  modelKey: undefined,
                  isNearVerified,
                })
              }
            }
          } catch {
            // Ignore JSON parse errors in streaming data
          }
        })

        // Streaming completed successfully
        completedCallback({
          ok: true,
          value: {
            event: null,
            modelKey: undefined,
            isNearVerified,
          },
        })
      } else {
        // Non-streaming response
        const json = (await response.json()) as Record<string, unknown>
        const completion = json.completion as string | undefined
        const model = json.model as string | undefined

        const completionEvent: Mojom.ConversationEntryEvent | null = completion
          ? {
              completionEvent: { completion },
              searchQueriesEvent: undefined,
              searchStatusEvent: undefined,
              selectedLanguageEvent: undefined,
              conversationTitleEvent: undefined,
              sourcesEvent: undefined,
              contentReceiptEvent: undefined,
              toolUseEvent: undefined,
            }
          : null

        completedCallback({
          ok: true,
          value: {
            event: completionEvent,
            modelKey: model ? modelKeyLookup?.(model) : undefined,
            isNearVerified,
          },
        })
      }
    } catch (error) {
      // Check if this was an abort
      if (error instanceof Error && error.name === 'AbortError') {
        // Request was cancelled, don't call callback
        return
      }

      completedCallback({ ok: false, error: Mojom.APIError.ConnectionIssue })
    } finally {
      this.abortController = null
    }
  }
}

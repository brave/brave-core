// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// TypeScript ConversationHandler for the ai_chat_app build.
// Manages in-memory conversation history, calls the AI backend over HTTP/SSE,
// and broadcasts history updates to the untrusted conversation-entries frame
// via BroadcastChannel.

import * as Mojom from '../../../common/mojom'
import { makeCloseable } from '$web-common/api'
import createConversationApi from '../conversation_api'
import { AIChatAPI } from '../ai_chat_api'
import { APP_MODELS, DEFAULT_MODEL_KEY } from './app_models'
import * as IDB from './idb_store'
import * as Registry from './conversation_registry'

// Channel key shared with bind_app_untrusted_conversation.ts.
export const CONVERSATION_CHANNEL_PREFIX = 'ai-chat-conv-'

// Base URL for AI backend requests.
// apiBaseUrl is baked into strings.m.js at build time from the AI_BACKEND env
// var (default http://127.0.0.1:8000). The backend must set CORS headers.
export const config = {
  apiBaseUrl: (window as any).loadTimeData?.getValue('apiBaseUrl') as string ?? '',
  // Default model sent with every request. The UI's changeModel() overrides
  // this per-conversation.
  model: 'automatic',
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

function makeTurn(
  text: string,
  character: Mojom.CharacterType,
  uploadedFiles: Mojom.UploadedFile[] = [],
): Mojom.ConversationTurn {
  // internalValue is a Windows FILETIME (100-ns intervals since 1601-01-01).
  const winEpochOffsetMs = BigInt('11644473600000')
  return {
    uuid: crypto.randomUUID(),
    text,
    characterType: character,
    actionType: Mojom.ActionType.UNSPECIFIED,
    prompt: undefined,
    selectedText: undefined,
    edits: [],
    createdTime: { internalValue: (BigInt(Date.now()) + winEpochOffsetMs) * BigInt(10000) },
    events: [],
    uploadedFiles,
    fromBraveSearchSERP: false,
    skill: undefined,
    modelKey: '',
    nearVerificationStatus: undefined,
  }
}

function buildRequestBody(
  history: Mojom.ConversationTurn[],
  modelKey: string,
): object {
  const messages = history.map((turn) => {
    const role =
      turn.characterType === Mojom.CharacterType.HUMAN ? 'user' : 'assistant'
    // Use completionEvent text for assistant turns when available.
    let text = turn.text
    if (turn.characterType === Mojom.CharacterType.ASSISTANT) {
      const ce = turn.events?.find((e) => e.completionEvent)
      if (ce?.completionEvent?.completion) text = ce.completionEvent.completion
    }
    return { role, content: [{ type: 'text', text }] }
  })

  return {
    messages,
    brave_capability: ['chat'],
    model: modelKey || config.model,
    system_language: navigator.language.replace('-', '_'),
    stream: true,
  }
}

async function* parseSSE(
  response: Response,
): AsyncGenerator<Record<string, unknown>> {
  const reader = response.body!.getReader()
  const decoder = new TextDecoder()
  let buffer = ''
  try {
    while (true) {
      const { done, value } = await reader.read()
      if (done) break
      buffer += decoder.decode(value, { stream: true })
      let nl: number
      while ((nl = buffer.indexOf('\n')) !== -1) {
        const line = buffer.slice(0, nl).trim()
        buffer = buffer.slice(nl + 1)
        if (!line.startsWith('data: ')) continue
        const payload = line.slice(6).trim()
        if (payload === '[DONE]') return
        try {
          yield JSON.parse(payload) as Record<string, unknown>
        } catch {
          // ignore malformed frames
        }
      }
    }
  } finally {
    reader.releaseLock()
  }
}

// ---------------------------------------------------------------------------
// App conversation handler factory
// ---------------------------------------------------------------------------

const CONV_KEY_PREFIX = Registry.CONV_KEY_PREFIX

function createAppConversationHandler(uuid: string) {
  const history: Mojom.ConversationTurn[] = []
  let observer: Mojom.ConversationUIInterface | undefined
  let isRequestInProgress = false
  let abortController: AbortController | undefined
  let modelKey = DEFAULT_MODEL_KEY

  // Persist history to IndexedDB with a 1-second debounce.
  let saveTimer: ReturnType<typeof setTimeout> | undefined
  function scheduleSave() {
    clearTimeout(saveTimer)
    saveTimer = setTimeout(() => {
      Registry.notifyConversationUpdated(uuid, [...history])
      void IDB.set(CONV_KEY_PREFIX + uuid, history)
    }, 1000)
  }

  // Load any previously saved history and replay it to the observer.
  void IDB.get<Mojom.ConversationTurn[]>(CONV_KEY_PREFIX + uuid).then((saved) => {
    if (!saved?.length) return
    history.push(...saved)
    for (const turn of saved) observer?.onConversationHistoryUpdate(turn)
  })

  const channel = new BroadcastChannel(CONVERSATION_CHANNEL_PREFIX + uuid)

  // Respond to history-sync requests from the untrusted frame.
  channel.addEventListener('message', (event: MessageEvent) => {
    if (event.data?.type === 'requestHistory') {
      channel.postMessage({ type: 'historyResponse', history })
    }
  })

  async function generateResponse() {
    // Snapshot history for the request BEFORE the assistant turn is appended.
    const requestHistory = [...history]

    isRequestInProgress = true
    observer?.onAPIRequestInProgress(true)
    channel.postMessage({ type: 'stateChanged', isGenerating: true })

    const assistantTurn = makeTurn('', Mojom.CharacterType.ASSISTANT)
    history.push(assistantTurn)
    observer?.onConversationHistoryUpdate({ ...assistantTurn })
    channel.postMessage({ type: 'historyUpdate', entry: { ...assistantTurn } })

    try {
      abortController = new AbortController()
      const response = await fetch(`${config.apiBaseUrl}/v1/chat/completions`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          Accept: 'text/event-stream',
        },
        body: JSON.stringify(buildRequestBody(requestHistory, modelKey)),
        signal: abortController.signal,
      })

      if (!response.ok) throw new Error(`HTTP ${response.status}`)

      const SEARCH_TOOL_NAMES = new Set([
        'brave_web_search',
        'brave_news_search',
        'brave_faqs_search',
      ])

      let accumulated = ''
      const extraEvents: Mojom.ConversationEntryEvent[] = []

      function broadcastTurn() {
        const updated: Mojom.ConversationTurn = {
          ...assistantTurn,
          text: accumulated,
          events: [
            ...extraEvents,
            ...(accumulated
              ? [{ completionEvent: { completion: accumulated } } as Mojom.ConversationEntryEvent]
              : []),
          ],
        }
        history[history.length - 1] = updated
        Object.assign(assistantTurn, updated)
        observer?.onConversationHistoryUpdate({ ...assistantTurn })
        channel.postMessage({ type: 'historyUpdate', entry: { ...assistantTurn } })
        scheduleSave()
      }

      for await (const event of parseSSE(response)) {
        if (event.object === 'brave-chat.toolStart') {
          const toolName = event.tool_name as string | undefined
          if (toolName && SEARCH_TOOL_NAMES.has(toolName)) {
            extraEvents.push({
              searchStatusEvent: { isSearching: true },
            } as Mojom.ConversationEntryEvent)
            broadcastTurn()
          }
        } else if (event.object === 'brave-chat.inlineSearch') {
          const query = event.query as string | undefined
          if (query) {
            extraEvents.push({
              searchQueriesEvent: { searchQueries: [query] },
            } as Mojom.ConversationEntryEvent)
            broadcastTurn()
          }
        } else {
          // v2 API: OpenAI-compatible streaming chunks
          const delta = (event.choices as any)?.[0]?.delta?.content
          if (event.object === 'chat.completion.chunk' && typeof delta === 'string') {
            accumulated += delta
            broadcastTurn()
          }
        }
      }
    } catch (err: unknown) {
      const isAbort = err instanceof Error && err.name === 'AbortError'
      if (isAbort) {
        history.pop()
        observer?.onConversationHistoryUpdate(null)
        channel.postMessage({ type: 'historyInvalidate' })
      } else {
        observer?.onAPIResponseError(Mojom.APIError.ConnectionIssue)
      }
    } finally {
      isRequestInProgress = false
      abortController = undefined
      observer?.onAPIRequestInProgress(false)
      channel.postMessage({ type: 'stateChanged', isGenerating: false })
    }
  }

  function submitEntry(
    text: string,
    uploadedFiles: Mojom.UploadedFile[] = [],
  ) {
    if (isRequestInProgress) return
    const humanTurn = makeTurn(text, Mojom.CharacterType.HUMAN, uploadedFiles)
    history.push(humanTurn)
    Registry.notifyConversationUpdated(uuid, [...history])
    observer?.onConversationHistoryUpdate(humanTurn)
    channel.postMessage({ type: 'historyUpdate', entry: humanTurn })
    scheduleSave()
    void generateResponse()
  }

  const emptyTurn = makeTurn('', Mojom.CharacterType.HUMAN)

  return {
    setObserver(obs: Mojom.ConversationUIInterface) {
      observer = obs
    },
    closeChannel() {
      clearTimeout(saveTimer)
      channel.close()
    },

    // -- ConversationHandlerInterface --
    getState: async () => ({
      conversationState: {
        conversationUuid: uuid,
        isRequestInProgress,
        currentModelKey: modelKey,
        defaultModelKey: modelKey,
        allModels: APP_MODELS,
        associatedContent: [] as Mojom.AssociatedContent[],
        error: Mojom.APIError.None,
        temporary: false,
        toolUseTaskState: Mojom.TaskState.kNone,
      } as Mojom.ConversationState,
    }),
    getConversationHistory: async () => ({
      conversationHistory: [...history],
    }),
    getConversationUuid: async () => ({ conversationUuid: uuid }),
    getModels: async () => ({
      models: APP_MODELS,
      currentModelKey: modelKey,
    }),
    getIsRequestInProgress: async () => ({ isRequestInProgress }),
    getAssociatedContentInfo: async () => ({
      associatedContent: [] as Mojom.AssociatedContent[],
    }),
    getScreenshots: async () => ({ screenshots: [] as Mojom.UploadedFile[] }),
    clearErrorAndGetFailedMessage: async () => ({ turn: emptyTurn }),
    setTemporary: async () => ({}),
    stopGenerationAndMaybeGetHumanEntry: async () => {
      abortController?.abort()
      return { humanEntry: null }
    },
    rateMessage: async () => ({ ratingId: null }),
    sendFeedback: async () => ({ isSuccess: true }),
    changeModel: (key: string) => {
      modelKey = key
      observer?.onModelDataChanged(key, key, APP_MODELS)
    },
    submitHumanConversationEntry: (
      text: string,
      files: Mojom.UploadedFile[] | null,
    ) => {
      submitEntry(text, files ?? [])
    },
    submitHumanConversationEntryWithAction: (
      text: string,
      _actionType: Mojom.ActionType,
      _selectedText?: string,
    ) => {
      submitEntry(text)
    },
    submitHumanConversationEntryWithSkill: (
      text: string,
      _skillId: string,
    ) => {
      submitEntry(text)
    },
    pauseTask: () => {},
    resumeTask: () => {},
    stopTask: () => {},
  }
}

// ---------------------------------------------------------------------------
// Public binding functions — same shape as bind_conversation.ts exports
// ---------------------------------------------------------------------------

function createBindings(uuid: string) {
  const handler = createAppConversationHandler(uuid)
  const conversationAPI = createConversationApi(
    makeCloseable<Mojom.ConversationHandlerInterface>(handler),
  )
  handler.setObserver(conversationAPI.conversationUIObserver)
  conversationAPI.api.getState.update({ conversationUuid: uuid })

  return {
    conversationHandler: handler,
    close: () => {
      conversationAPI.close()
      handler.closeChannel()
    },
    api: conversationAPI.api,
  }
}

// Export with the same names as bind_conversation.ts so active_chat_context.tsx
// can import this module interchangeably via <if expr>.
export function newConversation(_aiChat: AIChatAPI['api']) {
  return createBindings(crypto.randomUUID())
}

export function bindConversation(
  _aiChat: AIChatAPI['api'],
  id: string | undefined,
) {
  return createBindings(id ?? crypto.randomUUID())
}

export type ConversationBindings = ReturnType<typeof newConversation>

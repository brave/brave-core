// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import {
  type EngineConsumer,
  type GenerationResultData,
  type GenerationResult,
  type PageContentsMap,
  type Tool,
  canPerformCompletionRequest,
} from './engine/engine_interface'
import { V1Engine, type V1EngineConfig } from './engine/v1/v1_engine'
import type ModelStore from './stores/model_store'

// Generates a UUID
function generateUUID(): string {
  return crypto.randomUUID()
}

// Suggestion with optional prompt
interface Suggestion {
  title: string
  prompt?: string
  actionType: Mojom.ActionType
}

interface FullConversationHandler
  extends Mojom.ConversationHandlerInterface,
    Mojom.UntrustedConversationHandlerInterface {}

// Configuration for creating a conversation handler
export interface ConversationHandlerConfig {
  engineConfig: V1EngineConfig
  modelStore: ModelStore
  getSkillById?: (id: string) => Mojom.Skill | undefined
}

export default class ConversationHandler implements FullConversationHandler {
  private observer: Mojom.ConversationUIInterface | null = null
  private entriesObserver: Mojom.UntrustedConversationUIInterface | null = null

  // Conversation metadata
  private metadata: Mojom.Conversation
  private config: ConversationHandlerConfig

  // Conversation state
  private chatHistory: Mojom.ConversationTurn[] = []
  private modelKey: string
  private selectedLanguage: string = ''
  private isRequestInProgress: boolean = false
  private currentError: Mojom.APIError = Mojom.APIError.None

  // Tool loop state
  private isToolUseInProgress: boolean = false
  private toolUseTaskState: Mojom.TaskState = Mojom.TaskState.kNone

  // Suggestions state
  private suggestions: Suggestion[] = []
  private suggestionGenerationStatus: Mojom.SuggestionGenerationStatus =
    Mojom.SuggestionGenerationStatus.None

  // Engine
  private engine: EngineConsumer

  // Implement Closable<>
  public $ = { close() {} }

  constructor(
    conversation: Mojom.Conversation,
    config: ConversationHandlerConfig,
  ) {
    this.metadata = conversation
    this.config = config
    this.modelKey =
      conversation.modelKey ?? config.modelStore.getDefaultModelKey()

    // Initialize engine
    this.initEngine()
  }

  get conversationUuid(): string {
    return this.metadata.uuid
  }

  get isTemporary(): boolean {
    return this.metadata.temporary
  }

  setObserver(observer: Mojom.ConversationUIInterface) {
    this.observer = observer
  }

  setEntriesObserver(observer: Mojom.UntrustedConversationUIInterface) {
    this.entriesObserver = observer
  }

  private initEngine() {
    if (this.engine) {
      this.engine.clearAllQueries()
    }
    const modelName =
      this.config.modelStore.getModelNameByKey(this.modelKey)
      ?? this.config.engineConfig.modelName
    this.engine = new V1Engine({ ...this.config.engineConfig, modelName })
  }

  // Get conversation entries state for the UI
  private getStateForConversationEntries(): Mojom.ConversationEntriesState {
    return {
      isGenerating: this.isRequestInProgress,
      isToolExecuting: this.isToolUseInProgress,
      toolUseTaskState: this.toolUseTaskState,
      isLeoModel: true, // Always true for V1 engine
      allModels: this.config.modelStore.getModels(),
      currentModelKey: this.modelKey,
      contentUsedPercentage: undefined, // Not tracking associated content
      visualContentUsedPercentage: undefined,
      canSubmitUserEntries: !this.isRequestInProgress,
      totalTokens: this.metadata.totalTokens,
      trimmedTokens: this.metadata.trimmedTokens,
      conversationCapability: Mojom.ConversationCapability.CHAT,
      isPremiumUser: false, // Can be updated later
    }
  }

  // Notify observers of state changes
  private notifyHistoryUpdate(entry: Mojom.ConversationTurn | null) {
    console.log('Notifying history update:', entry)
    this.observer?.onConversationHistoryUpdate(entry)
    this.entriesObserver?.onConversationHistoryUpdate(entry)
  }

  private notifyRequestInProgressChanged() {
    this.observer?.onAPIRequestInProgress(this.isRequestInProgress)
    this.entriesObserver?.onEntriesUIStateChanged?.(
      this.getStateForConversationEntries(),
    )
  }

  private notifyTaskStateChanged() {
    this.observer?.onTaskStateChanged(this.toolUseTaskState)
    this.entriesObserver?.onEntriesUIStateChanged?.(
      this.getStateForConversationEntries(),
    )
  }

  private notifyError(error: Mojom.APIError) {
    this.currentError = error
    this.observer?.onAPIResponseError(error)
  }

  private notifySuggestedQuestionsChanged() {
    this.observer?.onSuggestedQuestionsChanged(
      this.suggestions.map((s) => s.title),
      this.suggestionGenerationStatus,
    )
  }

  // Add entry to conversation history
  private addToConversationHistory(turn: Mojom.ConversationTurn) {
    // Generate UUID if not present
    if (!turn.uuid) {
      turn.uuid = generateUUID()
    }

    this.chatHistory.push(turn)
    this.notifyHistoryUpdate(turn)

    // Update metadata timestamp
    this.metadata.hasContent = true
  }

  // Create a human conversation turn
  private createHumanTurn(
    text: string,
    actionType: Mojom.ActionType = Mojom.ActionType.UNSPECIFIED,
    selectedText?: string,
    uploadedFiles?: Mojom.UploadedFile[],
    skill?: Mojom.SkillEntry,
  ): Mojom.ConversationTurn {
    return {
      uuid: generateUUID(),
      characterType: Mojom.CharacterType.HUMAN,
      actionType,
      text,
      prompt: undefined,
      selectedText: selectedText ?? undefined,
      events: undefined,
      createdTime: { internalValue: BigInt(Date.now() * 1000) },
      edits: undefined,
      uploadedFiles: uploadedFiles ?? undefined,
      skill: skill ?? undefined,
      fromBraveSearchSERP: false,
      modelKey: this.modelKey,
      nearVerificationStatus: undefined,
    }
  }

  // Create an assistant conversation turn
  private createAssistantTurn(): Mojom.ConversationTurn {
    return {
      uuid: generateUUID(),
      characterType: Mojom.CharacterType.ASSISTANT,
      actionType: Mojom.ActionType.RESPONSE,
      text: '',
      prompt: undefined,
      selectedText: undefined,
      events: [],
      createdTime: { internalValue: BigInt(Date.now() * 1000) },
      edits: undefined,
      uploadedFiles: undefined,
      skill: undefined,
      fromBraveSearchSERP: false,
      modelKey: this.modelKey,
      nearVerificationStatus: undefined,
    }
  }

  // Get the last assistant entry (for updating during streaming)
  private getLastAssistantEntry(): Mojom.ConversationTurn | undefined {
    for (let i = this.chatHistory.length - 1; i >= 0; i--) {
      if (this.chatHistory[i].characterType === Mojom.CharacterType.ASSISTANT) {
        return this.chatHistory[i]
      }
    }
    return undefined
  }

  // Update or create assistant entry from generation result
  private updateOrCreateLastAssistantEntry(result: GenerationResultData) {
    let assistantEntry = this.getLastAssistantEntry()
    const isNewEntry =
      !assistantEntry
      || this.chatHistory[this.chatHistory.length - 1].characterType
        !== Mojom.CharacterType.ASSISTANT

    if (isNewEntry) {
      assistantEntry = this.createAssistantTurn()
      this.chatHistory.push(assistantEntry)
    }

    assistantEntry = assistantEntry!

    if (!assistantEntry.events) {
      assistantEntry.events = []
    }

    const event = result.event
    if (!event) return

    // Handle different event types
    if (event.completionEvent) {
      const lastEvent = assistantEntry.events.at(-1)
      if (lastEvent?.completionEvent) {
        // Merge completion events
        if (this.engine.supportsDeltaTextResponses()) {
          event.completionEvent.completion =
            lastEvent.completionEvent.completion
            + event.completionEvent.completion
        }
        // Remove the last event because we'll replace in both delta and
        // non-delta cases
        assistantEntry.events.pop()
      }

      assistantEntry.text = event.completionEvent.completion

      // Add completion event to events array
      assistantEntry.events.push(event)
    } else if (event.toolUseEvent) {
      // Tool use event
      assistantEntry.events.push(event)
      // Start tool execution
      this.isToolUseInProgress = true
      this.toolUseTaskState = Mojom.TaskState.kRunning
      this.notifyTaskStateChanged()
    } else if (event.searchStatusEvent) {
      assistantEntry.events.push(event)
    } else if (event.searchQueriesEvent) {
      assistantEntry.events.push(event)
    } else if (event.sourcesEvent) {
      assistantEntry.events.push(event)
    } else if (event.conversationTitleEvent) {
      // Update conversation title
      this.metadata.title = event.conversationTitleEvent.title
    } else if (event.selectedLanguageEvent) {
      this.selectedLanguage = event.selectedLanguageEvent.selectedLanguage
    } else if (event.contentReceiptEvent) {
      this.metadata.totalTokens = event.contentReceiptEvent.totalTokens
      this.metadata.trimmedTokens = event.contentReceiptEvent.trimmedTokens
    }

    // Update near verification status
    if (result.isNearVerified !== undefined) {
      assistantEntry.nearVerificationStatus = {
        verified: result.isNearVerified,
      }
    }

    // Update model key if provided
    if (result.modelKey) {
      assistantEntry.modelKey = result.modelKey
    }

    this.notifyHistoryUpdate(assistantEntry)
  }

  // Perform assistant generation
  private async performAssistantGeneration() {
    if (!canPerformCompletionRequest(this.chatHistory)) {
      return
    }

    this.isRequestInProgress = true
    this.currentError = Mojom.APIError.None
    this.notifyRequestInProgressChanged()

    // No associated content for now (empty map)
    const pageContents: PageContentsMap = new Map()

    // No tools for now (can be added later)
    const tools: Tool[] = []

    await this.engine.generateAssistantResponse(
      pageContents,
      this.chatHistory,
      this.selectedLanguage,
      this.metadata.temporary,
      tools,
      undefined, // preferredToolName
      Mojom.ConversationCapability.CHAT,
      (data) => this.onEngineCompletionDataReceived(data),
      (result) => this.onEngineCompletionComplete(result),
    )
  }

  private onEngineCompletionDataReceived(result: GenerationResultData) {
    this.updateOrCreateLastAssistantEntry(result)
  }

  private onEngineCompletionComplete(result: GenerationResult) {
    if (!result.ok) {
      this.notifyError(result.error)
      this.completeGeneration(false)
      return
    }

    // Check if there are pending tool use requests
    if (this.hasPendingToolUseRequests()) {
      // Wait for tool responses
      return
    }

    this.completeGeneration(true)
  }

  private completeGeneration(success: boolean) {
    this.isRequestInProgress = false
    this.isToolUseInProgress = false
    this.toolUseTaskState = Mojom.TaskState.kNone
    this.notifyRequestInProgressChanged()
    this.notifyTaskStateChanged()

    if (success) {
      // Maybe seed suggestions for next turn
      this.maybeSeedOrClearSuggestions()
    }
  }

  // Check if there are tool use requests waiting for responses
  private hasPendingToolUseRequests(): boolean {
    const lastEntry = this.getLastAssistantEntry()
    if (!lastEntry?.events) return false

    for (const event of lastEntry.events) {
      if (event.toolUseEvent && !event.toolUseEvent.output) {
        return true
      }
    }
    return false
  }

  // Get tool use event by ID
  private getToolUseEventForLastResponse(
    toolId: string,
  ): Mojom.ToolUseEvent | undefined {
    const lastEntry = this.getLastAssistantEntry()
    if (!lastEntry?.events) return undefined

    for (const event of lastEntry.events) {
      if (event.toolUseEvent && event.toolUseEvent.id === toolId) {
        return event.toolUseEvent
      }
    }
    return undefined
  }

  // Maybe seed suggestions based on conversation state
  private maybeSeedOrClearSuggestions() {
    // Clear suggestions after first assistant response
    if (
      this.suggestionGenerationStatus
      === Mojom.SuggestionGenerationStatus.HasGenerated
    ) {
      this.suggestions = []
      this.suggestionGenerationStatus = Mojom.SuggestionGenerationStatus.None
      this.notifySuggestedQuestionsChanged()
    }
  }

  // Mojom.ConversationHandlerInterface
  getState(): Promise<{ conversationState: Mojom.ConversationState }> {
    return Promise.resolve({
      conversationState: {
        conversationUuid: this.metadata.uuid,
        isRequestInProgress: this.isRequestInProgress,
        allModels: this.config.modelStore.getModels(),
        currentModelKey: this.modelKey,
        defaultModelKey: this.config.modelStore.getDefaultModelKey(),
        suggestedQuestions: this.suggestions.map((s) => s.title),
        suggestionStatus: this.suggestionGenerationStatus,
        associatedContent: [], // No associated content
        error: this.currentError,
        temporary: this.metadata.temporary,
        toolUseTaskState: this.toolUseTaskState,
      },
    })
  }

  getConversationUuid(): Promise<{ conversationUuid: string }> {
    return Promise.resolve({ conversationUuid: this.metadata.uuid })
  }

  getModels(): Promise<{
    models: Array<Mojom.Model>
    currentModelKey: string
  }> {
    return Promise.resolve({
      models: this.config.modelStore.getModels(),
      currentModelKey: this.modelKey,
    })
  }

  changeModel(modelKey: string): void {
    const model = this.config.modelStore.getModelByKey(modelKey)
    if (!model) return

    this.modelKey = modelKey
    this.metadata.modelKey = modelKey

    this.initEngine()

    this.observer?.onModelDataChanged(
      modelKey,
      this.config.modelStore.getDefaultModelKey(),
      this.config.modelStore.getModels(),
    )
  }

  getIsRequestInProgress(): Promise<{ isRequestInProgress: boolean }> {
    return Promise.resolve({ isRequestInProgress: this.isRequestInProgress })
  }

  getConversationHistory(): Promise<{
    conversationHistory: Array<Mojom.ConversationTurn>
  }> {
    return Promise.resolve({ conversationHistory: [...this.chatHistory] })
  }

  generateQuestions(): void {
    if (
      this.suggestionGenerationStatus
      !== Mojom.SuggestionGenerationStatus.CanGenerate
    ) {
      return
    }

    this.suggestionGenerationStatus =
      Mojom.SuggestionGenerationStatus.IsGenerating
    this.notifySuggestedQuestionsChanged()

    // Generate questions based on conversation (no page content for now)
    this.engine.generateQuestionSuggestions(
      [],
      this.selectedLanguage,
      (result) => {
        if (!result.ok) {
          this.suggestionGenerationStatus =
            Mojom.SuggestionGenerationStatus.None
          this.notifySuggestedQuestionsChanged()
          return
        }

        this.suggestions = result.value.map((title) => ({
          title,
          actionType: Mojom.ActionType.SUGGESTION,
        }))
        this.suggestionGenerationStatus =
          Mojom.SuggestionGenerationStatus.HasGenerated
        this.notifySuggestedQuestionsChanged()
      },
    )
  }

  submitHumanConversationEntry(
    input: string,
    uploadedFiles: Array<Mojom.UploadedFile> | null,
  ): void {
    if (this.isRequestInProgress) return
    if (!input.trim() && (!uploadedFiles || uploadedFiles.length === 0)) return

    const turn = this.createHumanTurn(
      input,
      Mojom.ActionType.QUERY,
      undefined,
      uploadedFiles ?? undefined,
    )

    this.addToConversationHistory(turn)
    this.performAssistantGeneration()
  }

  submitHumanConversationEntryWithAction(
    input: string,
    actionType: Mojom.ActionType,
  ): void {
    if (this.isRequestInProgress) return

    const turn = this.createHumanTurn(input, actionType)
    this.addToConversationHistory(turn)
    this.performAssistantGeneration()
  }

  submitHumanConversationEntryWithSkill(input: string, skillId: string): void {
    if (this.isRequestInProgress) return

    const skill = this.config.getSkillById?.(skillId)
    if (!skill) return

    const skillEntry: Mojom.SkillEntry = {
      shortcut: skill.shortcut,
      prompt: skill.prompt,
    }

    const turn = this.createHumanTurn(
      input,
      Mojom.ActionType.QUERY,
      undefined,
      undefined,
      skillEntry,
    )

    this.addToConversationHistory(turn)
    this.performAssistantGeneration()
  }

  submitSummarizationRequest(): void {
    if (this.isRequestInProgress) return

    const turn = this.createHumanTurn('', Mojom.ActionType.SUMMARIZE_PAGE)
    this.addToConversationHistory(turn)
    this.performAssistantGeneration()
  }

  submitSuggestion(suggestion: string): void {
    const found = this.suggestions.find((s) => s.title === suggestion)
    if (!found) return

    const text = found.prompt ?? found.title
    const turn = this.createHumanTurn(text, found.actionType)

    // Remove the used suggestion
    this.suggestions = this.suggestions.filter((s) => s.title !== suggestion)
    this.notifySuggestedQuestionsChanged()

    this.addToConversationHistory(turn)
    this.performAssistantGeneration()
  }

  getAssociatedContentInfo(): Promise<{
    associatedContent: Array<Mojom.AssociatedContent>
  }> {
    // No associated content support for now
    return Promise.resolve({ associatedContent: [] })
  }

  getAPIResponseError(): Promise<{ error: Mojom.APIError }> {
    return Promise.resolve({ error: this.currentError })
  }

  retryAPIRequest(): void {
    if (this.isRequestInProgress) return
    if (this.currentError === Mojom.APIError.None) return

    this.currentError = Mojom.APIError.None
    this.performAssistantGeneration()
  }

  clearErrorAndGetFailedMessage(): Promise<{ turn: Mojom.ConversationTurn }> {
    this.currentError = Mojom.APIError.None

    // Get the last human message that failed
    let lastHumanTurn: Mojom.ConversationTurn | undefined
    for (let i = this.chatHistory.length - 1; i >= 0; i--) {
      if (this.chatHistory[i].characterType === Mojom.CharacterType.HUMAN) {
        lastHumanTurn = this.chatHistory[i]
        break
      }
    }

    if (!lastHumanTurn) {
      throw new Error('No human message found')
    }

    return Promise.resolve({ turn: lastHumanTurn })
  }

  stopGenerationAndMaybeGetHumanEntry(): Promise<{
    humanEntry: Mojom.ConversationTurn | null
  }> {
    this.engine.clearAllQueries()

    // If the last entry was human and there's no assistant response yet,
    // return it so the user can edit
    if (this.chatHistory.length > 0) {
      const lastEntry = this.chatHistory[this.chatHistory.length - 1]
      if (lastEntry.characterType === Mojom.CharacterType.HUMAN) {
        // Remove the last human entry
        this.chatHistory.pop()
        this.completeGeneration(false)
        return Promise.resolve({ humanEntry: lastEntry })
      }
    }

    this.completeGeneration(false)
    return Promise.resolve({ humanEntry: null })
  }

  rateMessage(
    isLiked: boolean,
    turnUuid: string,
  ): Promise<{ ratingId: string | null }> {
    // Not implemented - would need feedback API
    return Promise.resolve({ ratingId: null })
  }

  sendFeedback(
    category: string,
    feedback: string,
    ratingId: string,
    sendHostname: boolean,
  ): Promise<{ isSuccess: boolean }> {
    // Not implemented - would need feedback API
    return Promise.resolve({ isSuccess: false })
  }

  getScreenshots(): Promise<{ screenshots: Array<Mojom.UploadedFile> | null }> {
    // Not implemented
    return Promise.resolve({ screenshots: null })
  }

  setTemporary(temporary: boolean): void {
    // Can only set to temporary if conversation is empty
    if (this.chatHistory.length === 0) {
      this.metadata.temporary = temporary
    }
  }

  pauseTask(): void {
    if (this.toolUseTaskState === Mojom.TaskState.kRunning) {
      this.toolUseTaskState = Mojom.TaskState.kPaused
      this.notifyTaskStateChanged()
    }
  }

  resumeTask(): void {
    if (this.toolUseTaskState === Mojom.TaskState.kPaused) {
      this.toolUseTaskState = Mojom.TaskState.kRunning
      this.notifyTaskStateChanged()
      // Continue tool execution
      this.maybeRespondToNextToolUseRequest()
    }
  }

  stopTask(): void {
    if (
      this.toolUseTaskState === Mojom.TaskState.kRunning
      || this.toolUseTaskState === Mojom.TaskState.kPaused
    ) {
      this.toolUseTaskState = Mojom.TaskState.kStopped
      this.notifyTaskStateChanged()
      this.completeGeneration(false)
    }
  }

  // Try to respond to the next pending tool use request
  private maybeRespondToNextToolUseRequest(): boolean {
    if (this.toolUseTaskState !== Mojom.TaskState.kRunning) {
      return false
    }

    const lastEntry = this.getLastAssistantEntry()
    if (!lastEntry?.events) return false

    for (const event of lastEntry.events) {
      if (event.toolUseEvent && !event.toolUseEvent.output) {
        // Found a pending tool use request
        // In the browser, tools would be executed here
        // For now, we'll wait for external tool responses
        return true
      }
    }

    // No more pending tool use requests, continue generation
    this.performPostToolAssistantGeneration()
    return false
  }

  // Continue generation after tools have responded
  private async performPostToolAssistantGeneration() {
    this.isToolUseInProgress = false
    this.toolUseTaskState = Mojom.TaskState.kNone
    this.notifyTaskStateChanged()

    // Continue with assistant generation using tool results
    await this.performAssistantGeneration()
  }

  // UntrustedConversationHandlerInterface
  bindUntrustedConversationUI(
    untrustedUi: Mojom.UntrustedConversationUIRemote,
  ): Promise<{ conversationEntriesState: Mojom.ConversationEntriesState }> {
    // Leave as unimplemented as this is for a real Mojo environment
    throw new Error('Method not implemented.')
  }

  modifyConversation(entryUuid: string, newText: string): void {
    const entry = this.chatHistory.find((e) => e.uuid === entryUuid)
    if (!entry) return
    if (entry.characterType !== Mojom.CharacterType.HUMAN) return

    // Create an edit entry
    const edit: Mojom.ConversationTurn = {
      ...entry,
      uuid: generateUUID(),
      text: newText,
      createdTime: { internalValue: BigInt(Date.now() * 1000) },
    }

    if (!entry.edits) {
      entry.edits = []
    }
    entry.edits.push(edit)

    this.notifyHistoryUpdate(entry)

    // Remove all entries after this one and regenerate
    const entryIndex = this.chatHistory.indexOf(entry)
    if (entryIndex !== -1) {
      this.chatHistory = this.chatHistory.slice(0, entryIndex + 1)
      this.performAssistantGeneration()
    }
  }

  respondToToolUseRequest(
    toolId: string,
    outputJson: Array<Mojom.ContentBlock>,
  ): void {
    const toolEvent = this.getToolUseEventForLastResponse(toolId)
    if (!toolEvent) return

    toolEvent.output = outputJson
    toolEvent.permissionChallenge = undefined

    // Notify of the update
    const lastEntry = this.getLastAssistantEntry()
    if (lastEntry) {
      this.notifyHistoryUpdate(lastEntry)
    }

    // Check if we should continue the tool loop
    this.maybeRespondToNextToolUseRequest()
  }

  processPermissionChallenge(toolUseId: string, userResult: boolean): void {
    const toolEvent = this.getToolUseEventForLastResponse(toolUseId)
    if (!toolEvent) return

    if (userResult) {
      // User approved - clear the challenge and let tool execute
      toolEvent.permissionChallenge = undefined
    } else {
      // User denied - provide a denial output
      toolEvent.output = [
        {
          textContentBlock: { text: 'User denied permission for this action.' },
          imageContentBlock: undefined,
          fileContentBlock: undefined,
          pageExcerptContentBlock: undefined,
          pageTextContentBlock: undefined,
          videoTranscriptContentBlock: undefined,
          requestTitleContentBlock: undefined,
          changeToneContentBlock: undefined,
          memoryContentBlock: undefined,
          filterTabsContentBlock: undefined,
          suggestFocusTopicsContentBlock: undefined,
          suggestFocusTopicsWithEmojiContentBlock: undefined,
          reduceFocusTopicsContentBlock: undefined,
          simpleRequestContentBlock: undefined,
        } as Mojom.ContentBlock,
      ]
    }

    // Notify of the update
    const lastEntry = this.getLastAssistantEntry()
    if (lastEntry) {
      this.notifyHistoryUpdate(lastEntry)
    }

    // Continue the tool loop
    this.maybeRespondToNextToolUseRequest()
  }

  regenerateAnswer(turnUuid: string, modelKey: string): void {
    // Find the turn and the following assistant turn
    const turnIndex = this.chatHistory.findIndex((e) => e.uuid === turnUuid)
    if (turnIndex === -1) return

    const turn = this.chatHistory[turnIndex]
    if (turn.characterType !== Mojom.CharacterType.HUMAN) return

    // Update model key if specified
    if (modelKey && modelKey !== this.modelKey) {
      turn.modelKey = modelKey
      this.changeModel(modelKey)
    }

    // Remove everything after the human turn
    this.chatHistory = this.chatHistory.slice(0, turnIndex + 1)
    this.notifyHistoryUpdate(null) // Signal full refresh needed

    // Regenerate
    this.performAssistantGeneration()
  }
}

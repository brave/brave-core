// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'

interface FullConversationHandler
  extends Mojom.ConversationHandlerInterface,
    Mojom.UntrustedConversationHandlerInterface {}

export default class ConversationHandler implements FullConversationHandler {
  private observer: Mojom.ConversationUIInterface | null = null
  private entriesObserver: Mojom.UntrustedConversationUIInterface | null = null

  // Implement Closable<>
  public $ = { close() {} }

  setObserver(observer: Mojom.ConversationUIInterface) {
    this.observer = observer
    console.log(this.observer)
  }

  setEntriesObserver(observer: Mojom.UntrustedConversationUIInterface) {
    this.entriesObserver = observer
    console.log(this.entriesObserver)
  }

  // Mojom.ConversationHandlerInterface
  getState(): Promise<{ conversationState: Mojom.ConversationState }> {
    throw new Error('Method not implemented.')
  }

  getConversationUuid(): Promise<{ conversationUuid: string }> {
    throw new Error('Method not implemented.')
  }

  getModels(): Promise<{
    models: Array<Mojom.Model>
    currentModelKey: string
  }> {
    throw new Error('Method not implemented.')
  }

  changeModel(modelKey: string): void {
    throw new Error('Method not implemented.')
  }

  getIsRequestInProgress(): Promise<{ isRequestInProgress: boolean }> {
    throw new Error('Method not implemented.')
  }

  getConversationHistory(): Promise<{
    conversationHistory: Array<Mojom.ConversationTurn>
  }> {
    throw new Error('Method not implemented.')
  }

  generateQuestions(): void {
    throw new Error('Method not implemented.')
  }

  submitHumanConversationEntry(
    input: string,
    uploadedFiles: Array<Mojom.UploadedFile> | null,
  ): void {
    throw new Error('Method not implemented.')
  }

  submitHumanConversationEntryWithAction(
    input: string,
    actionType: Mojom.ActionType,
  ): void {
    throw new Error('Method not implemented.')
  }

  submitHumanConversationEntryWithSkill(input: string, skillId: string): void {
    throw new Error('Method not implemented.')
  }

  submitSummarizationRequest(): void {
    throw new Error('Method not implemented.')
  }

  submitSuggestion(suggestion: string): void {
    throw new Error('Method not implemented.')
  }

  getAssociatedContentInfo(): Promise<{
    associatedContent: Array<Mojom.AssociatedContent>
  }> {
    throw new Error('Method not implemented.')
  }

  getAPIResponseError(): Promise<{ error: Mojom.APIError }> {
    throw new Error('Method not implemented.')
  }

  retryAPIRequest(): void {
    throw new Error('Method not implemented.')
  }

  clearErrorAndGetFailedMessage(): Promise<{ turn: Mojom.ConversationTurn }> {
    throw new Error('Method not implemented.')
  }

  stopGenerationAndMaybeGetHumanEntry(): Promise<{
    humanEntry: Mojom.ConversationTurn | null
  }> {
    throw new Error('Method not implemented.')
  }

  rateMessage(
    isLiked: boolean,
    turnUuid: string,
  ): Promise<{ ratingId: string | null }> {
    throw new Error('Method not implemented.')
  }

  sendFeedback(
    category: string,
    feedback: string,
    ratingId: string,
    sendHostname: boolean,
  ): Promise<{ isSuccess: boolean }> {
    throw new Error('Method not implemented.')
  }

  getScreenshots(): Promise<{ screenshots: Array<Mojom.UploadedFile> | null }> {
    throw new Error('Method not implemented.')
  }

  setTemporary(temporary: boolean): void {
    throw new Error('Method not implemented.')
  }

  pauseTask(): void {
    throw new Error('Method not implemented.')
  }

  resumeTask(): void {
    throw new Error('Method not implemented.')
  }

  stopTask(): void {
    throw new Error('Method not implemented.')
  }

  // UntrustedConversationHandlerInterface
  bindUntrustedConversationUI(
    untrustedUi: Mojom.UntrustedConversationUIRemote,
  ): Promise<{ conversationEntriesState: Mojom.ConversationEntriesState }> {
    // Leave as unimplemented as this is for a real Mojo environment
    throw new Error('Method not implemented.')
  }

  modifyConversation(entryUuid: string, newText: string): void {
    throw new Error('Method not implemented.')
  }

  respondToToolUseRequest(
    toolId: string,
    outputJson: Array<Mojom.ContentBlock>,
  ): void {
    throw new Error('Method not implemented.')
  }

  processPermissionChallenge(toolUseId: string, userResult: boolean): void {
    throw new Error('Method not implemented.')
  }

  regenerateAnswer(turnUuid: string, modelKey: string): void {
    throw new Error('Method not implemented.')
  }
}

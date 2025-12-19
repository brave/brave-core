// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import type ConversationHandler from './conversation_handler'

export default class AIChatService implements Mojom.ServiceInterface {
  private observer: Mojom.ServiceObserverInterface | null = null

  private state: Mojom.ServiceState = {
    hasAcceptedAgreement: false,
    isStoragePrefEnabled: true,
    isStorageNoticeDismissed: true,
    canShowPremiumPrompt: false,
  }

  // Implement Closable<>
  public $ = { close() {} }

  newConversation(): ConversationHandler {
    throw new Error('Method not implemented.')
  }

  getConversationHandler(conversationUuid: string): ConversationHandler {
    // Get a conversation that's already created and loaded in-memory
    throw new Error('Method not implemented.')
  }

  // Mojom.ServiceInterface

  markAgreementAccepted(): void {
    // temporary:
    this.state.hasAcceptedAgreement = true
    this.observer?.onStateChanged(this.state)
  }

  enableStoragePref(): void {
    throw new Error('Method not implemented.')
  }

  dismissStorageNotice(): void {
    throw new Error('Method not implemented.')
  }

  dismissPremiumPrompt(): void {
    throw new Error('Method not implemented.')
  }

  getSkills(): Promise<{ skills: Array<Mojom.Skill> }> {
    throw new Error('Method not implemented.')
  }

  createSkill(shortcut: string, prompt: string, model: string | null): void {
    throw new Error('Method not implemented.')
  }

  updateSkill(
    id: string,
    shortcut: string,
    prompt: string,
    model: string | null,
  ): void {
    throw new Error('Method not implemented.')
  }

  deleteSkill(id: string): void {
    throw new Error('Method not implemented.')
  }

  getConversations(): Promise<{ conversations: Array<Mojom.Conversation> }> {
    throw new Error('Method not implemented.')
  }

  getActionMenuList(): Promise<{ actionList: Array<Mojom.ActionGroup> }> {
    throw new Error('Method not implemented.')
  }

  getPremiumStatus(): Promise<{
    status: Mojom.PremiumStatus
    info: Mojom.PremiumInfo | null
  }> {
    throw new Error('Method not implemented.')
  }

  deleteConversation(id: string): void {
    throw new Error('Method not implemented.')
  }

  renameConversation(id: string, newName: string): void {
    throw new Error('Method not implemented.')
  }

  conversationExists(conversationUuid: string): Promise<{ exists: boolean }> {
    throw new Error('Method not implemented.')
  }

  bindObserver(
    ui: Mojom.ServiceObserverRemote,
  ): Promise<{ state: Mojom.ServiceState }> {
    // Leave as not implemented as this is the real version
    throw new Error('Method not implemented.')
  }

  bindObserverInterface(
    ui: Mojom.ServiceObserverInterface,
  ): Promise<{ state: Mojom.ServiceState }> {
    this.observer = ui
    return Promise.resolve({ state: this.state })
  }

  bindConversation(
    conversationUuid: string,
    conversation: Mojom.ConversationHandlerPendingReceiver,
    conversationUi: Mojom.ConversationUIRemote,
  ): void {
    throw new Error('Method not implemented.')
  }

  bindMetrics(metrics: Mojom.MetricsPendingReceiver): void {
    throw new Error('Method not implemented.')
  }
}

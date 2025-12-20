// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from '../../common/mojom'
import ConversationHandler, {
  type ConversationHandlerConfig,
} from './conversation_handler'
import type { V1EngineConfig } from './engine/v1/v1_engine'
import type ModelStore from './stores/model_store'

// Generates a UUID
function generateUUID(): string {
  return crypto.randomUUID()
}

// Configuration for the service
export interface AIChatServiceConfig {
  engineConfig: V1EngineConfig
  modelStore: ModelStore
}

export default class AIChatService implements Mojom.ServiceInterface {
  private observer: Mojom.ServiceObserverInterface | null = null
  private config: AIChatServiceConfig

  // State
  private state: Mojom.ServiceState = {
    hasAcceptedAgreement: false,
    isStoragePrefEnabled: true,
    isStorageNoticeDismissed: true,
    canShowPremiumPrompt: false,
  }

  // Skills (in-memory)
  private skills: Mojom.Skill[] = []

  // Conversations (in-memory)
  private conversations: Map<string, Mojom.Conversation> = new Map()
  private conversationHandlers: Map<string, ConversationHandler> = new Map()

  // Implement Closable<>
  public $ = { close() {} }

  constructor(config: AIChatServiceConfig) {
    this.config = config
  }

  // Get modelStore for convenience
  get modelStore(): ModelStore {
    return this.config.modelStore
  }

  // Create a conversation handler config
  private createConversationHandlerConfig(): ConversationHandlerConfig {
    return {
      engineConfig: this.config.engineConfig,
      modelStore: this.config.modelStore,
      getSkillById: (id) => this.skills.find((s) => s.id === id),
    }
  }

  // Create a new conversation
  newConversation(): ConversationHandler {
    const uuid = generateUUID()
    const now = { internalValue: BigInt(Date.now() * 1000) }

    const conversation: Mojom.Conversation = {
      uuid,
      title: 'New Conversation',
      updatedTime: now,
      hasContent: false,
      modelKey: this.modelStore.getDefaultModelKey(),
      totalTokens: BigInt(0),
      trimmedTokens: BigInt(0),
      temporary: false,
      associatedContent: [],
    }

    this.conversations.set(uuid, conversation)

    const handler = new ConversationHandler(
      conversation,
      this.createConversationHandlerConfig(),
    )
    this.conversationHandlers.set(uuid, handler)

    // Notify observers
    this.notifyConversationListChanged()

    return handler
  }

  // Get or create a conversation handler
  getConversationHandler(conversationUuid: string): ConversationHandler | null {
    // Check if already loaded
    let handler = this.conversationHandlers.get(conversationUuid)
    if (handler) {
      return handler
    }

    // Check if conversation exists
    const conversation = this.conversations.get(conversationUuid)
    if (!conversation) {
      return null
    }

    // Create handler for existing conversation
    handler = new ConversationHandler(
      conversation,
      this.createConversationHandlerConfig(),
    )
    this.conversationHandlers.set(conversationUuid, handler)

    return handler
  }

  // Notify observers of conversation list changes
  private notifyConversationListChanged() {
    const conversations = Array.from(this.conversations.values())
    this.observer?.onConversationListChanged(conversations)
  }

  // Notify observers of state changes
  private notifyStateChanged() {
    this.observer?.onStateChanged(this.state)
  }

  // Notify observers of skills changes
  private notifySkillsChanged() {
    this.observer?.onSkillsChanged(this.skills)
  }

  // Mojom.ServiceInterface

  markAgreementAccepted(): void {
    this.state.hasAcceptedAgreement = true
    this.notifyStateChanged()
  }

  enableStoragePref(): void {
    this.state.isStoragePrefEnabled = true
    this.notifyStateChanged()
  }

  dismissStorageNotice(): void {
    this.state.isStorageNoticeDismissed = true
    this.notifyStateChanged()
  }

  dismissPremiumPrompt(): void {
    this.state.canShowPremiumPrompt = false
    this.notifyStateChanged()
  }

  getSkills(): Promise<{ skills: Array<Mojom.Skill> }> {
    return Promise.resolve({ skills: [...this.skills] })
  }

  createSkill(shortcut: string, prompt: string, model: string | null): void {
    const now = { internalValue: BigInt(Date.now() * 1000) }
    const skill: Mojom.Skill = {
      id: generateUUID(),
      shortcut,
      prompt,
      model: model ?? undefined,
      createdTime: now,
      lastUsed: now,
    }
    this.skills.push(skill)
    this.notifySkillsChanged()
  }

  updateSkill(
    id: string,
    shortcut: string,
    prompt: string,
    model: string | null,
  ): void {
    const skill = this.skills.find((s) => s.id === id)
    if (!skill) return

    skill.shortcut = shortcut
    skill.prompt = prompt
    skill.model = model ?? undefined

    this.notifySkillsChanged()
  }

  deleteSkill(id: string): void {
    this.skills = this.skills.filter((s) => s.id !== id)
    this.notifySkillsChanged()
  }

  getConversations(): Promise<{ conversations: Array<Mojom.Conversation> }> {
    const conversations = Array.from(this.conversations.values())
    // Sort by updated time, newest first
    conversations.sort((a, b) => {
      const aTime = Number(a.updatedTime.internalValue)
      const bTime = Number(b.updatedTime.internalValue)
      return bTime - aTime
    })
    return Promise.resolve({ conversations })
  }

  getActionMenuList(): Promise<{ actionList: Array<Mojom.ActionGroup> }> {
    // Return default action menu
    const actionList: Mojom.ActionGroup[] = [
      {
        category: 'Rewrite',
        entries: [
          {
            details: {
              label: 'Paraphrase',
              type: Mojom.ActionType.PARAPHRASE,
            },
            subheading: undefined,
          },
          {
            details: {
              label: 'Improve',
              type: Mojom.ActionType.IMPROVE,
            },
            subheading: undefined,
          },
        ],
      },
      {
        category: 'Change Tone',
        entries: [
          {
            details: {
              label: 'Professional',
              type: Mojom.ActionType.PROFESSIONALIZE,
            },
            subheading: undefined,
          },
          {
            details: {
              label: 'Academic',
              type: Mojom.ActionType.ACADEMICIZE,
            },
            subheading: undefined,
          },
          {
            details: {
              label: 'Casual',
              type: Mojom.ActionType.CASUALIZE,
            },
            subheading: undefined,
          },
        ],
      },
      {
        category: 'Change Length',
        entries: [
          {
            details: {
              label: 'Shorten',
              type: Mojom.ActionType.SHORTEN,
            },
            subheading: undefined,
          },
          {
            details: {
              label: 'Expand',
              type: Mojom.ActionType.EXPAND,
            },
            subheading: undefined,
          },
        ],
      },
    ]
    return Promise.resolve({ actionList })
  }

  getPremiumStatus(): Promise<{
    status: Mojom.PremiumStatus
    info: Mojom.PremiumInfo | null
  }> {
    // Default to inactive premium
    return Promise.resolve({
      status: Mojom.PremiumStatus.Inactive,
      info: null,
    })
  }

  deleteConversation(id: string): void {
    // Delete from conversations map
    this.conversations.delete(id)

    // Delete handler if loaded
    const handler = this.conversationHandlers.get(id)
    if (handler) {
      // Notify handler observers that conversation was deleted
      handler.setObserver(null as any)
      this.conversationHandlers.delete(id)
    }

    this.notifyConversationListChanged()
  }

  renameConversation(id: string, newName: string): void {
    const conversation = this.conversations.get(id)
    if (!conversation) return

    conversation.title = newName
    this.notifyConversationListChanged()
  }

  conversationExists(conversationUuid: string): Promise<{ exists: boolean }> {
    return Promise.resolve({
      exists: this.conversations.has(conversationUuid),
    })
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
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  // Bind conversation using interface (for non-Mojo environments)
  bindConversationInterface(
    conversationUuid: string,
    conversationUi: Mojom.ConversationUIInterface,
  ): ConversationHandler | null {
    const handler = this.getConversationHandler(conversationUuid)
    if (!handler) return null

    handler.setObserver(conversationUi)
    return handler
  }

  bindMetrics(metrics: Mojom.MetricsPendingReceiver): void {
    // Not implemented - metrics not needed for app
  }

  // Additional methods for app usage

  // Get all models
  getModels(): Mojom.Model[] {
    return this.modelStore.getModels()
  }

  // Get default model key
  getDefaultModelKey(): string {
    return this.modelStore.getDefaultModelKey()
  }

  // Set default model key
  setDefaultModelKey(key: string): void {
    this.modelStore.setDefaultModelKey(key)
  }
}

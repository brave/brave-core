// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Url } from 'gen/url/mojom/url.mojom.m'
import * as Mojom from '../../common/mojom'

interface FullUIHandler
  extends Mojom.AIChatUIHandlerInterface,
    Mojom.UntrustedUIHandlerInterface {}

export default class UIHandler implements FullUIHandler {
  // Implement Closable<>
  public $ = { close() {} }

  private observer: Mojom.ChatUIInterface | null = null

  // AIChatUIHandlerInterface
  openAIChatSettings(): void {
    throw new Error('Method not implemented.')
  }

  openMemorySettings(): void {
    throw new Error('Method not implemented.')
  }

  openConversationFullPage(conversationUuid: string): void {
    throw new Error('Method not implemented.')
  }

  openAIChatAgentProfile(): void {
    throw new Error('Method not implemented.')
  }

  openURL(url: Url): void {
    throw new Error('Method not implemented.')
  }

  openStorageSupportUrl(): void {
    throw new Error('Method not implemented.')
  }

  openModelSupportUrl(): void {
    throw new Error('Method not implemented.')
  }

  goPremium(): void {
    throw new Error('Method not implemented.')
  }

  refreshPremiumSession(): void {
    throw new Error('Method not implemented.')
  }

  managePremium(): void {
    throw new Error('Method not implemented.')
  }

  handleVoiceRecognition(conversationUuid: string): void {
    throw new Error('Method not implemented.')
  }

  showSoftKeyboard(): void {
    throw new Error('Method not implemented.')
  }

  uploadFile(
    useMediaCapture: boolean,
  ): Promise<{ uploadedFiles: Array<Mojom.UploadedFile> | null }> {
    throw new Error('Method not implemented.')
  }

  processImageFile(
    fileData: Array<number>,
    filename: string,
  ): Promise<{ processedFile: Mojom.UploadedFile | null }> {
    throw new Error('Method not implemented.')
  }

  getPluralString(
    key: string,
    count: number,
  ): Promise<{ pluralString: string }> {
    throw new Error('Method not implemented.')
  }

  closeUI(): void {
    throw new Error('Method not implemented.')
  }

  setChatUI(chatUi: Mojom.ChatUIRemote): Promise<{ isStandalone: boolean }> {
    // Keep as not implemented because this class doesn't deal with remotes
    throw new Error('Method not implemented.')
  }

  setChatUIInterface(
    chatUi: Mojom.ChatUIInterface,
  ): Promise<{ isStandalone: boolean }> {
    this.observer = chatUi
    console.log(this.observer)
    return Promise.resolve({ isStandalone: true })
  }

  bindRelatedConversation(
    conversation: Mojom.ConversationHandlerPendingReceiver,
    conversationUi: Mojom.ConversationUIRemote,
  ): void {
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  associateTab(tab: Mojom.TabData, conversationUuid: string): void {
    throw new Error('Method not implemented.')
  }

  associateUrlContent(url: Url, title: string, conversationUuid: string): void {
    throw new Error('Method not implemented.')
  }

  disassociateContent(
    content: Mojom.AssociatedContent,
    conversationUuid: string,
  ): void {
    throw new Error('Method not implemented.')
  }

  newConversation(
    conversation: Mojom.ConversationHandlerPendingReceiver,
    conversationUi: Mojom.ConversationUIRemote,
  ): void {
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  // UntrustedUIHandlerInterface
  bindConversationHandler(
    conversationId: string,
    conversationHandler: Mojom.UntrustedConversationHandlerPendingReceiver,
  ): void {
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  bindUntrustedUI(untrustedUi: Mojom.UntrustedUIRemote): void {
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  openSearchURL(query: string): void {
    throw new Error('Method not implemented.')
  }

  openLearnMoreAboutBraveSearchWithLeo(): void {
    throw new Error('Method not implemented.')
  }

  openURLFromResponse(url: Url): void {
    throw new Error('Method not implemented.')
  }

  openAIChatCustomizationSettings(): void {
    throw new Error('Method not implemented.')
  }

  addTabToThumbnailTracker(tabId: number): void {
    throw new Error('Method not implemented.')
  }

  removeTabFromThumbnailTracker(tabId: number): void {
    throw new Error('Method not implemented.')
  }

  bindParentPage(parentFrame: Mojom.ParentUIFramePendingReceiver): void {
    // Leave as not implemented as this is for Mojo platforms
    throw new Error('Method not implemented.')
  }

  deleteMemory(memory: string): void {
    throw new Error('Method not implemented.')
  }

  hasMemory(memory: string): Promise<{ exists: boolean }> {
    throw new Error('Method not implemented.')
  }
}

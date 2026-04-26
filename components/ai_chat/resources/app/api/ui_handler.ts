// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Url } from 'gen/url/mojom/url.mojom.m'
import * as Mojom from '../../common/mojom'
import MemoryStore, { type MemoryStoreObserver } from './stores/memory_store'

// URLs for various support/info pages
const LEO_BRAVE_SEARCH_SUPPORT_URL = 'https://brave.com/leo'
const BRAVE_SEARCH_URL = 'https://search.brave.com/search'

interface FullUIHandler
  extends Mojom.AIChatUIHandlerInterface,
    Mojom.UntrustedUIHandlerInterface {}

// Configuration for UIHandler
export interface UIHandlerConfig {
  // Memory store for shared memory access
  memoryStore: MemoryStore
  // Callback for opening URLs (can be customized for Capacitor)
  openUrl?: (url: string) => void
  // Callback for opening settings (platform-specific)
  openSettings?: () => void
  // Callback for opening memory settings (platform-specific)
  openMemorySettings?: () => void
  // Callback for voice recognition (platform-specific)
  handleVoiceRecognition?: (conversationUuid: string) => void
  // Callback for closing UI (platform-specific)
  closeUI?: () => void
  // Callback for showing soft keyboard (platform-specific)
  showSoftKeyboard?: () => void
  // Callback for going premium (platform-specific)
  goPremium?: () => void
  // Callback for managing premium (platform-specific)
  managePremium?: () => void
  // Callback for refreshing premium session (platform-specific)
  refreshPremiumSession?: () => void
}

export default class UIHandler implements FullUIHandler, MemoryStoreObserver {
  // Implement Closable<>
  public $ = { close() {} }

  private observer: Mojom.ChatUIInterface | null = null
  private untrustedObserver: Mojom.UntrustedUIInterface | null = null
  private config: UIHandlerConfig

  constructor(config: UIHandlerConfig) {
    this.config = config
    // Register as observer to get memory change notifications
    this.config.memoryStore.addObserver(this)
  }

  // MemoryStoreObserver implementation
  onMemoriesChanged(memories: string[]): void {
    this.untrustedObserver?.onMemoriesChanged(memories)
  }

  // Helper to open URLs
  private openUrlInternal(url: string): void {
    if (this.config.openUrl) {
      this.config.openUrl(url)
    } else {
      // Default: use window.open
      window.open(url, '_blank', 'noopener,noreferrer')
    }
  }

  // AIChatUIHandlerInterface
  openAIChatSettings(): void {
    if (this.config.openSettings) {
      this.config.openSettings()
    } else {
      console.warn('openAIChatSettings not implemented for this platform')
    }
  }

  openMemorySettings(): void {
    if (this.config.openMemorySettings) {
      this.config.openMemorySettings()
    } else {
      console.warn('openMemorySettings not implemented for this platform')
    }
  }

  openConversationFullPage(conversationUuid: string): void {
    // In a standalone app, this is likely already full page
    console.warn('openConversationFullPage not needed in standalone app')
  }

  openAIChatAgentProfile(): void {
    console.warn('openAIChatAgentProfile not implemented for this platform')
  }

  openURL(url: Url): void {
    if (url.url && url.url.startsWith('https://')) {
      this.openUrlInternal(url.url)
    }
  }

  openStorageSupportUrl(): void {
    this.openUrlInternal('https://support.brave.com/hc/en-us/articles/storage')
  }

  openModelSupportUrl(): void {
    this.openUrlInternal('https://support.brave.com/hc/en-us/articles/models')
  }

  goPremium(): void {
    if (this.config.goPremium) {
      this.config.goPremium()
    } else {
      // Default: open premium page
      this.openUrlInternal('https://account.brave.com/leo')
    }
  }

  refreshPremiumSession(): void {
    if (this.config.refreshPremiumSession) {
      this.config.refreshPremiumSession()
    } else {
      console.warn('refreshPremiumSession not implemented for this platform')
    }
  }

  managePremium(): void {
    if (this.config.managePremium) {
      this.config.managePremium()
    } else {
      // Default: open account management
      this.openUrlInternal('https://account.brave.com/')
    }
  }

  handleVoiceRecognition(conversationUuid: string): void {
    if (this.config.handleVoiceRecognition) {
      this.config.handleVoiceRecognition(conversationUuid)
    } else {
      console.warn('handleVoiceRecognition not implemented for this platform')
    }
  }

  showSoftKeyboard(): void {
    if (this.config.showSoftKeyboard) {
      this.config.showSoftKeyboard()
    } else {
      // Default: no-op on web
    }
  }

  async uploadFile(
    useMediaCapture: boolean,
  ): Promise<{ uploadedFiles: Array<Mojom.UploadedFile> | null }> {
    // Note: if we use a native file picker and we need to show
    // progress in between selecting a file and processing it,
    // we should call observer.onUploadFilesSelected which is
    // what occurs in the Browser WebUI platform.
    return new Promise((resolve) => {
      const input = document.createElement('input')
      input.type = 'file'
      input.multiple = true

      if (useMediaCapture) {
        input.accept = 'image/*'
        input.capture = 'environment'
      } else {
        input.accept = 'image/*,application/pdf'
      }

      input.onchange = async () => {
        if (!input.files || input.files.length === 0) {
          resolve({ uploadedFiles: null })
          return
        }

        const uploadedFiles: Mojom.UploadedFile[] = []

        for (const file of Array.from(input.files)) {
          try {
            const arrayBuffer = await file.arrayBuffer()
            const data = Array.from(new Uint8Array(arrayBuffer))

            let type: Mojom.UploadedFileType
            if (file.type.startsWith('image/')) {
              type = Mojom.UploadedFileType.kImage
            } else if (file.type === 'application/pdf') {
              type = Mojom.UploadedFileType.kPdf
            } else {
              continue // Skip unsupported types
            }

            uploadedFiles.push({
              filename: file.name,
              data,
              type,
              filesize: data.length,
            })
          } catch (error) {
            console.error('Error reading file:', error)
          }
        }

        resolve({
          uploadedFiles: uploadedFiles.length > 0 ? uploadedFiles : null,
        })
      }

      input.oncancel = () => {
        resolve({ uploadedFiles: null })
      }

      input.click()
    })
  }

  async processImageFile(
    fileData: Array<number>,
    filename: string,
  ): Promise<{ processedFile: Mojom.UploadedFile | null }> {
    // For now, just wrap the data as an image file
    // In a full implementation, you might want to resize/compress
    return {
      processedFile: {
        filename,
        data: fileData,
        type: Mojom.UploadedFileType.kImage,
        filesize: fileData.length,
      },
    }
  }

  async getPluralString(
    key: string,
    count: number,
  ): Promise<{ pluralString: string }> {
    // Basic pluralization - in a full implementation, use i18n library
    // This handles common patterns like "1 item" vs "2 items"
    const pluralRules = new Intl.PluralRules('en-US')
    const form = pluralRules.select(count)

    // Return a generic pattern - actual strings should come from i18n
    return {
      pluralString: `${count} ${form === 'one' ? 'item' : 'items'}`,
    }
  }

  closeUI(): void {
    if (this.config.closeUI) {
      this.config.closeUI()
    } else {
      console.warn('closeUI not implemented for this platform')
    }
  }

  setChatUI(chatUi: Mojom.ChatUIRemote): Promise<{ isStandalone: boolean }> {
    // Not implemented - this is for Mojo platforms
    throw new Error('Method not implemented - use setChatUIInterface instead')
  }

  setChatUIInterface(
    chatUi: Mojom.ChatUIInterface,
  ): Promise<{ isStandalone: boolean }> {
    this.observer = chatUi
    // Create a use of this.observer to avoid compile error. We might
    // need to call a method on it when implementing more of this app.
    console.debug('ChatUIInterface set:', this.observer)
    return Promise.resolve({ isStandalone: true })
  }

  bindRelatedConversation(
    conversation: Mojom.ConversationHandlerPendingReceiver,
    conversationUi: Mojom.ConversationUIRemote,
  ): void {
    // Not implemented - this is for Mojo platforms
    throw new Error('Method not implemented - use direct handler binding')
  }

  associateTab(tab: Mojom.TabData, conversationUuid: string): void {
    // Not applicable in standalone app - no browser tabs
    console.warn('associateTab not available in standalone app')
  }

  associateUrlContent(url: Url, title: string, conversationUuid: string): void {
    // Not applicable in standalone app
    console.warn('associateUrlContent not available in standalone app')
  }

  disassociateContent(
    content: Mojom.AssociatedContent,
    conversationUuid: string,
  ): void {
    // Not applicable in standalone app
    console.warn('disassociateContent not available in standalone app')
  }

  newConversation(
    conversation: Mojom.ConversationHandlerPendingReceiver,
    conversationUi: Mojom.ConversationUIRemote,
  ): void {
    // Not implemented - this is for Mojo platforms
    throw new Error(
      'Method not implemented - use AIChatService.newConversation',
    )
  }

  // UntrustedUIHandlerInterface
  bindConversationHandler(
    conversationId: string,
    conversationHandler: Mojom.UntrustedConversationHandlerPendingReceiver,
  ): void {
    // Not implemented - this is for Mojo platforms
    throw new Error(
      'Method not implemented - use AIChatService.bindConversationInterface',
    )
  }

  bindUntrustedUI(untrustedUi: Mojom.UntrustedUIRemote): void {
    // Not implemented - this is for Mojo platforms
    throw new Error(
      'Method not implemented - use bindUntrustedUIInterface instead',
    )
  }

  bindUntrustedUIInterface(untrustedUi: Mojom.UntrustedUIInterface): void {
    this.untrustedObserver = untrustedUi
  }

  openSearchURL(query: string): void {
    const encodedQuery = encodeURIComponent(query)
    this.openUrlInternal(`${BRAVE_SEARCH_URL}?q=${encodedQuery}`)
  }

  openLearnMoreAboutBraveSearchWithLeo(): void {
    this.openUrlInternal(LEO_BRAVE_SEARCH_SUPPORT_URL)
  }

  openURLFromResponse(url: Url): void {
    // Only allow HTTPS URLs from responses
    if (url.url && url.url.startsWith('https://')) {
      this.openUrlInternal(url.url)
    }
  }

  openAIChatCustomizationSettings(): void {
    if (this.config.openSettings) {
      this.config.openSettings()
    } else {
      console.warn(
        'openAIChatCustomizationSettings not implemented for this platform',
      )
    }
  }

  addTabToThumbnailTracker(tabId: number): void {
    // Not applicable in standalone app - no browser tabs
    console.warn('addTabToThumbnailTracker not available in standalone app')
  }

  removeTabFromThumbnailTracker(tabId: number): void {
    // Not applicable in standalone app - no browser tabs
    console.warn(
      'removeTabFromThumbnailTracker not available in standalone app',
    )
  }

  bindParentPage(parentFrame: Mojom.ParentUIFramePendingReceiver): void {
    // Not implemented - this is for Mojo platforms
    throw new Error('Method not implemented - not needed in standalone app')
  }

  deleteMemory(memory: string): void {
    this.config.memoryStore.deleteMemory(memory)
  }

  async hasMemory(memory: string): Promise<{ exists: boolean }> {
    return { exists: this.config.memoryStore.hasMemory(memory) }
  }

  // Additional methods for app usage

  // Get the memory store for direct access
  get memoryStore(): MemoryStore {
    return this.config.memoryStore
  }
}

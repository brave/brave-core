/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import 'chrome://resources/cr_elements/cr_input/cr_input.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { I18nMixin, I18nMixinInterface } from
  'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from
  'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { PrefsMixin, PrefsMixinInterface } from
  '/shared/settings/prefs/prefs_mixin.js'
import { BaseMixin, BaseMixinInterface } from '../base_mixin.js'
import {
  CustomizationOperationError
} from '../customization_settings.mojom-webui.js'
import {
  MAX_MEMORY_RECORD_LENGTH
} from '../ai_chat.mojom-webui.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'
import { getTemplate } from './memory_section.html.js'

const MemorySectionBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement))) as {
  new (): PolymerElement & PrefsMixinInterface & I18nMixinInterface &
    BaseMixinInterface
}

class MemorySection extends MemorySectionBase {
  static get is() {
    return 'memory-section'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      memoriesList_: {
        type: Array,
        value: []
      },
      editingMemoryItem_: { // holds the memory string being edited, or null
        type: String,
        value: null
      },
      showMemoryDialog_: {
        type: Boolean,
        value: false
      },
      editingMemory_: {  // holds the current user input
        type: String,
        value: ''
      },
      isMemoryInputValid_: {
        type: Boolean,
        computed: 'computeMemoryInputValid_(editingMemory_)'
      },
      showDeleteDialog_: {
        type: Boolean,
        value: false
      },
      deleteMemoryItem_: { // holds the memory string to delete, or null
        type: String,
        value: null
      },
      showDeleteAllDialog_: {
        type: Boolean,
        value: false
      },
      searchQuery_: {
        type: String,
        value: ''
      },
      // The memories related to searchQuery_ by meaning, found once typing
      // pauses.
      relatedMemories_: {
        type: Array,
        value: []
      }
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare memoriesList_: string[]
  declare editingMemoryItem_: string | null
  declare showMemoryDialog_: boolean
  declare editingMemory_: string
  declare isMemoryInputValid_: boolean
  declare showDeleteDialog_: boolean
  declare deleteMemoryItem_: string | null
  declare showDeleteAllDialog_: boolean
  declare searchQuery_: string
  declare relatedMemories_: string[]
  private relatedMemoriesTimer_: ReturnType<typeof setTimeout> | null = null
  private relatedMemoriesRequestId_ = 0

  override ready() {
    super.ready()
    this.loadMemories_()
    this.setupCallbacks_()
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (this.relatedMemoriesTimer_ !== null) {
      clearTimeout(this.relatedMemoriesTimer_)
      this.relatedMemoriesTimer_ = null
    }
  }

  setupCallbacks_() {
    const callbackRouter =
      this.browserProxy_.getCustomizationSettingsCallbackRouter()
    callbackRouter.onMemoriesChanged.addListener((memories: string[]) => {
      this.memoriesList_ = [...memories]
      this.relatedMemories_ = this.relatedMemories_.filter(
        (memory) => memories.includes(memory))
    })
  }

  loadMemories_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.getMemories().then((result: { memories: string[] }) => {
      this.memoriesList_ = result.memories
    })
  }

  saveMemory_(memoryItem: string, oldItem: string | null) {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()

    if (!oldItem) {
      // Adding new memory
      handler.addMemory(memoryItem).then(
        (result: { error: CustomizationOperationError | null }) => {
          if (result.error) {
            this.handleMemoryError_(result.error)
          }
        })
    } else {
      // Editing existing memory
      handler.editMemory(oldItem, memoryItem).then(
        (result: { error: CustomizationOperationError | null }) => {
          if (result.error) {
            this.handleMemoryError_(result.error)
          }
        })
    }
  }

  handleMemoryError_(error: CustomizationOperationError) {
    // Handle different error types
    switch (error) {
      // This is unlikely to happen since we have a length limit in frontend
      // too.
      case CustomizationOperationError.kInvalidLength:
        console.error('Memory input exceeds length limit')
        break
      // Should be rare, in theory it might happen if memory changes during
      // edit.
      case CustomizationOperationError.kNotFound:
        console.error('Can\'t find the target memory to edit')
        break
    }
  }

  handleDelete_(e: { model: { item: string | null }}) {
    this.deleteMemoryItem_ = e.model.item
    this.showDeleteDialog_ = true
  }

  onDeleteDialogCancel_() {
    this.showDeleteDialog_ = false
    this.deleteMemoryItem_ = null
  }

  onDeleteDialogConfirm_() {
    if (this.deleteMemoryItem_) {
      const handler = this.browserProxy_.getCustomizationSettingsHandler()
      handler.deleteMemory(this.deleteMemoryItem_)
    }
    this.showDeleteDialog_ = false
    this.deleteMemoryItem_ = null
  }

  handleEdit_(e: { model: { item: string | null }}) {
    this.editingMemoryItem_ = e.model.item
    this.editingMemory_ = e.model.item || ''
    this.showMemoryDialog_ = true
  }

  handleAddNewMemory_() {
    this.editingMemoryItem_ = null
    this.editingMemory_ = ''
    this.showMemoryDialog_ = true
  }

  private isTooLong(text: string) : boolean {
    return text.trim().length > MAX_MEMORY_RECORD_LENGTH
  }

  onDialogInput_(e: { value: string }) {
    this.editingMemory_ = e.value
  }

  computeMemoryInputValid_(editingMemory: string): boolean {
    const trimmedMemory = editingMemory.trim()
    return trimmedMemory.length > 0 && !this.isTooLong(trimmedMemory)
  }

  hasMemories_(memoriesList: string[]): boolean {
    return memoriesList.length > 0
  }

  onDialogSave_() {
    const trimmedMemory = this.editingMemory_.trim()
    this.saveMemory_(trimmedMemory, this.editingMemoryItem_)
    this.showMemoryDialog_ = false
    this.editingMemoryItem_ = null
    this.editingMemory_ = ''
  }

  onDialogCancel_() {
    this.showMemoryDialog_ = false
    this.editingMemoryItem_ = null
    this.editingMemory_ = ''
  }

  private getDialogTitle_(editingMemoryItem: string | null) {
    return editingMemoryItem === null
      ? this.i18n('braveLeoAssistantCreateMemoryDialogTitle')
      : this.i18n('braveLeoAssistantEditMemoryDialogTitle')
  }

  handleDeleteAllMemories_() {
    this.showDeleteAllDialog_ = true
  }

  onDeleteAllDialogCancel_() {
    this.showDeleteAllDialog_ = false
  }

  onDeleteAllDialogConfirm_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.deleteAllMemories()
    this.showDeleteAllDialog_ = false
  }

  onSearchInput_(e: { value: string }) {
    this.searchQuery_ = e.value
    this.scheduleRelatedMemoriesSearch_()
  }

  clearSearch_() {
    this.searchQuery_ = ''
    this.scheduleRelatedMemoriesSearch_()
  }

  // Searches by meaning once typing pauses, as brave://history's search field
  // does (see cr_search_field_mixin.ts): 500ms after the first character,
  // 100ms less for each further one, down to 200ms.
  scheduleRelatedMemoriesSearch_() {
    if (this.relatedMemoriesTimer_ !== null) {
      clearTimeout(this.relatedMemoriesTimer_)
      this.relatedMemoriesTimer_ = null
    }
    const query = this.searchQuery_.trim()
    if (!query) {
      this.relatedMemoriesRequestId_++
      this.relatedMemories_ = []
      return
    }
    const delayMs = 500 - 100 * (Math.min(query.length, 4) - 1)
    this.relatedMemoriesTimer_ = setTimeout(() => {
      this.relatedMemoriesTimer_ = null
      this.searchRelatedMemories_(query)
    }, delayMs)
  }

  searchRelatedMemories_(query: string) {
    const requestId = ++this.relatedMemoriesRequestId_
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.searchMemories(query).then(
      (result: { memories: string[] | null }) => {
        // A later search or a cleared box makes this result stale.
        if (requestId !== this.relatedMemoriesRequestId_) {
          return
        }
        this.relatedMemories_ = result.memories ?? []
      })
  }

  // The memories containing the query, except those already shown as related.
  getFilteredMemories_(memoriesList: string[], searchQuery: string,
                       relatedMemories: string[]): string[] {
    const unrelated = memoriesList.filter(
      memory => !relatedMemories.includes(memory))
    if (!searchQuery || !searchQuery.trim()) {
      return unrelated
    }

    const query = searchQuery.trim().toLowerCase()
    return unrelated.filter(memory =>
      memory.toLowerCase().includes(query)
    )
  }

  hasNoSearchResults_(memoriesList: string[], searchQuery: string,
                      relatedMemories: string[]): boolean {
    if (!searchQuery || !searchQuery.trim()) {
      return false // No search query means we're not searching
    }

    // If no memories exist at all, clear search instead of showing "no results"
    if (memoriesList.length === 0) {
      this.searchQuery_ = ''
      return false
    }

    const filteredResults = this.getFilteredMemories_(
      memoriesList, searchQuery, relatedMemories)
    return filteredResults.length === 0 && relatedMemories.length === 0
  }

  shouldShowFilteredMemories_(memoriesList: string[], searchQuery: string,
                              relatedMemories: string[]): boolean {
    return this.getFilteredMemories_(
      memoriesList, searchQuery, relatedMemories).length > 0
  }

  hasRelatedMemories_(relatedMemories: string[]): boolean {
    return relatedMemories.length > 0
  }

}

customElements.define(MemorySection.is, MemorySection)

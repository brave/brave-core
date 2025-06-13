/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import 'chrome://resources/cr_elements/cr_input/cr_input.js'
import 'chrome://resources/cr_elements/icons.html.js'

import { PrefsMixin, PrefsMixinInterface } from
  '/shared/settings/prefs/prefs_mixin.js'
import { I18nMixin, I18nMixinInterface } from
  'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from
  'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { BaseMixin, BaseMixinInterface } from '../base_mixin.js'
import { getTemplate } from './memory_section.html.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'

const USER_MEMORIES_PREF_KEY = 'brave.ai_chat.user_memories'

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
      isMemoryInputError_: {
        type: Boolean,
        value: false
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
      }
    }
  }

  static get observers() {
    return [
      'loadMemories_(prefs.brave.ai_chat.user_memories.value)',
    ]
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare memoriesList_: string[]
  declare editingMemoryItem_: string | null
  declare showMemoryDialog_: boolean
  declare editingMemory_: string
  declare isMemoryInputValid_: boolean
  declare isMemoryInputError_: boolean
  declare showDeleteDialog_: boolean
  declare deleteMemoryItem_: string | null
  declare showDeleteAllDialog_: boolean

  override ready() {
    super.ready()
    this.loadMemories_();
  }

  loadMemories_() {
    // Check if prefs and the specific pref are available before accessing
    if (this.prefs?.brave?.ai_chat?.user_memories?.value) {
      this.memoriesList_ = [...this.prefs.brave.ai_chat.user_memories.value];
    } else {
      // Initialize with empty array if prefs are not available yet
      this.memoriesList_ = [];
    }
  }

  saveMemory_(memoryItem: string, oldItem: string | null) {
    console.log('saveMemory_ called with oldItem:', oldItem);
    if (!oldItem) {
      console.log('Adding new memory');
      this.appendPrefListItem(USER_MEMORIES_PREF_KEY, memoryItem);
      // Manually reload memories immediately
      this.loadMemories_();
    } else {
      console.log('Updating memory from:', oldItem, 'to:', memoryItem);
      this.updatePrefListItem(USER_MEMORIES_PREF_KEY, oldItem, memoryItem);
      // Manually reload memories immediately
      this.loadMemories_();
    }
  }

  onMemoryDialogSave_(e: { detail: { memoryItem: string } }) {
    this.showMemoryDialog_ = false
    this.saveMemory_(e.detail.memoryItem, this.editingMemoryItem_);
    this.editingMemoryItem_ = null;
  }

  onMemoryDialogClose_() {
    this.editingMemoryItem_ = null;
    this.showMemoryDialog_ = false;
  }

  handleDelete_(e: any) {
    this.deleteMemoryItem_ = e.model.item;
    this.showDeleteDialog_ = true;
  }

  onDeleteDialogCancel_() {
    this.showDeleteDialog_ = false;
    this.deleteMemoryItem_ = null;
  }

  onDeleteDialogConfirm_() {
    if (this.deleteMemoryItem_) {
      this.deletePrefListItem(USER_MEMORIES_PREF_KEY, this.deleteMemoryItem_);
      this.loadMemories_();
    }
    this.showDeleteDialog_ = false;
    this.deleteMemoryItem_ = null;
  }

  handleEdit_(e: any) {
    this.editingMemoryItem_ = e.model.item;
    this.editingMemory_ = e.model.item || '';
    this.showMemoryDialog_ = true;
  }

  handleAddNewMemory_() {
    this.editingMemoryItem_ = null;
    this.editingMemory_ = '';
    this.showMemoryDialog_ = true;
  }

  onDialogInput_(e: any) {
    const trimmedValue = e.value.trim()
    this.editingMemory_ = trimmedValue
    this.isMemoryInputError_ = trimmedValue.length > 512
  }

  computeMemoryInputValid_(editingMemory: string): boolean {
    const trimmedMemory = editingMemory.trim()
    return trimmedMemory.length > 0 && trimmedMemory.length <= 512
  }

  computeHasMemories_(memoriesList: string[]): boolean {
    return memoriesList.length > 0
  }

  hasMemories_(memoriesList: string[]): boolean {
    return memoriesList.length > 0
  }

  onDialogSave_() {
    this.saveMemory_(this.editingMemory_, this.editingMemoryItem_);
    this.showMemoryDialog_ = false;
    this.editingMemoryItem_ = null;
    this.editingMemory_ = '';
  }

  onDialogCancel_() {
    this.showMemoryDialog_ = false;
    this.editingMemoryItem_ = null;
    this.editingMemory_ = '';
  }

  private getDialogTitle_(editingMemoryItem: string | null) {
    return editingMemoryItem === null
      ? this.i18n('braveLeoAssistantCreateMemoryDialogTitle')
      : this.i18n('braveLeoAssistantEditMemoryDialogTitle')
  }

  private getInputPlaceholder_(editingMemory: string) {
    return editingMemory === ''
      ? this.i18n('braveLeoAssistantMemoryInputPlaceholder')
      : ''
  }

  handleDeleteAllMemories_() {
    this.showDeleteAllDialog_ = true;
  }

  onDeleteAllDialogCancel_() {
    this.showDeleteAllDialog_ = false;
  }

  onDeleteAllDialogConfirm_() {
    this.setPrefValue(USER_MEMORIES_PREF_KEY, []);
    this.loadMemories_();
    this.showDeleteAllDialog_ = false;
  }
}

customElements.define(MemorySection.is, MemorySection)

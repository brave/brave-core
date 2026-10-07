/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'

import { I18nMixin, I18nMixinInterface } from
  'chrome://resources/cr_elements/i18n_mixin.js'
import { PolymerElement } from
  'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import { PrefsMixin, PrefsMixinInterface } from
  '/shared/settings/prefs/prefs_mixin.js'
import { BaseMixin, BaseMixinInterface } from '../base_mixin.js'
import {
  DreamNowResult,
  DreamNowStatus,
  LearnedMemoryItem,
  LearnedMemoryType
} from '../customization_settings.mojom-webui.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'
import { getTemplate } from './learned_memory_section.html.js'
import {
  matchesQuery,
  splitByQuery,
  TextSegment
} from './memory_search_utils.js'

// Mojo times count microseconds since 1601-01-01 (Windows epoch).
const WINDOWS_TO_UNIX_EPOCH_MS = 11644473600000

const LearnedMemorySectionBase =
  PrefsMixin(I18nMixin(BaseMixin(PolymerElement))) as {
    new (): PolymerElement & PrefsMixinInterface & I18nMixinInterface &
      BaseMixinInterface
  }

// Shows the memories that Leo learned from the saved chats (Dreaming), and the
// "Dream now" button. The section is hidden when learned memory is not
// available in the browser.
//
// The search box of the memory section searches this list too. This section
// gets the text of the box and the learned memories that are related to it by
// meaning, and it shows the related memories first, then those that contain
// the text, with the text marked. It tells how many memories it shows, so that
// the memory section can say when neither list has a result.
class LearnedMemorySection extends LearnedMemorySectionBase {
  static get is() {
    return 'learned-memory-section'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      available_: {
        type: Boolean,
        value: false
      },
      learnedMemories_: {
        type: Array,
        value: []
      },
      isDreaming_: {
        type: Boolean,
        value: false
      },
      dreamResult_: {
        type: String,
        value: ''
      },
      showDeleteAllDialog_: {
        type: Boolean,
        value: false
      },
      // The text in the search box of the memory section.
      searchQuery: {
        type: String,
        value: ''
      },
      // The uuids of the learned memories that are related to searchQuery by
      // meaning, best first.
      relatedUuids: {
        type: Array,
        value: () => []
      },
      isSearching_: {
        type: Boolean,
        computed: 'computeIsSearching_(searchQuery)'
      },
      // The related memories that are in the list.
      relatedMemories_: {
        type: Array,
        computed: 'computeRelatedMemories_(learnedMemories_, relatedUuids, ' +
            'isSearching_)'
      },
      // The memories of the list after the related ones: those that contain
      // searchQuery, or all of them when there is no search.
      listedMemories_: {
        type: Array,
        computed: 'computeListedMemories_(learnedMemories_, relatedMemories_, ' +
            'searchQuery, isSearching_)'
      },
      // For the memory section: how many learned memories there are, and how
      // many the search shows.
      learnedCount: {
        type: Number,
        value: 0,
        notify: true
      },
      matchCount: {
        type: Number,
        value: 0,
        notify: true
      }
    }
  }

  static get observers() {
    return [
      'updateCounts_(available_, learnedMemories_, relatedMemories_, ' +
          'listedMemories_)'
    ]
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare available_: boolean
  declare learnedMemories_: LearnedMemoryItem[]
  declare isDreaming_: boolean
  declare dreamResult_: string
  declare showDeleteAllDialog_: boolean
  declare searchQuery: string
  declare relatedUuids: string[]
  declare isSearching_: boolean
  declare relatedMemories_: LearnedMemoryItem[]
  declare listedMemories_: LearnedMemoryItem[]
  declare learnedCount: number
  declare matchCount: number

  override ready() {
    super.ready()
    this.loadLearnedMemories_()
  }

  loadLearnedMemories_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.getLearnedMemories().then(
      (result: { available: boolean, memories: LearnedMemoryItem[] }) => {
        this.available_ = result.available
        this.learnedMemories_ = result.memories
      })
  }

  onDreamNow_() {
    if (this.isDreaming_) {
      return
    }
    this.isDreaming_ = true
    this.dreamResult_ = ''
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.dreamNow().then((response: { result: DreamNowResult }) => {
      this.isDreaming_ = false
      this.dreamResult_ = this.getDreamResultText_(response.result)
      this.loadLearnedMemories_()
    })
  }

  getDreamResultText_(result: DreamNowResult): string {
    const counts = [
      result.turnsRead, result.turnsKept, result.memoriesAdded,
      result.memoriesUpdated
    ].map(String)
    switch (result.status) {
      case DreamNowStatus.kCompleted:
        return this.i18n('braveLeoAssistantDreamResultCompleted', ...counts)
      case DreamNowStatus.kTimedOut:
        return this.i18n('braveLeoAssistantDreamResultTimedOut', ...counts)
      case DreamNowStatus.kFailed:
        return this.i18n('braveLeoAssistantDreamResultFailed')
      case DreamNowStatus.kBusy:
        return this.i18n('braveLeoAssistantDreamResultBusy')
      case DreamNowStatus.kCanceled:
      case DreamNowStatus.kUnavailable:
      default:
        return this.i18n('braveLeoAssistantDreamResultUnavailable')
    }
  }

  onDelete_(e: { model: { item: LearnedMemoryItem } }) {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.deleteLearnedMemory(e.model.item.uuid).then(() => {
      this.loadLearnedMemories_()
    })
  }

  onDeleteAll_() {
    this.showDeleteAllDialog_ = true
  }

  onDeleteAllDialogCancel_() {
    this.showDeleteAllDialog_ = false
  }

  onDeleteAllDialogConfirm_() {
    this.showDeleteAllDialog_ = false
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.deleteAllLearnedMemories().then(() => {
      this.dreamResult_ = ''
      this.loadLearnedMemories_()
    })
  }

  // A delete during "Dream now" would stop the run, so the button waits.
  canDeleteAll_(memories: LearnedMemoryItem[], isDreaming: boolean): boolean {
    return memories.length > 0 && !isDreaming
  }

  computeIsSearching_(searchQuery: string): boolean {
    return !!searchQuery && !!searchQuery.trim()
  }

  computeRelatedMemories_(memories: LearnedMemoryItem[],
                          relatedUuids: string[],
                          isSearching: boolean): LearnedMemoryItem[] {
    if (!isSearching) {
      return []
    }
    // A related memory that was deleted since the search is not in the list.
    return relatedUuids
      .map(uuid => memories.find(memory => memory.uuid === uuid))
      .filter((memory): memory is LearnedMemoryItem => memory !== undefined)
  }

  computeListedMemories_(memories: LearnedMemoryItem[],
                         related: LearnedMemoryItem[], searchQuery: string,
                         isSearching: boolean): LearnedMemoryItem[] {
    if (!isSearching) {
      return memories
    }
    return memories.filter(memory =>
      !related.includes(memory) && matchesQuery(memory.text, searchQuery))
  }

  updateCounts_(available: boolean, memories: LearnedMemoryItem[],
                related: LearnedMemoryItem[], listed: LearnedMemoryItem[]) {
    this.learnedCount = available ? memories.length : 0
    this.matchCount = available ? related.length + listed.length : 0
  }

  getSegments_(item: LearnedMemoryItem, searchQuery: string): TextSegment[] {
    return splitByQuery(item.text, searchQuery)
  }

  getSegmentClass_(segment: TextSegment): string {
    return segment.match ? 'match' : ''
  }

  shouldShow_(available: boolean, memoryEnabled: boolean): boolean {
    return available && memoryEnabled
  }

  hasLearnedMemories_(memories: LearnedMemoryItem[]): boolean {
    return memories.length > 0
  }

  getDreamButtonText_(isDreaming: boolean): string {
    return isDreaming ? this.i18n('braveLeoAssistantDreamingLabel')
                      : this.i18n('braveLeoAssistantDreamNowButtonLabel')
  }

  hasDreamResult_(dreamResult: string): boolean {
    return dreamResult.length > 0
  }

  getTypeLabel_(item: LearnedMemoryItem): string {
    switch (item.type) {
      case LearnedMemoryType.kPermanent:
        return this.i18n('braveLeoAssistantLearnedMemoryPermanent')
      case LearnedMemoryType.kShortTerm:
        return this.i18n('braveLeoAssistantLearnedMemoryShortTerm')
      default:
        return ''
    }
  }

  getDateLabel_(item: LearnedMemoryItem): string {
    const unixMs =
      Number(item.updatedTime.internalValue / BigInt(1000)) -
      WINDOWS_TO_UNIX_EPOCH_MS
    return new Date(unixMs).toLocaleDateString()
  }

  getDetails_(item: LearnedMemoryItem): string {
    return [this.getTypeLabel_(item), this.getDateLabel_(item)]
      .filter(Boolean)
      .join(' · ')
  }

  hasPreviousText_(item: LearnedMemoryItem): boolean {
    return !!item.previousText
  }

  getPreviousText_(item: LearnedMemoryItem): string {
    return this.i18n('braveLeoAssistantLearnedMemoryPrevious',
                     item.previousText ?? '')
  }
}

customElements.define(LearnedMemorySection.is, LearnedMemorySection)

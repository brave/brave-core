/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'

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
      }
    }
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare available_: boolean
  declare learnedMemories_: LearnedMemoryItem[]
  declare isDreaming_: boolean
  declare dreamResult_: string

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

/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_checkbox/cr_checkbox.js'
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
  CustomizationOperationError,
  DreamNowResult,
  DreamNowStatus,
  LearnedMemoryChange,
  LearnedMemoryItem,
  LearnedMemoryProposal,
  LearnedMemoryReviewDecision,
  LearnedMemorySource,
  LearnedMemoryType,
  MemorySearchResult
} from '../customization_settings.mojom-webui.js'
import {
  MAX_MEMORY_RECORD_LENGTH
} from '../ai_chat.mojom-webui.js'
import {
  BraveLeoAssistantBrowserProxy,
  BraveLeoAssistantBrowserProxyImpl
} from './brave_leo_assistant_browser_proxy.js'
import { getTemplate } from './memory_section.html.js'
import {
  matchesQuery,
  splitByQuery,
  TextSegment
} from './memory_search_utils.js'

// Mojo times count microseconds since 1601-01-01 (Windows epoch).
const WINDOWS_TO_UNIX_EPOCH_MS = 11644473600000

// A memory in the list: one that the user wrote, or one that Leo learned from
// the saved chats (Dreaming).
export interface MemoryRow {
  // Tells the rows apart: the text of a memory that the user wrote, or the
  // uuid of a learned one.
  key: string
  text: string
  learned: boolean
  // The uuid of a learned memory. Empty for one that the user wrote.
  uuid: string
  // Leo does not change the memory. A memory that the user wrote is always
  // kept. A learned one is kept when the user chooses so.
  permanent: boolean
  shortTerm: boolean
  // The text before the last replace or merge of a learned memory.
  previousText: string
  // The date of the last change of a learned memory. Empty for the others.
  date: string
  // The date when Leo forgets a learned memory that it does not use. Empty for
  // a memory that is kept, and for the memories that the user wrote.
  expires: string
}

// A part of the list. The memories that the search found by meaning come in a
// part of their own, before the others.
export interface MemoryGroup {
  related: boolean
  rows: MemoryRow[]
}

// What is wrong with the text of a change of the review.
type ReviewTextError = '' | 'empty' | 'tooLong'

// A line with a part that opens a chat. The translated line stays whole: the
// page puts the link in place of a placeholder. Without `url` the line has no
// link, and all its text is in `before`.
interface LinkedText {
  before: string
  link: string
  after: string
  url: string
}

// A sentence that Leo read, under a change of the review.
interface ReviewQuote {
  sentence: string
  // The chat and the date of the sentence. Null when all the sentences come
  // from one chat: the line above them tells it.
  caption: LinkedText | null
}

// A change of the Dreaming review, as the user edits it.
interface ReviewItem {
  proposal: LearnedMemoryProposal
  keep: boolean
  text: string
  // The text box is open for the change.
  editing: boolean
  // The text when the box opened. Esc goes back to it.
  textBeforeEdit: string
  error: ReviewTextError
  // The first change of its kind, which has the heading of the kind above it.
  firstOfKind: boolean
  // The line that tells where the change came from. Null without sources.
  source: LinkedText | null
  quotes: ReviewQuote[]
  // The sentences show under the line. They show at first for a change of a
  // saved memory: the user needs them to decide.
  sourcesOpen: boolean
}

type LeoInputEvent = { value: string, model: { index: number } }
type LeoCheckboxEvent = { checked: boolean, model: { index: number } }

// The order of the kinds of change in the review.
const REVIEW_KIND_ORDER = [
  LearnedMemoryChange.kNew,
  LearnedMemoryChange.kChanged,
  LearnedMemoryChange.kSeenAgain
]

// The text box of a change shows this many lines at most, then it scrolls. A
// memory of the longest length takes about this many lines.
const REVIEW_TEXT_MAX_LINES = 8

// Takes the place of a chat title in a translated line. The title itself never
// goes through i18n(), which reads HTML: a title can contain "<".
const CHAT_TITLE_MARKER = 'LEO_CHAT_TITLE_MARKER'

const MemorySectionBase = PrefsMixin(I18nMixin(BaseMixin(PolymerElement))) as {
  new (): PolymerElement & PrefsMixinInterface & I18nMixinInterface &
    BaseMixinInterface
}

// One list of all that Leo remembers about the user: the memories that the
// user wrote, and the memories that Leo learned from the saved chats
// (Dreaming). The memories that the user wrote are always kept as they are.
// The user can keep a learned memory as it is too. Dreaming never changes
// such a memory.
//
// The learned memories come from the browser only when learned memory is
// available. Then the section also has the "Learn from chats" button.
//
// Dreaming stores nothing by itself: the changes of a run wait for the user's
// review. The review dialog opens when "Learn from chats" ends, and a notice
// offers it for a run of the daily timer. The user keeps or drops each change,
// and can edit its text first.
//
// One search box searches the whole list. The browser finds the memories that
// are related to the text by meaning, and they come first. Then come the
// memories that contain the text, with the text marked.
class MemorySection extends MemorySectionBase {
  static get is() {
    return 'memory-section'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      // The memories that the user wrote.
      memoriesList_: {
        type: Array,
        value: () => []
      },
      available_: {
        type: Boolean,
        value: false
      },
      learnedMemories_: {
        type: Array,
        value: () => []
      },
      rows_: {
        type: Array,
        computed: 'computeRows_(memoriesList_, learnedMemories_, available_)'
      },
      groups_: {
        type: Array,
        computed: 'computeGroups_(rows_, searchResults_, searchQuery)'
      },
      editingRow_: { // holds the memory being edited, or null for a new one
        type: Object,
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
      deleteRow_: { // holds the memory to delete, or null
        type: Object,
        value: null
      },
      showDeleteAllDialog_: {
        type: Boolean,
        value: false
      },
      isDreaming_: {
        type: Boolean,
        value: false
      },
      dreamResult_: {
        type: String,
        value: ''
      },
      reviewItems_: {
        type: Array,
        value: () => []
      },
      showReviewDialog_: {
        type: Boolean,
        value: false
      },
      reviewError_: {
        type: Boolean,
        value: false
      },
      // The changes are being saved.
      reviewSaving_: {
        type: Boolean,
        value: false
      },
      reviewKeptCount_: {
        type: Number,
        computed: 'computeKeptCount_(reviewItems_.*)'
      },
      reviewAllKept_: {
        type: Boolean,
        computed: 'computeAllKept_(reviewKeptCount_, reviewItems_.length)'
      },
      // The text in the search box.
      searchQuery: {
        type: String,
        value: ''
      },
      // The memories that are related to searchQuery by meaning, best first,
      // found once typing pauses.
      searchResults_: {
        type: Array,
        value: () => []
      }
    }
  }

  static get observers() {
    return [
      'onRowsChanged_(rows_)',
      'onReviewLengthChanged_(reviewItems_.length)'
    ]
  }

  browserProxy_: BraveLeoAssistantBrowserProxy =
    BraveLeoAssistantBrowserProxyImpl.getInstance()
  declare memoriesList_: string[]
  declare available_: boolean
  declare learnedMemories_: LearnedMemoryItem[]
  declare rows_: MemoryRow[]
  declare groups_: MemoryGroup[]
  declare editingRow_: MemoryRow | null
  declare showMemoryDialog_: boolean
  declare editingMemory_: string
  declare isMemoryInputValid_: boolean
  declare showDeleteDialog_: boolean
  declare deleteRow_: MemoryRow | null
  declare showDeleteAllDialog_: boolean
  declare isDreaming_: boolean
  declare dreamResult_: string
  declare reviewItems_: ReviewItem[]
  declare showReviewDialog_: boolean
  declare reviewError_: boolean
  declare reviewSaving_: boolean
  declare reviewKeptCount_: number
  declare reviewAllKept_: boolean
  declare searchQuery: string
  declare searchResults_: MemorySearchResult[]
  private searchTimer_: ReturnType<typeof setTimeout> | null = null
  private searchRequestId_ = 0

  override ready() {
    super.ready()
    this.loadMemories_()
    this.loadLearnedMemories_()
    this.loadReview_()
    this.setupCallbacks_()
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (this.searchTimer_ !== null) {
      clearTimeout(this.searchTimer_)
      this.searchTimer_ = null
    }
  }

  setupCallbacks_() {
    const callbackRouter =
      this.browserProxy_.getCustomizationSettingsCallbackRouter()
    callbackRouter.onMemoriesChanged.addListener((memories: string[]) => {
      this.memoriesList_ = [...memories]
    })
  }

  loadMemories_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.getMemories().then((result: { memories: string[] }) => {
      this.memoriesList_ = result.memories
    })
  }

  loadLearnedMemories_() {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.getLearnedMemories().then(
      (result: { available: boolean, memories: LearnedMemoryItem[] }) => {
        this.available_ = result.available
        this.learnedMemories_ = result.memories
      })
  }

  // Resolves with the number of changes that wait for review.
  loadReview_(): Promise<number> {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    return handler.getDreamingReview().then(
      (result: { proposals: LearnedMemoryProposal[] }) => {
        // New memories first, then the changed ones, then the ones that came
        // again. Each kind keeps the order of the run.
        const ordered = result.proposals
          .map((proposal, index) => ({ proposal, index }))
          .sort((a, b) =>
            REVIEW_KIND_ORDER.indexOf(a.proposal.change) -
              REVIEW_KIND_ORDER.indexOf(b.proposal.change) ||
            a.index - b.index)
          .map(entry => entry.proposal)
        this.reviewItems_ = ordered.map((proposal, i) => ({
          proposal,
          keep: true,
          text: proposal.text,
          editing: false,
          textBeforeEdit: proposal.text,
          error: '',
          firstOfKind: i === 0 || ordered[i - 1].change !== proposal.change,
          ...this.getReviewSources_(proposal.sources),
          sourcesOpen: proposal.change === LearnedMemoryChange.kChanged
        }))
        return this.reviewItems_.length
      })
  }

  // The list. The memories that Leo keeps as they are come first: those that
  // the user wrote, then the learned ones that the user keeps. The other
  // learned memories follow, the newest first.
  computeRows_(memories: string[], learned: LearnedMemoryItem[],
               available: boolean): MemoryRow[] {
    const written = memories.map((text): MemoryRow => ({
      key: `written:${text}`,
      text,
      learned: false,
      uuid: '',
      permanent: true,
      shortTerm: false,
      previousText: '',
      date: '',
      expires: ''
    }))
    const learnedRows = (available ? learned : []).map((item): MemoryRow => ({
      key: `learned:${item.uuid}`,
      text: item.text,
      learned: true,
      uuid: item.uuid,
      permanent: item.type === LearnedMemoryType.kPermanent,
      shortTerm: item.type === LearnedMemoryType.kShortTerm,
      previousText: item.previousText ?? '',
      date: this.formatDate_(item.updatedTime),
      expires: item.expireTime ? this.formatDate_(item.expireTime) : ''
    }))
    return [
      ...written,
      ...learnedRows.filter(row => row.permanent),
      ...learnedRows.filter(row => !row.permanent)
    ]
  }

  formatDate_(time: { internalValue: bigint }): string {
    const unixMs =
      Number(time.internalValue / BigInt(1000)) - WINDOWS_TO_UNIX_EPOCH_MS
    return new Date(unixMs).toLocaleDateString()
  }

  // The line under a change of the review: the chat and the date of the last
  // sentence, with a link that opens the chat at the sentence. When the
  // sentences come from more than one chat, the line does not name a chat, and
  // each sentence tells and links its own.
  getReviewSources_(sources: LearnedMemorySource[]):
      { source: LinkedText | null, quotes: ReviewQuote[] } {
    if (sources.length === 0) {
      return { source: null, quotes: [] }
    }
    const last = sources.reduce((a, b) =>
      b.date.internalValue > a.date.internalValue ? b : a)
    const severalChats =
      new Set(sources.map(source => source.conversationUuid)).size > 1
    const source = severalChats
      ? {
          before: this.i18n('braveLeoAssistantDreamReviewSourceSeveral',
                            this.formatDate_(last.date)),
          link: '', after: '', url: ''
        }
      : this.linkChat_('braveLeoAssistantDreamReviewSource',
                       'braveLeoAssistantDreamReviewSourceUntitled', last)
    const quotes = sources.map((source): ReviewQuote => ({
      sentence: source.sentence,
      caption: severalChats
        ? this.linkChat_('braveLeoAssistantDreamReviewSourceCaption',
                         'braveLeoAssistantDreamReviewSourceCaptionUntitled',
                         source)
        : null
    }))
    return { source, quotes }
  }

  // The line |id| with the title of the chat of |source| as a link. A chat
  // without a title gets the line |untitledId|, all of it a link.
  linkChat_(id: string, untitledId: string,
            source: LearnedMemorySource): LinkedText {
    const date = this.formatDate_(source.date)
    const url = this.getChatUrl_(source)
    if (!source.conversationTitle) {
      return { before: '', link: this.i18n(untitledId, date), after: '', url }
    }
    const [before, after = ''] =
      this.i18n(id, CHAT_TITLE_MARKER, date).split(CHAT_TITLE_MARKER)
    return { before, link: source.conversationTitle, after, url }
  }

  // The chat at the message of the sentence. The Leo page scrolls to the
  // message and marks it.
  getChatUrl_(source: LearnedMemorySource): string {
    const fragment = new URLSearchParams({ entry: source.entryUuid })
    return `chrome://leo-ai/${encodeURIComponent(source.conversationUuid)}` +
      `#${fragment}`
  }

  // Without a search, one part with all the memories. With a search, the
  // related memories first, in the order of the search, then the memories that
  // contain the text.
  computeGroups_(rows: MemoryRow[], results: MemorySearchResult[],
                 searchQuery: string): MemoryGroup[] {
    if (rows.length === 0) {
      return []
    }
    if (!this.isSearching_(searchQuery)) {
      return [{ related: false, rows }]
    }
    // A memory that was deleted since the search is not in the list.
    const related: MemoryRow[] = []
    for (const result of results) {
      const key = result.learnedUuid !== null
        ? `learned:${result.learnedUuid}`
        : `written:${result.text}`
      const row = rows.find(candidate => candidate.key === key)
      if (row && !related.includes(row)) {
        related.push(row)
      }
    }
    const matching = rows.filter(row =>
      !related.includes(row) && matchesQuery(row.text, searchQuery))
    const groups: MemoryGroup[] = []
    if (related.length > 0) {
      groups.push({ related: true, rows: related })
    }
    if (matching.length > 0) {
      groups.push({ related: false, rows: matching })
    }
    return groups
  }

  onRowsChanged_(rows: MemoryRow[]) {
    // There is nothing to search in. Do not leave a search that shows nothing.
    if (rows.length === 0 && this.searchQuery) {
      this.clearSearch_()
    }
  }

  saveMemory_(memoryItem: string, oldRow: MemoryRow | null) {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()

    if (!oldRow) {
      // Adding new memory
      handler.addMemory(memoryItem).then(
        (result: { error: CustomizationOperationError | null }) => {
          if (result.error) {
            this.handleMemoryError_(result.error)
          }
        })
    } else if (oldRow.learned) {
      // Editing a learned memory. The list and the review can change.
      handler.editLearnedMemory(oldRow.uuid, memoryItem).then(
        (result: { error: CustomizationOperationError | null }) => {
          if (result.error) {
            this.handleMemoryError_(result.error)
          }
          this.loadLearnedMemories_()
          this.loadReview_()
        })
    } else {
      // Editing a memory that the user wrote
      handler.editMemory(oldRow.text, memoryItem).then(
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

  // Delete one memory, after the user confirms.

  handleDelete_(e: { model: { row: MemoryRow }}) {
    this.deleteRow_ = e.model.row
    this.showDeleteDialog_ = true
  }

  onDeleteDialogCancel_() {
    this.showDeleteDialog_ = false
    this.deleteRow_ = null
  }

  onDeleteDialogConfirm_() {
    const row = this.deleteRow_
    this.showDeleteDialog_ = false
    this.deleteRow_ = null
    if (!row) {
      return
    }
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    if (!row.learned) {
      // The browser tells the page about the new list.
      handler.deleteMemory(row.text)
      return
    }
    handler.deleteLearnedMemory(row.uuid).then(() => {
      this.loadLearnedMemories_()
      // The change of the memory leaves the review.
      this.loadReview_()
    })
  }

  // Edit and add.

  handleEdit_(e: { model: { row: MemoryRow }}) {
    this.editingRow_ = e.model.row
    this.editingMemory_ = e.model.row.text
    this.showMemoryDialog_ = true
  }

  handleAddNewMemory_() {
    this.editingRow_ = null
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

  onDialogSave_() {
    const trimmedMemory = this.editingMemory_.trim()
    this.saveMemory_(trimmedMemory, this.editingRow_)
    this.showMemoryDialog_ = false
    this.editingRow_ = null
    this.editingMemory_ = ''
  }

  onDialogCancel_() {
    this.showMemoryDialog_ = false
    this.editingRow_ = null
    this.editingMemory_ = ''
  }

  private getDialogTitle_(editingRow: MemoryRow | null) {
    return editingRow === null
      ? this.i18n('braveLeoAssistantCreateMemoryDialogTitle')
      : this.i18n('braveLeoAssistantEditMemoryDialogTitle')
  }

  // Keep a learned memory as it is.

  onToggleKeep_(e: { model: { row: MemoryRow } }) {
    const row = e.model.row
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.setLearnedMemoryPermanent(row.uuid, !row.permanent).then(() => {
      this.loadLearnedMemories_()
      // A new text for a memory that the user keeps leaves the review.
      this.loadReview_()
    })
  }

  getKeepIcon_(row: MemoryRow): string {
    return row.permanent ? 'lock' : 'lock-open'
  }

  getKeepClass_(row: MemoryRow): string {
    return row.permanent ? 'keep kept' : 'keep'
  }

  getKeepLabel_(row: MemoryRow): string {
    return row.permanent
      ? this.i18n('braveLeoAssistantMemoryKeptAsIs')
      : this.i18n('braveLeoAssistantMemoryKeepAsIs')
  }

  // Delete all.

  handleDeleteAllMemories_() {
    this.showDeleteAllDialog_ = true
  }

  onDeleteAllDialogCancel_() {
    this.showDeleteAllDialog_ = false
  }

  onDeleteAllDialogConfirm_() {
    this.showDeleteAllDialog_ = false
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.deleteAllMemories()
    if (!this.available_) {
      return
    }
    handler.deleteAllLearnedMemories().then(() => {
      this.dreamResult_ = ''
      this.loadLearnedMemories_()
      // A review that waits is discarded.
      this.loadReview_()
    })
  }

  // A delete during "Learn from chats" would stop the run, so the button waits.
  canDeleteAll_(rows: MemoryRow[], isDreaming: boolean): boolean {
    return rows.length > 0 && !isDreaming
  }

  // Search.

  onSearchInput_(e: { value: string }) {
    this.searchQuery = e.value
    this.scheduleRelatedMemoriesSearch_()
  }

  clearSearch_() {
    this.searchQuery = ''
    this.scheduleRelatedMemoriesSearch_()
  }

  // Searches by meaning once typing pauses, as brave://history's search field
  // does (see cr_search_field_mixin.ts): 500ms after the first character,
  // 100ms less for each further one, down to 200ms.
  scheduleRelatedMemoriesSearch_() {
    if (this.searchTimer_ !== null) {
      clearTimeout(this.searchTimer_)
      this.searchTimer_ = null
    }
    const query = this.searchQuery.trim()
    if (!query) {
      this.searchRequestId_++
      this.searchResults_ = []
      return
    }
    const delayMs = 500 - 100 * (Math.min(query.length, 4) - 1)
    this.searchTimer_ = setTimeout(() => {
      this.searchTimer_ = null
      this.searchRelatedMemories_(query)
    }, delayMs)
  }

  searchRelatedMemories_(query: string) {
    const requestId = ++this.searchRequestId_
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.searchMemories(query).then(
      (response: { results: MemorySearchResult[] | null }) => {
        // A later search or a cleared box makes this result stale.
        if (requestId !== this.searchRequestId_) {
          return
        }
        this.searchResults_ = response.results ?? []
      })
  }

  // The memory cut into the parts that the search box matches and the rest,
  // for the page to mark.
  getSegments_(row: MemoryRow, searchQuery: string): TextSegment[] {
    return splitByQuery(row.text, searchQuery)
  }

  getSegmentClass_(segment: TextSegment): string {
    return segment.match ? 'match' : ''
  }

  isSearching_(searchQuery: string): boolean {
    return !!searchQuery && !!searchQuery.trim()
  }

  // The empty list only says that there is nothing to search in.
  showEmptyList_(rows: MemoryRow[], searchQuery: string): boolean {
    return rows.length === 0 && !this.isSearching_(searchQuery)
  }

  hasNoSearchResults_(rows: MemoryRow[], groups: MemoryGroup[],
                      searchQuery: string): boolean {
    return rows.length > 0 && groups.length === 0 &&
        this.isSearching_(searchQuery)
  }

  hasRows_(rows: MemoryRow[]): boolean {
    return rows.length > 0
  }

  getGroupClass_(group: MemoryGroup): string {
    return group.related ? 'list related-memories' : 'list'
  }

  // What the line under a memory tells: who made it, and for a learned memory
  // when it changed.
  getDetails_(row: MemoryRow): string {
    if (!row.learned) {
      return this.i18n('braveLeoAssistantMemorySourceWritten')
    }
    return [
      this.i18n('braveLeoAssistantMemorySourceLearned'),
      row.shortTerm ? this.i18n('braveLeoAssistantLearnedMemoryShortTerm') : '',
      row.date,
      row.expires ? this.i18n('braveLeoAssistantMemoryExpires', row.expires)
                  : ''
    ].filter(Boolean).join(' · ')
  }

  // The tooltip of the line under a memory: how a learned memory expires.
  getDetailsTitle_(row: MemoryRow): string {
    return row.expires ? this.i18n('braveLeoAssistantMemoryExpiresTooltip') : ''
  }

  hasPreviousText_(row: MemoryRow): boolean {
    return !!row.previousText
  }

  getPreviousText_(row: MemoryRow): string {
    return this.i18n('braveLeoAssistantLearnedMemoryPrevious',
                     row.previousText)
  }

  // Dreaming.

  onDreamNow_() {
    if (this.isDreaming_) {
      return
    }
    // The changes of the last run come first.
    if (this.reviewItems_.length > 0) {
      this.openReview_()
      return
    }
    this.isDreaming_ = true
    this.dreamResult_ = ''
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    handler.dreamNow().then((response: { result: DreamNowResult }) => {
      this.isDreaming_ = false
      this.loadLearnedMemories_()
      this.loadReview_().then(count => {
        this.dreamResult_ = this.getDreamResultText_(response.result, count)
        if (count > 0) {
          this.openReview_()
        }
      })
    })
  }

  getDreamResultText_(result: DreamNowResult, reviewCount: number): string {
    const counts = [
      result.turnsRead, result.turnsKept, result.memoriesAdded,
      result.memoriesUpdated
    ].map(String)
    if (reviewCount > 0 && (result.status === DreamNowStatus.kCompleted ||
                            result.status === DreamNowStatus.kTimedOut)) {
      return this.i18n('braveLeoAssistantDreamResultReview',
                       String(result.turnsRead))
    }
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

  getDreamButtonText_(isDreaming: boolean): string {
    return isDreaming ? this.i18n('braveLeoAssistantDreamingLabel')
                      : this.i18n('braveLeoAssistantDreamNowButtonLabel')
  }

  hasDreamResult_(dreamResult: string): boolean {
    return dreamResult.length > 0
  }

  // The review of the changes of a Dreaming run.

  openReview_() {
    this.reviewError_ = false
    this.showReviewDialog_ = true
  }

  onReviewDialogClose_() {
    // The review stays for later.
    this.showReviewDialog_ = false
  }

  // The dialog has nothing to show when no change is left, for example after
  // the memories of the changes were deleted.
  onReviewLengthChanged_(length: number) {
    if (length === 0 && this.showReviewDialog_) {
      this.showReviewDialog_ = false
    }
  }

  computeKeptCount_(): number {
    return this.reviewItems_.filter(item => item.keep).length
  }

  computeAllKept_(keptCount: number, total: number): boolean {
    return total > 0 && keptCount === total
  }

  getKeptCountText_(keptCount: number, total: number): string {
    return this.i18n('braveLeoAssistantDreamReviewSelectedCount',
                     String(keptCount), String(total))
  }

  onReviewKeepChange_(e: LeoCheckboxEvent) {
    this.set(`reviewItems_.${e.model.index}.keep`, e.checked)
  }

  onReviewKeepAllChange_(e: { checked: boolean }) {
    for (let i = 0; i < this.reviewItems_.length; ++i) {
      this.set(`reviewItems_.${i}.keep`, e.checked)
    }
  }

  // Opens the text box of a change. A change that the user edits is one that
  // the user wants to keep.
  onReviewEdit_(e: { model: { index: number } }) {
    const index = e.model.index
    this.set(`reviewItems_.${index}.textBeforeEdit`,
             this.reviewItems_[index].text)
    this.set(`reviewItems_.${index}.keep`, true)
    this.set(`reviewItems_.${index}.editing`, true)
    this.focusReviewText_(index)
  }

  onReviewTextInput_(e: LeoInputEvent) {
    const index = e.model.index
    this.set(`reviewItems_.${index}.text`, e.value)
    const textarea = this.getReviewTextArea_(index)
    if (textarea) {
      this.fitReviewText_(textarea)
    }
    // An error goes as soon as the text is fixed. A new one waits for the end
    // of the edit or for the save.
    if (this.reviewItems_[index].error &&
        !this.getReviewTextError_(e.value)) {
      this.set(`reviewItems_.${index}.error`, '')
    }
  }

  // Enter ends the edit, because a memory is one line. Esc goes back to the
  // text from before the edit, and leaves the dialog open: it closes one layer
  // at a time.
  onReviewTextKeyDown_(
      e: { innerEvent: KeyboardEvent, model: { index: number } }) {
    // The text box is a Leo component. It gives the event of the key in
    // `innerEvent`.
    const key = e.innerEvent
    const index = e.model.index
    if (key.key === 'Enter' && !key.shiftKey) {
      key.preventDefault()
      this.finishReviewEdit_(index)
    } else if (key.key === 'Escape') {
      key.preventDefault()
      key.stopPropagation()
      this.cancelReviewEdit_(index)
    }
  }

  onReviewEditDone_(e: { model: { index: number } }) {
    this.finishReviewEdit_(e.model.index)
  }

  // The button does what Esc does.
  onReviewEditCancel_(e: { model: { index: number } }) {
    this.cancelReviewEdit_(e.model.index)
  }

  cancelReviewEdit_(index: number) {
    this.set(`reviewItems_.${index}.text`,
             this.reviewItems_[index].textBeforeEdit)
    this.set(`reviewItems_.${index}.error`, '')
    this.set(`reviewItems_.${index}.editing`, false)
    this.focusReviewEditButton_(index)
  }

  finishReviewEdit_(index: number) {
    const error = this.getReviewTextError_(this.reviewItems_[index].text)
    this.set(`reviewItems_.${index}.error`, error)
    if (error) {
      this.focusReviewText_(index)
      return
    }
    this.set(`reviewItems_.${index}.editing`, false)
    this.focusReviewEditButton_(index)
  }

  getReviewTextError_(text: string): ReviewTextError {
    const trimmed = text.trim()
    if (trimmed.length === 0) {
      return 'empty'
    }
    return trimmed.length > MAX_MEMORY_RECORD_LENGTH ? 'tooLong' : ''
  }

  // The text area inside the Leo text box of the change |index|.
  getReviewTextArea_(index: number): HTMLTextAreaElement | null {
    const box = this.shadowRoot?.querySelector(
      `.review-text[data-index="${index}"]`) as HTMLElement | null
    return box?.shadowRoot?.querySelector('textarea') ?? null
  }

  // The Leo text box grows only at a line break, not when a long line wraps,
  // so it showed one line of a long memory at a time. This gives it the
  // height of its text, up to REVIEW_TEXT_MAX_LINES lines. Remove it when the
  // Leo text box grows with wrapped lines.
  fitReviewText_(textarea: HTMLTextAreaElement) {
    const lineHeight = parseFloat(getComputedStyle(textarea).lineHeight)
    const maxHeight = Number.isFinite(lineHeight)
      ? lineHeight * REVIEW_TEXT_MAX_LINES
      : Infinity
    textarea.style.height = 'auto'
    textarea.style.height = `${Math.min(textarea.scrollHeight, maxHeight)}px`
  }

  // Focus follows the person: into the text box when it opens, and back to the
  // button when it closes. The text box gets the height of its text first.
  focusReviewText_(index: number) {
    this.focusWhenReady_(() => {
      const textarea = this.getReviewTextArea_(index)
      if (textarea) {
        this.fitReviewText_(textarea)
      }
      return textarea
    })
  }

  // A text box that opens without the focus, for an error at the save.
  fitReviewTextWhenReady_(index: number, triesLeft = 25) {
    setTimeout(() => {
      const textarea = this.getReviewTextArea_(index)
      if (textarea) {
        this.fitReviewText_(textarea)
      } else if (triesLeft > 0) {
        this.fitReviewTextWhenReady_(index, triesLeft - 1)
      }
    }, 20)
  }

  focusReviewEditButton_(index: number) {
    this.focusWhenReady_(() => this.shadowRoot?.querySelector(
      `.review-edit-button[data-index="${index}"]`) as HTMLElement | null)
  }

  // The element is made after the page updates, and a Leo component makes its
  // inner element after that. Try for about half a second, then give up.
  focusWhenReady_(find: () => HTMLElement | null, triesLeft = 25) {
    setTimeout(() => {
      const element = find()
      if (element) {
        element.focus()
      } else if (triesLeft > 0) {
        this.focusWhenReady_(find, triesLeft - 1)
      }
    }, 20)
  }

  onReviewSave_() {
    // Check the texts of the changes that the user keeps. A text with a
    // problem shows its error at its box, and the first one gets the focus.
    let firstError = -1
    this.reviewItems_.forEach((item, index) => {
      const error = item.keep ? this.getReviewTextError_(item.text) : ''
      this.set(`reviewItems_.${index}.error`, error)
      if (error) {
        this.set(`reviewItems_.${index}.editing`, true)
        if (firstError < 0) {
          firstError = index
        } else {
          this.fitReviewTextWhenReady_(index)
        }
      }
    })
    if (firstError >= 0) {
      this.focusReviewText_(firstError)
      return
    }
    const kept: LearnedMemoryReviewDecision[] = this.reviewItems_
      .filter(item => item.keep)
      .map(item => ({
        uuid: item.proposal.uuid,
        text: item.text === item.proposal.text ? null : item.text
      }))
    this.applyReview_(kept)
  }

  onReviewDiscard_() {
    this.applyReview_([])
  }

  applyReview_(kept: LearnedMemoryReviewDecision[]) {
    const handler = this.browserProxy_.getCustomizationSettingsHandler()
    this.reviewError_ = false
    this.reviewSaving_ = true
    handler.applyDreamingReview(kept).then(
      (response: { success: boolean }) => {
        this.reviewSaving_ = false
        this.loadLearnedMemories_()
        if (response.success) {
          this.dreamResult_ = ''
          this.loadReview_()
          return
        }
        // The browser did not save. Keep what the user typed, and say so,
        // unless no review waits any more.
        handler.getDreamingReview().then(
          (result: { proposals: LearnedMemoryProposal[] }) => {
            if (result.proposals.length === 0) {
              this.reviewItems_ = []
            } else {
              this.reviewError_ = true
            }
          })
      })
  }

  hasReview_(items: ReviewItem[]): boolean {
    return items.length > 0
  }

  getChangeLabel_(item: ReviewItem): string {
    switch (item.proposal.change) {
      case LearnedMemoryChange.kNew:
        return this.i18n('braveLeoAssistantDreamReviewNew')
      case LearnedMemoryChange.kChanged:
        return this.i18n('braveLeoAssistantDreamReviewChanged')
      default:
        return this.i18n('braveLeoAssistantDreamReviewSeenAgain')
    }
  }

  // The stored text is shown when the change gives a new one.
  hasStoredText_(item: ReviewItem): boolean {
    return item.proposal.change === LearnedMemoryChange.kChanged &&
      !!item.proposal.storedText
  }

  getStoredText_(item: ReviewItem): string {
    return this.i18n('braveLeoAssistantLearnedMemoryPrevious',
                     item.proposal.storedText ?? '')
  }

  hasReviewSources_(item: ReviewItem): boolean {
    return item.quotes.length > 0
  }

  // The arguments are the fields, not only the item, so that the page asks
  // again when the user opens or closes the sentences.
  showReviewQuotes_(item: ReviewItem, open: boolean): boolean {
    return open && item.quotes.length > 0
  }

  onReviewSourcesToggle_(e: { model: { index: number } }) {
    const index = e.model.index
    this.set(`reviewItems_.${index}.sourcesOpen`,
             !this.reviewItems_[index].sourcesOpen)
  }

  // The chevron points to where the sentences go: down to open them, up to
  // close them.
  getSourcesButtonIcon_(open: boolean): string {
    return open ? 'carat-up' : 'carat-down'
  }

  // The Leo button does not pass aria-expanded to its inner button, so the
  // name of the button tells the state.
  getSourcesButtonLabel_(open: boolean): string {
    return this.i18n(open ? 'braveLeoAssistantDreamReviewHideSources'
                          : 'braveLeoAssistantDreamReviewShowSources')
  }

  // A change that the user does not keep reads quieter.
  // The arguments are the fields, not the item, so that the page asks again
  // when one of them changes.
  getReviewItemClass_(keep: boolean, editing: boolean): string {
    return ['review-item', keep ? '' : 'skipped', editing ? 'editing' : '']
      .filter(Boolean).join(' ')
  }

  hasReviewTextError_(error: ReviewTextError): boolean {
    return error !== ''
  }

  getReviewTextErrorText_(error: ReviewTextError): string {
    return error === 'tooLong'
      ? this.i18n('braveLeoAssistantInputLengthError')
      : this.i18n('braveLeoAssistantDreamReviewEmptyError')
  }

  // Nothing to save when the user keeps nothing: "Discard all" does that.
  canSaveReview_(keptCount: number, saving: boolean): boolean {
    return keptCount > 0 && !saving
  }
}

customElements.define(MemorySection.is, MemorySection)

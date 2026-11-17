// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '//resources/cr_elements/cr_lazy_render/cr_lazy_render_lit.js'
import '//resources/cr_elements/cr_action_menu/cr_action_menu.js'
import '//resources/cr_elements/cr_icon/cr_icon.js'
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js'
import '//resources/cr_elements/cr_loading_gradient/cr_loading_gradient.js'
import '//resources/cr_elements/cr_url_list_item/cr_url_list_item.js'
import './icons.html.js'

import { getInstance as getAnnouncerInstance } from '//resources/cr_elements/cr_a11y_announcer/cr_a11y_announcer.js'
import type { CrActionMenuElement } from '//resources/cr_elements/cr_action_menu/cr_action_menu.js'
import type { CrLazyRenderLitElement } from '//resources/cr_elements/cr_lazy_render/cr_lazy_render_lit.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import { assert } from '//resources/js/assert.js'
import { loadTimeData } from '//resources/js/load_time_data.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js'

import { BrowserProxyImpl } from '../browser_proxy.js'
import { getCss } from './history_embeddings.css.js'
import { getHtml } from './history_embeddings.html.js'
import { browserProxyFactory } from '../history_embeddings.mojom-webui.js'
import type {
  SearchQuery,
  SearchResult,
  SearchResultItem,
} from '../history_embeddings.mojom-webui.js'

/* Minimum time the loading state should be visible. This is to prevent the
 * loading animation from flashing. */
export const LOADING_STATE_MINIMUM_MS = 300

export interface BraveHistoryEmbeddingsElement {
  $: {
    sharedMenu: CrLazyRenderLitElement<CrActionMenuElement>
  }
}

const BraveHistoryEmbeddingsElementBase = I18nMixinLit(CrLitElement)

export class BraveHistoryEmbeddingsElement extends BraveHistoryEmbeddingsElementBase {
  static get is() {
    return 'brave-history-embeddings'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      enabled_: { type: Boolean },
      loadingResults_: { type: Boolean },
      searchResult_: { type: Object },
      searchResultDirty_: { type: Boolean },
      searchQuery: { type: String },

      webuiRoundedIconsEnabled_: { type: Boolean },
    }
  }

  private actionMenuItem_: SearchResultItem | null = null
  // Whether the profile's HistoryEmbeddingsService was built. It is built
  // once at profile setup, so this holds for the whole session.
  protected accessor enabled_: boolean =
    loadTimeData.valueExists('enableBraveHistoryEmbeddings')
      && loadTimeData.getBoolean('enableBraveHistoryEmbeddings')
  protected accessor loadingResults_ = false
  private loadingStateMinimumMs_ = LOADING_STATE_MINIMUM_MS
  protected accessor searchResult_: SearchResult | null = null
  protected accessor searchResultDirty_: boolean = false
  protected accessor webuiRoundedIconsEnabled_: boolean =
    loadTimeData.getBoolean('webuiRoundedIconsEnabled')
  private searchTimestamp_: number = 0
  accessor searchQuery: string = ''
  private searchResultChangedId_: number | null = null
  /**
   * A promise of a setTimeout for the first set of search results to come back
   * from a search. The loading state has a minimum time it needs to be on the
   * screen before showing the first set of search results, and any subsequent
   * search result for the same query is queued after it.
   */
  private searchResultPromise_: Promise<void> | null = null

  override connectedCallback() {
    super.connectedCallback()
    if (!this.enabled_) {
      return
    }
    this.searchResultChangedId_ = browserProxyFactory
      .getInstance()
      .callbackRouter.searchResultChanged.addListener(
        this.searchResultChanged_.bind(this),
      )
  }

  override disconnectedCallback() {
    super.disconnectedCallback()
    if (this.searchResultChangedId_ !== null) {
      browserProxyFactory
        .getInstance()
        .callbackRouter.removeListener(this.searchResultChangedId_)
      this.searchResultChangedId_ = null
    }
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)

    const changedPrivateProperties = changedProperties as Map<
      PropertyKey,
      unknown
    >

    if (changedProperties.has('searchQuery')) {
      this.onSearchQueryChanged_()
    }

    if (
      changedPrivateProperties.has('searchResultDirty_')
      && this.searchResultDirty_
    ) {
      this.searchResultDirty_ = false
    }
  }

  // Whether the results card shows: a search is loading or has results.
  protected showResults_(): boolean {
    if (!this.shouldSearch_()) {
      return false
    }
    return this.loadingResults_ || !!this.searchResult_?.items.length
  }

  // Whether the query gets an embeddings search: the profile's
  // HistoryEmbeddingsService was built, and the query has enough words.
  protected shouldSearch_(): boolean {
    if (!this.enabled_ || !this.searchQuery) {
      return false
    }
    return (
      this.searchQuery.split(' ').filter((part) => part.length > 0).length
      >= loadTimeData.getInteger('braveHistoryEmbeddingsSearchMinimumWordCount')
    )
  }

  protected getHeadingText_(): string {
    if (this.loadingResults_) {
      return this.i18n('braveHistoryEmbeddingsHeadingLoading', this.searchQuery)
    }

    return this.i18n('braveHistoryEmbeddingsHeading', this.searchQuery)
  }

  protected onMoreActionsClick_(e: Event) {
    e.preventDefault()
    e.stopPropagation()

    assert(this.searchResult_)
    const target = e.target as HTMLElement
    const index = Number(target.dataset['index'])
    const item = this.searchResult_.items[index]
    assert(item)
    this.actionMenuItem_ = item
    this.$.sharedMenu.get().showAt(target)
  }

  protected onMoreFromSiteClick_() {
    assert(this.actionMenuItem_)
    this.fire('change-query', {
      search: 'host:' + new URL(this.actionMenuItem_.url).hostname,
    })
    this.$.sharedMenu.get().close()
  }

  protected async onRemoveFromHistoryClick_() {
    assert(this.searchResult_)
    assert(this.actionMenuItem_)

    this.searchResult_.items.splice(
      this.searchResult_.items.indexOf(this.actionMenuItem_),
      1,
    )
    this.searchResultDirty_ = true
    await this.updateComplete
    BrowserProxyImpl.getInstance().handler.removeVisits([
      {
        url: this.actionMenuItem_.url,
        timestamps: [this.actionMenuItem_.lastUrlVisitTimestamp],
      },
    ])
    this.$.sharedMenu.get().close()
  }

  private onSearchQueryChanged_() {
    this.searchResultPromise_ = null
    if (!this.shouldSearch_()) {
      this.loadingResults_ = false
      this.searchResult_ = null
      return
    }

    this.loadingResults_ = true

    const query: SearchQuery = {
      query: this.searchQuery,
    }
    this.searchTimestamp_ = performance.now()
    browserProxyFactory.getInstance().handler.search(query)
  }

  private searchResultChanged_(result: SearchResult) {
    if (this.searchResultPromise_) {
      // If there is already a search result waiting to be processed, chain
      // this result to it so that it immediately runs after the previous
      // result was processed.
      this.searchResultPromise_ = this.searchResultPromise_.then(() =>
        this.searchResultChangedImpl_(result),
      )
    } else {
      // Artificial delay of loadingStateMinimumMs_ for UX.
      this.searchResultPromise_ = new Promise((resolve) => {
        setTimeout(
          () => {
            this.searchResultChangedImpl_(result)
            resolve()
          },
          Math.max(
            0,
            this.searchTimestamp_
              + this.loadingStateMinimumMs_
              - performance.now(),
          ),
        )
      })
    }
  }

  private searchResultChangedImpl_(result: SearchResult) {
    if (result.query !== this.searchQuery) {
      // Results are for an outdated query. Skip these results.
      return
    }

    const isNewQuery = this.searchResult_?.query !== result.query
    const hasResults = result.items.length > 0
    const hasNewResults =
      this.searchResult_?.items.length !== result.items.length
    const shouldAnnounceForResults =
      (isNewQuery && hasResults) || (!isNewQuery && hasNewResults)

    this.searchResult_ = result
    this.loadingResults_ = false

    if (shouldAnnounceForResults) {
      const resultsLabelId =
        result.items.length === 1
          ? 'braveHistoryEmbeddingsMatch'
          : 'braveHistoryEmbeddingsMatches'
      const message = loadTimeData.getStringF(
        'foundSearchResults',
        result.items.length,
        loadTimeData.getString(resultsLabelId),
        result.query,
      )
      getAnnouncerInstance().announce(message)
    }
  }

  overrideLoadingStateMinimumMsForTesting(ms: number) {
    this.loadingStateMinimumMs_ = ms
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-history-embeddings': BraveHistoryEmbeddingsElement
  }
}

customElements.define(
  BraveHistoryEmbeddingsElement.is,
  BraveHistoryEmbeddingsElement,
)

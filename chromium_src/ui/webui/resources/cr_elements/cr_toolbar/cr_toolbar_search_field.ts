// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js';

import { CrSearchFieldMixinLit } from '../cr_search_field/cr_search_field_mixin_lit.js';
import { getHtml } from './cr_toolbar_search_field.html.js'
import { getCss } from './cr_toolbar_search_field.css.js';

import type { PropertyValues } from '//resources/lit/v3_0/lit.rollup.js';

const BraveToolbarSearchFieldBase = CrSearchFieldMixinLit(CrLitElement)

export interface CrToolbarSearchFieldElement {
  $: {
    pageSearchToggle: HTMLInputElement
    searchInput: HTMLInputElement
    icon: any
  }
}

export class CrToolbarSearchFieldElement extends BraveToolbarSearchFieldBase {
  static get is() {
    return 'br-toolbar-search-field'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      narrow: { type: Boolean, reflect: true },
      showingSearch: { type: Boolean, reflect: true, notify: true },

      // When true, show a loading spinner to indicate that the backend is
      // processing the search. Will only show if the search field is open.
      spinnerActive: { type: Boolean, reflect: true },

      iconOverride: { type: String },
      searchFocused_: { type: Boolean },
      inputAriaDescription: { type: String },
    }
  }

  narrow = false;
  showingSearch = false;
  spinnerActive = false;

  searchFocused_ = false;
  isBlurring_ = false;
  iconOverride?: string
  inputAriaDescription = ''

  get isSpinnerShown() {
    return this.computeIsSpinnerShown_()
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties);

    if (changedProperties.has('showingSearch')) {
      this.showingSearchChanged_(this.showingSearch, changedProperties.get('showingSearch'))
    }
  }

  override getSearchInput() {
    return this.$.searchInput
  }

  isSearchFocused() {
    return this.searchFocused_
  }

  showAndFocus() {
    this.showingSearch = true
    this.focus_()
  }

  override onSearchTermInput() {
    super.onSearchTermInput()
    this.showingSearch = this.hasSearchText || this.isSearchFocused()
  }

  async focus_() {
    this.getSearchInput().focus()
  }

  computeIconTabIndex_(narrow: boolean) {
    return narrow ? 0 : -1
  }

  computeIconAriaHidden_(narrow: boolean) {
    return Boolean(!narrow).toString()
  }

  computeIsSpinnerShown_() {
    // TODO(petemill): Show a spinner for brave version of toolbar
    const showSpinner = this.spinnerActive && this.showingSearch
    return showSpinner
  }

  /** @private */
  onInputFocus_() {
    this.searchFocused_ = true
  }

  /** @private */
  onInputBlur_() {
    this.isBlurring_ = true
    this.searchFocused_ = false
  }

  onSearchTermKeydown_(e: KeyboardEvent) {
    if (e.key === 'Escape')
      this.showingSearch = false
  }

  showSearch_() {
    this.showingSearch = true
  }

  clearSearch_() {
    this.setValue('')
    this.focus_()
  }

  labelMouseDown_(e: MouseEvent) {
    e.preventDefault() // prevents input blur
  }

  showingSearchChanged_(current: boolean, previous?: boolean) {
    const wasBlurring = this.isBlurring_
    this.isBlurring_ = false

    // Prevent unnecessary 'search-changed' event from firing on startup.
    if (previous === undefined)
      return

    // Prevent unneccessary re-enable when blurring from input to toggle
    if (wasBlurring && !this.hasSearchText)
      return

    if (current) {
      this.focus_()
      return
    }

    this.setValue('')
    this.getSearchInput().blur()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toolbar-search-field': CrToolbarSearchFieldElement;
  }
}

customElements.define(
  CrToolbarSearchFieldElement.is, CrToolbarSearchFieldElement)

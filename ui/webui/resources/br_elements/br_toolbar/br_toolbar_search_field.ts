// Copyright 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {CrSearchFieldMixin} from 'chrome://resources/cr_elements/cr_search_field/cr_search_field_mixin.js';
import {getTemplate} from './br_toolbar_search_field.html.js'

const BraveToolbarSearchFieldBase = CrSearchFieldMixin(PolymerElement)

class BraveToolbarSearchField extends BraveToolbarSearchFieldBase {
  static get is() {
    return 'br-toolbar-search-field'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      narrow: {
	type: Boolean,
	reflectToAttribute: true,
      },

      showingSearch: {
	type: Boolean,
	value: false,
	notify: true,
	observer: 'showingSearchChanged_',
	reflectToAttribute: true
      },

      // Prompt text to display in the search field.
      label: String,

      // Tooltip to display on the clear search button.
      clearLabel: String,

      // When true, show a loading spinner to indicate that the backend is
      // processing the search. Will only show if the search field is open.
      spinnerActive: {type: Boolean, reflectToAttribute: true},

      /** @private */
      isSpinnerShown_: {
	type: Boolean,
	computed: 'computeIsSpinnerShown_(spinnerActive, showingSearch)'
      },

      /** @private */
      searchFocused_: {type: Boolean, value: false},
    }
  }

  /** @return {!HTMLInputElement} */
  getSearchInput() {
    return this.$.searchInput
  }

  /** @return {boolean} */
  isSearchFocused() {
    return this.searchFocused_
  }

  showAndFocus() {
    this.showingSearch = true
    this.focus_()
  }

  onSearchTermInput() {
    super.onSearchTermInput(this)
    this.showingSearch = this.hasSearchText || this.isSearchFocused()
  }

  /** @private */
  async focus_() {
    this.getSearchInput().focus()
  }

  /**
   * @param {boolean} narrow
   * @return {number}
   * @private
   */
  computeIconTabIndex_(narrow) {
    return narrow ? 0 : -1
  }

  /**
   * @param {boolean} narrow
   * @return {string}
   * @private
   */
  computeIconAriaHidden_(narrow) {
    return Boolean(!narrow).toString()
  }

  /**
   * @return {boolean}
   * @private
   */
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
    if (!this.hasSearchText)
      this.showingSearch = false
  }

  /** @private */
  onSearchTermKeydown_(e) {
    if (e.key == 'Escape')
      this.showingSearch = false
  }

  /**
   * @param {Event} e
   * @private
   */
  showSearch_(e) {
    this.showingSearch = true
  }

  /**
   * @param {Event} e
   * @private
   */
  clearSearch_(e) {
    this.setValue('')
    this.focus_()
  }

  showingSearchInputClicked_() {
    this.showingSearch = this.$$('.page-search_toggle').checked
  }

  labelMouseDown_(e) {
    e.preventDefault() // prevents input blur
  }

  /**
   * @param {boolean} current
   * @param {boolean|undefined} previous
   * @private
   */
  showingSearchChanged_(current, previous) {
    const wasBlurring = this.isBlurring_
    this.isBlurring_ = false

    // Prevent unnecessary 'search-changed' event from firing on startup.
    if (previous == undefined)
      return

    // Prevent unneccessary re-enable when bluring from input to toggle
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

customElements.define(
  BraveToolbarSearchField.is, BraveToolbarSearchField)

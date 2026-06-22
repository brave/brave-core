// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  RegisterPolymerComponentBehaviors,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'
import {html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

type SearchFieldWithInput = HTMLElement & {
  getSearchInput: () => HTMLInputElement
}

const BraveSubpageSearchFocusBehavior = {
  ready: function(this: HTMLElement) {
    this.addEventListener('click', (event: Event) => {
      const searchField =
        this.shadowRoot?.querySelector('cr-search-field') as SearchFieldWithInput | null
      if (!searchField || !event.composedPath().includes(searchField)) {
        return
      }

      // Keep the header's existing focus target from winning when the
      // magnifier icon is clicked.
      searchField.getSearchInput().focus()
    })
  }
}

RegisterPolymerComponentBehaviors({
  'settings-subpage': [
    BraveSubpageSearchFocusBehavior
  ]
})

RegisterStyleOverride(
  'settings-subpage', html`
  <style include="settings-shared">
    :host {
        min-height: auto !important;
        display: inline;
        position: relative;
    }
    :host(:not(.multi-card)) {
        background-color: var(--leo-color-page-background) !important;
        box-shadow: none !important;
    }
    :host(:not(.multi-card):not([id=safetyHub])) slot {
        box-shadow: var(--leo-effect-elevation-01) !important;
        background-color: var(--leo-color-container-background) !important;
        border-radius: var(--leo-radius-m) !important;
        display: block;
        padding-bottom: var(--leo-spacing-2xl);
    }
    #headerLine {
        padding-top: 0 !important;
        padding-left: var(--leo-spacing-m) !important;
        padding-right: var(--leo-spacing-m) !important;
        padding-bottom: 0 !important;
        margin-top: var(--cr-section-vertical-margin);
    }
    .cr-title-text {
        color: var(--cr-title-text-color);
        font-weight: 600 !important;
        font-size: var(--leo-typography-heading-h4-font-size) !important;
    }
  </style>`
)

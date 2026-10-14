// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import { CategoryReferenceCardElement } from '../autofill_page/category_reference_card.js'
import { SettingsAutofillPageElement } from '../autofill_page/autofill_page.js'
import { SettingsAutofillPageIndexElement } from '../autofill_page/autofill_page_index.js'

// <if expr="enable_email_aliases">
import '../email_aliases_page/email_aliases_page.js'
import { loadTimeData } from '../i18n_setup.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import type { Route } from '../router.js'
// </if>

// Used by the lit_manglers for the Email Aliases card and view.
declare module '../autofill_page/autofill_page.js' {
  interface SettingsAutofillPageElement {
    isEmailAliasesEnabled_(): boolean
    onEmailAliasesDataCategoryClick(e: Event): void
  }
}
declare module '../autofill_page/autofill_page_index.js' {
  interface SettingsAutofillPageIndexElement {
    isEmailAliasesEnabled_(): boolean
  }
}

function isEmailAliasesEnabled() {
  let enabled = false
  // <if expr="enable_email_aliases">
  enabled = loadTimeData.getBoolean('isEmailAliasesEnabled')
  // </if>
  return enabled
}
SettingsAutofillPageElement.prototype.isEmailAliasesEnabled_ =
  isEmailAliasesEnabled
SettingsAutofillPageIndexElement.prototype.isEmailAliasesEnabled_ =
  isEmailAliasesEnabled

// Each entry should act purely as a link, as it did before the "Your saved
// info" redesign, so drop the chip grid and the separator above it.
injectStyle(CategoryReferenceCardElement, css`
  hr,
  .chips-container {
    display: none;
  }
`)

// Make this page's section-header title style the same as the
// settings-section's '#header .title' style, and stack the category cards into
// a single card of plain rows, as the page looked before the "Your saved info"
// redesign. These rules are included *after* upstream's, so any property
// upstream also declares needs `!important` to win.
injectStyle(SettingsAutofillPageElement, css`
  h2.section-header
  {
    margin-top: calc(var(--cr-section-vertical-margin) - var(--leo-spacing-xl)) !important;
    font-size: var(--leo-typography-heading-h4-font-size) !important;
    font-weight: 600 !important;
    padding-top: var(--leo-spacing-xl) !important;
    padding-bottom: var(--leo-spacing-xl) !important;
    margin-bottom: 0 !important;
    letter-spacing: 0 !important;
  }

  /* Carry the card chrome here instead of on each child, so the whole
     stack reads as one card. These are the Leo tokens br/settings_section
     gives a <settings-section> #card, so this matches every other settings
     card rather than upstream's unthemed --cr-card-* values. Hiding the
     overflow keeps row hover from bleeding past the rounded corners. */
  .card-container {
    background-color: var(--leo-color-container-background);
    border-radius: var(--leo-radius-m);
    box-shadow: var(--leo-effect-elevation-01);
    margin-bottom: var(--leo-spacing-xl);
    overflow: hidden;
    gap: 0 !important;
  }

  .card-container > category-reference-card {
    background-color: transparent;
    border-radius: 0;
    box-shadow: none;
  }

  /* Same separator rule the page used before the redesign. The general
     sibling combinator keeps this correct wherever the hidden identity
     docs and travel cards sit in the order. */
  .card-container > category-reference-card:not([hidden]) ~
      :is(category-reference-card, settings-toggle-button):not([hidden]) {
    border-top: var(--cr-separator-line);
  }
`)

// <if expr="enable_email_aliases">
SettingsAutofillPageElement.prototype.onEmailAliasesDataCategoryClick = function (
  e: Event
) {
  // The card container handles `data-category-click` for every card it holds
  // and only knows about the upstream category ids, so this one must not reach
  // it.
  e.stopPropagation()
  Router.getInstance().navigateTo(routes.EMAIL_ALIASES,
    /* dynamicParams =*/ undefined, /* removeSearch =*/ true)
}

// Point the search result bubble for the Email Aliases view at its card.
const originalGetAssociatedControlFor =
  SettingsAutofillPageElement.prototype.getAssociatedControlFor
SettingsAutofillPageElement.prototype.getAssociatedControlFor = function (
  childViewId: string
): HTMLElement {
  return childViewId === 'email-aliases'
    ? this.shadowRoot.querySelector<HTMLElement>('#emailAliasesCard')!
    : originalGetAssociatedControlFor.call(this, childViewId)
}

const originalCurrentRouteChanged =
  SettingsAutofillPageIndexElement.prototype.currentRouteChanged
SettingsAutofillPageIndexElement.prototype.currentRouteChanged = function (
  newRoute: Route,
  oldRoute?: Route
) {
  originalCurrentRouteChanged.call(this, newRoute, oldRoute)
  if (newRoute === routes.EMAIL_ALIASES) {
    // Upstream switches views in a microtask, so queue ours behind it.
    queueMicrotask(() => {
      this.$.viewManager.switchView('email-aliases', 'no-animation',
                                    'no-animation')
    })
  }
}
// </if>

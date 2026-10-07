// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerPrototypeModification,
  RegisterPolymerTemplateModifications,
  RegisterStyleOverride
} from 'chrome://resources/brave/polymer_overriding.js'
import { html as polymerHtml } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { loadTimeData } from '../i18n_setup.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import type { Route } from '../router.js'

// <if expr="enable_email_aliases">
import '../email_aliases_page/email_aliases_page.js'
import { EmailAliasesStrings } from '../brave_components_webui_strings.js'
// </if>

// Make this page's section-header title style the same as the
// settings-section's '#header .title' style, and stack the category cards into
// a single card of plain rows, as the page looked before the "Your saved info"
// redesign. These rules are included *before* upstream's, so any property
// upstream also declares needs `!important` to win.
RegisterStyleOverride(
  'settings-autofill-page',
  polymerHtml`
    <style>
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
    </style>
  `
)

RegisterPolymerTemplateModifications({
  'settings-autofill-page': (templateContent) => {
    // Hide top level title and subtitle - we don't need them, each section's
    // title is enough.
    const topTitle = templateContent.querySelector('#title')
    if (!topTitle) {
        throw new Error('[Settings] Unable to find the title on autofill-page')
    }
    topTitle.hidden = true
    const topSubTitle = templateContent.querySelector('#subtitle')
    if (!topSubTitle) {
        throw new Error('[Settings] Unable to find the title on autofill-page')
    }
    topSubTitle.hidden = true

    // Hide account-card - it's used for promotion/status of google sync
    // account.
    const accountCard = templateContent.querySelector('settings-account-card')
    if (!accountCard) {
      throw new Error('[Settings] Unable to find ' +
        'settings-account-card on autofill-page')
    }
    accountCard.hidden = true

    // Hide the category cards for the data types only Autofill AI fills. They
    // are hidden rather than removed, since upstream still resolves them as the
    // control that its identity docs and travel child views are associated
    // with, which is used to anchor search result bubbles.
    for (const selector of ['#identityManagerButton', '#travelManagerButton']) {
      const card = templateContent.querySelector(selector)
      if (!card) {
        throw new Error(
          `[Settings] Unable to find ${selector} on autofill-page`)
      }
      card.hidden = true
    }

    // Hide the whole "Related services" section, which only links to services
    // Brave doesn't offer. Its rows stay in the DOM, since upstream resolves
    // #passwordManagerButton as the control its passkeys child view is
    // associated with. <settings-section> sets `display: flex` on its host,
    // which wins over the `hidden` attribute, hence the inline style.
    const relatedServicesSection =
      templateContent.querySelector('#googleAccountButton')
        ?.closest('settings-section')
    if (!relatedServicesSection) {
      throw new Error('[Settings] Unable to find the related services ' +
        'section on autofill-page')
    }
    relatedServicesSection.style.display = 'none'

    // Everything Brave adds becomes a row of the single category card.
    const cardContainer = templateContent.querySelector('.card-container')
    if (!cardContainer) {
      throw new Error(
        '[Settings] Unable to find .card-container on autofill-page')
    }

    // The only row of the "Autofill settings" section is the collapsible card
    // holding the Autofill AI settings, so hide the whole section the same way
    // as the related services one. The card stays in the DOM in case upstream
    // resolves it as an associated control.
    const autofillSettingsCard =
      templateContent.querySelector('collapsible-autofill-settings-card')
    if (!autofillSettingsCard) {
      throw new Error('[Settings] Unable to find ' +
        'collapsible-autofill-settings-card on autofill-page')
    }
    autofillSettingsCard.hidden = true
    const autofillSettingsSection =
      autofillSettingsCard.closest('settings-section')
    if (!autofillSettingsSection) {
      throw new Error('[Settings] Unable to find the autofill settings ' +
        'section on autofill-page')
    }
    autofillSettingsSection.style.display = 'none'

    // <if expr="enable_email_aliases">
    // Give Email Aliases its own category card, alongside Payment methods and
    // Contact info.
    if (loadTimeData.getBoolean('isEmailAliasesEnabled')) {
      // A null reference node appends, so this lands just before Payment
      // methods, or last if upstream ever drops that card.
      cardContainer.insertBefore(html`
        <category-reference-card
          id="emailAliasesCard"
          card-title="${loadTimeData.getString(
            EmailAliasesStrings.SETTINGS_EMAIL_ALIASES_LABEL,
          )}"
          on-data-category-click="onEmailAliasesClick">
        </category-reference-card>
      `, cardContainer.querySelector('#paymentManagerButton'))
    }
    // </if>

    // Brave supports autofill in private windows, so that toggle becomes the
    // last row of the card rather than sitting under a header of its own.
    // Appended after the Email Aliases card so it always ends up last.
    cardContainer.appendChild(html`
      <settings-toggle-button
        id="autofillPrivateWindowsToggle"
        label="${loadTimeData.getString('autofillInPrivateSettingLabel')}"
        sub-label="${loadTimeData.getString('autofillInPrivateSettingDesc')}"
        pref="{{prefs.brave.autofill_private_windows}}">
      </settings-toggle-button>
    `)
  },
  'category-reference-card': (templateContent) => {
    // Each entry should act purely as a link, as it did before the "Your saved
    // info" redesign, so drop the chip grid and the separator above it. This
    // style is appended after upstream's, so it wins without `!important`.
    templateContent.appendChild(html`
      <style>
        hr,
        .chips-container {
          display: none;
        }
      </style>
    `)
  },
  // <if expr="enable_email_aliases">
  'settings-autofill-page-index': (templateContent) => {
    if (!loadTimeData.getBoolean('isEmailAliasesEnabled')) {
      return
    }
    const viewManager = templateContent.querySelector('cr-view-manager')
    if (!viewManager) {
      throw new Error('[Settings] Unable to find cr-view-manager on ' +
        'autofill-page-index')
    }
    viewManager.append(html`
      <settings-email-aliases-page slot="view" id="email-aliases"
          prefs="{{prefs}}" data-parent-view-id="parent">
      </settings-email-aliases-page>
    `)
  }
  // </if>
})

// <if expr="enable_email_aliases">
if (loadTimeData.getBoolean('isEmailAliasesEnabled')) {
  RegisterPolymerPrototypeModification({
    'settings-autofill-page': (prototype) => {
      prototype.onEmailAliasesClick = function (e: Event) {
        // The card container handles `data-category-click` for every card it
        // holds and only knows about the upstream category ids, so this one
        // must not reach it.
        e.stopPropagation()
        Router.getInstance().navigateTo(routes.EMAIL_ALIASES,
          /* dynamicParams =*/ undefined, /* removeSearch =*/ true)
      }
      // Point the search result bubble for the Email Aliases view at its card.
      const originalGetAssociatedControlFor:
        (childViewId: string) => HTMLElement = prototype.getAssociatedControlFor
      prototype.getAssociatedControlFor =
        function (childViewId: string): HTMLElement {
          return childViewId === 'email-aliases' ?
            this.shadowRoot.querySelector('#emailAliasesCard') :
            originalGetAssociatedControlFor.call(this, childViewId)
        }
    },
    'settings-autofill-page-index': (prototype) => {
      const originalCurrentRouteChanged = prototype.currentRouteChanged
      prototype.currentRouteChanged =
        function (newRoute: Route, oldRoute?: Route) {
          originalCurrentRouteChanged.call(this, newRoute, oldRoute)
          if (newRoute === routes.EMAIL_ALIASES) {
            // Upstream switches views in a microtask, so queue ours behind it.
            queueMicrotask(() => {
              this.$.viewManager.switchView('email-aliases', 'no-animation',
                                            'no-animation')
            })
          }
        }
    }
  })
}
// </if>

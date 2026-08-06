// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  html,
  RegisterPolymerPrototypeModification,
  RegisterPolymerTemplateModifications
} from 'chrome://resources/brave/polymer_overriding.js'
import { loadTimeData } from '../i18n_setup.js'
import { YourSavedInfoDataChip } from '../metrics_browser_proxy.js'
import { routes } from '../route.js'
import { Router } from '../router.js'
import type { Route } from '../router.js'
import type { DataChip } from '../autofill_page/autofill_page.js'

// <if expr="enable_email_aliases">
import '../email_aliases_page/email_aliases_page.js'
import { EmailAliasesStrings } from '../brave_components_webui_strings.js'
// </if>

RegisterPolymerTemplateModifications({
  'settings-autofill-page': (templateContent) => {
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

    // The only row of the "Autofill settings" section is the collapsible card
    // holding the Autofill AI settings, so hide it. Brave supports autofill in
    // private windows though, so the section keeps a toggle for that instead.
    const autofillSettingsCard =
      templateContent.querySelector('collapsible-autofill-settings-card')
    if (!autofillSettingsCard) {
      throw new Error('[Settings] Unable to find ' +
        'collapsible-autofill-settings-card on autofill-page')
    }
    autofillSettingsCard.hidden = true
    // No `class="hr"`: with the Autofill AI card hidden, this is the first row
    // of the section's card and needs no separator above it.
    autofillSettingsCard.parentElement.appendChild(html`
      <settings-toggle-button
        id="autofillPrivateWindowsToggle"
        label="${loadTimeData.getString('autofillInPrivateSettingLabel')}"
        sub-label="${loadTimeData.getString('autofillInPrivateSettingDesc')}"
        pref="{{prefs.brave.autofill_private_windows}}">
      </settings-toggle-button>
    `)

    // <if expr="enable_email_aliases">
    // Give Email Aliases its own category card, alongside Payment methods and
    // Contact info. It carries no chips, since aliases have no saved-data
    // breakdown to show.
    if (loadTimeData.getBoolean('isEmailAliasesEnabled')) {
      const cardContainer = templateContent.querySelector('.card-container')
      if (!cardContainer) {
        throw new Error(
          '[Settings] Unable to find .card-container on autofill-page')
      }
      // A null reference node appends, so this lands just before Payment
      // methods, or last if upstream ever drops that card.
      cardContainer.insertBefore(html`
        <category-reference-card
          id="emailAliasesCard"
          no-chips
          card-title="${loadTimeData.getString(
            EmailAliasesStrings.SETTINGS_EMAIL_ALIASES_LABEL,
          )}"
          on-data-category-click="onEmailAliasesClick">
        </category-reference-card>
      `, cardContainer.querySelector('#paymentManagerButton'))
    }
    // </if>
  },
  // <if expr="enable_email_aliases">
  'category-reference-card': (templateContent) => {
    // A card with no chips should not show the separator and the (empty) chip
    // grid that a populated category card does.
    templateContent.appendChild(html`
      <style>
        :host([no-chips]) hr,
        :host([no-chips]) .chips-container {
          display: none;
        }
      </style>
    `)
  },
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

RegisterPolymerPrototypeModification({
  'settings-autofill-page': (prototype) => {
    // Loyalty cards are stored in Google Wallet, so drop that chip from the
    // Payment methods card. Filtering the chips as they are rendered, rather
    // than removing the chip from the data type hierarchy, keeps the chip
    // count bookkeeping upstream does for it working.
    const originalGetVisibleChips: (chips: DataChip[]) => DataChip[] =
      prototype.getVisibleChips_
    prototype.getVisibleChips_ = function (chips: DataChip[]): DataChip[] {
      return originalGetVisibleChips.call(this, chips).filter(
        (chip: DataChip) => chip.id !== YourSavedInfoDataChip.LOYALTY_CARDS)
    }
  }
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

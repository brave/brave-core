// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

const query = (root: DocumentFragment, selector: string) => {
  const element = root.querySelector<HTMLElement>(selector)
  if (!element) {
    throw new Error(`[Settings] Autofill page: couldn't find ${selector}`)
  }
  return element
}

mangle((root) => {
  // Hide the top level title and subtitle, each section's title is enough.
  query(root, '#title').setAttribute('hidden', '')
  query(root, '#subtitle').setAttribute('hidden', '')

  // The account card promotes Google sync.
  query(root, 'settings-account-card').setAttribute('hidden', '')

  // Hide the category cards for the data types only Autofill AI fills. They
  // are hidden rather than removed, since upstream still resolves them as the
  // control that its identity docs and travel child views are associated with,
  // which is used to anchor search result bubbles.
  query(root, '#identityManagerButton').setAttribute('hidden', '')
  query(root, '#travelManagerButton').setAttribute('hidden', '')

  // Hide the whole "Related services" section, which only links to services
  // Brave doesn't offer. Its rows stay in the DOM, since upstream resolves
  // #passwordManagerButton as the control its passkeys child view is associated
  // with. <settings-section> sets `display: flex` on its host, which wins over
  // the `hidden` attribute, hence the inline style.
  const relatedServicesSection =
      query(root, '#googleAccountButton').closest('settings-section')
  if (!relatedServicesSection) {
    throw new Error(
        `[Settings] Autofill page: couldn't find the related services section`)
  }
  relatedServicesSection.setAttribute('style', 'display: none')

  // The only row of the "Autofill settings" section is the collapsible card
  // holding the Autofill AI settings, so hide the whole section the same way.
  // The card stays in the DOM in case upstream resolves it as an associated
  // control.
  const autofillSettingsCard = query(root, 'collapsible-autofill-settings-card')
  autofillSettingsCard.setAttribute('hidden', '')
  const autofillSettingsSection =
      autofillSettingsCard.closest('settings-section')
  if (!autofillSettingsSection) {
    throw new Error(
        `[Settings] Autofill page: couldn't find the autofill settings section`)
  }
  autofillSettingsSection.setAttribute('style', 'display: none')

  // Everything Brave adds becomes a row of the single category card.
  // Email Aliases gets its own card, just before Payment methods. Its label is
  // only registered when the feature is enabled, which `$i18n{}` can't handle.
  query(root, '#paymentManagerButton').insertAdjacentHTML(
      'beforebegin',
      `\${this.isEmailAliasesEnabled_() ? html\`
        <category-reference-card id="emailAliasesCard"
            card-title="\${this.i18n('SETTINGS_EMAIL_ALIASES_LABEL')}"
            @data-category-click="\${this.onEmailAliasesDataCategoryClick}">
        </category-reference-card>
      \` : ''}`)

  // Brave supports autofill in private windows, so that toggle becomes the last
  // row of the card rather than sitting under a header of its own.
  query(root, '.card-container').insertAdjacentHTML(
      'beforeend',
      `<settings-toggle-button id="autofillPrivateWindowsToggle"
          label="$i18n{autofillInPrivateSettingLabel}"
          sub-label="$i18n{autofillInPrivateSettingDesc}"
          pref-key="brave.autofill_private_windows">
      </settings-toggle-button>`)
})

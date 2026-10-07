// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

/**
 * Adds a Lit `?hidden` binding to an element that's already in the template.
 * It can't go through setAttribute(): '?' isn't valid in an attribute name, so
 * the DOM rejects it, even though the parser accepts it in markup. Re-parsing
 * the element through outerHTML replaces it, so this must be the last thing
 * done to a given element.
 */
function hideUnless(element: Element, expression: string) {
  const openingTagNameEnd = element.tagName.length + 1
  const outerHtml = element.outerHTML
  // Mangling happens at build time, over the template shipped with the
  // browser, so there is no untrusted input to sanitize here.
  // eslint-disable-next-line no-unsanitized/property
  element.outerHTML = outerHtml.slice(0, openingTagNameEnd)
    + ` ?hidden="\${!(${expression})}"`
    + outerHtml.slice(openingTagNameEnd)
}

// Brave splits this dialog into two tabs -- upstream's "delete now" flow, and
// an "On exit" tab holding the clear-on-exit prefs -- and adds rows for
// clearing Brave Ads data / resetting Brave Rewards. This used to be done with
// a RegisterPolymerTemplateModifications override
// (browser/resources/settings/br/clear_browsing_data_dialog.ts) before
// settings-clear-browsing-data-dialog was migrated to Lit; that mechanism only
// works on Polymer elements, so it silently stopped applying. Everything the
// injected markup binds to comes from the companion
// chromium_src/.../clear_browsing_data_dialog.ts override.
mangle((root) => {
  const title = root.querySelector('div[slot="title"]')
  if (!title) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find the title`)
  }
  // The tabs name both halves of the dialog, so the title would only repeat
  // the first one. cr-dialog still labels itself from it, so hide it rather
  // than removing it.
  title.setAttribute('hidden', '')
  title.insertAdjacentHTML(
    'afterend',
    `<div slot="header">
       \${this.renderBraveTabs_()}
       <div id="tabsDivider"></div>
     </div>`)

  const timePicker =
    root.querySelector('settings-clear-browsing-data-time-picker')
  const timePickerHeader = timePicker?.closest('div[slot="header"]')
  if (!timePickerHeader) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find the time picker `
      + `header`)
  }

  const body = root.querySelector('div[slot="body"]')
  if (!body) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find the body`)
  }

  // Brave has no Google account data to manage.
  const manageOtherGoogleDataRow =
    root.getElementById('manageOtherGoogleDataRow')
  if (!manageOtherGoogleDataRow) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find `
      + `#manageOtherGoogleDataRow`)
  }
  manageOtherGoogleDataRow.remove()

  // The companion override folds upstream's collapsed "more" data types into
  // the expanded list, so this never has anything left to reveal.
  const showMoreButton = root.getElementById('showMoreButton')
  if (!showMoreButton) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find #showMoreButton`)
  }
  showMoreButton.remove()

  body.insertAdjacentHTML(
    'afterend',
    `<div slot="body" id="onExitBody"
         ?hidden="\${!this.isOnExitTabSelected_()}">
       <settings-brave-clear-browsing-data-on-exit-page id="onExitTab"
           @clear-data-on-exit-page-change=
               "\${this.onClearDataOnExitPageChange_}">
       </settings-brave-clear-browsing-data-on-exit-page>
     </div>`)

  const buttonContainer = root.querySelector('div[slot="button-container"]')
  if (!buttonContainer) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find the button `
      + `container`)
  }
  // Exactly one of these two rows shows, depending on whether Rewards is on:
  // clearing ads data is what's on offer until it is, and resetting Rewards
  // (which covers ads data) once it is.
  buttonContainer.insertAdjacentHTML(
    'beforebegin',
    `\${this.showBraveDataOptions_ ? html\`
       <div slot="body" id="braveDataOptions">
         <a id="clearBraveAdsData" href="chrome://settings/privacy"
             ?hidden="\${this.braveRewardsEnabled_}"
             @click="\${this.onClearBraveAdsDataClick_}">
           <span id="clearBraveAdsDataLabel">$i18n{clearBraveAdsData}</span>
         </a>
         <a id="resetBraveRewardsData" href="chrome://rewards/#reset"
             ?hidden="\${!this.braveRewardsEnabled_}">
           <leo-icon name="product-bat-outline"></leo-icon>
           <span id="resetBraveRewardsDataLabel">$i18n{resetRewardsData}</span>
           <leo-icon name="launch"></leo-icon>
         </a>
       </div>
     \` : ''}`)

  const deleteButton = root.getElementById('deleteButton')
  if (!deleteButton) {
    throw new Error(
      `[Settings] Delete browsing data dialog: couldn't find #deleteButton`)
  }
  // The "On exit" tab commits its prefs on save rather than deleting anything,
  // so it gets its own confirm button in the delete button's place.
  deleteButton.insertAdjacentHTML(
    'afterend',
    `<cr-button id="saveOnExitSettingsConfirm" class="action-button"
         ?hidden="\${!this.isOnExitTabSelected_()}"
         ?disabled="\${!this.onExitSettingsModified_}"
         @click="\${this.onSaveOnExitSettingsClick_}">
       $i18n{save}
     </cr-button>`)

  // Everything above stays with the "delete now" tab. These go last: the
  // elements are re-parsed, which invalidates the references to them.
  hideUnless(timePickerHeader, 'this.isDeleteNowTabSelected_()')
  hideUnless(body, 'this.isDeleteNowTabSelected_()')
  hideUnless(deleteButton, 'this.isDeleteNowTabSelected_()')
})

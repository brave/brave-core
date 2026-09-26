// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_tabs/cr_tabs.js'

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css, html } from '//resources/lit/v3_0/lit.rollup.js'
import type {
  PropertyValues, TemplateResult
} from '//resources/lit/v3_0/lit.rollup.js'
import { PrefService } from '/shared/settings/prefs2/pref_service.js'

import {
  SettingsClearBrowsingDataDialogElement as
      SettingsClearBrowsingDataDialogElementChromium,
  // <if expr="enable_ai_chat">
  getDataTypePrefName,
  // </if>
} from './clear_browsing_data_dialog-chromium.js'

// Registers <settings-brave-clear-browsing-data-on-exit-page>, which the
// lit_mangler override for this element's template adds as its second tab.
import '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_on_exit_page.js'

import type {
  SettingsBraveClearBrowsingDataOnExitPageElement
} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_on_exit_page.js'
import type {
  BraveClearBrowsingDataDialogBrowserProxy
} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_proxy.js'
import {
  BraveClearBrowsingDataDialogBrowserProxyImpl
} from '../brave_clear_browsing_data_dialog/brave_clear_browsing_data_dialog_proxy.js'
import { loadTimeData } from '../i18n_setup.js'

// <if expr="enable_ai_chat">
import { BrowsingDataType } from './clear_browsing_data_browser_proxy.js'
import type {
  BrowsingDataTypeOption
} from './clear_browsing_data_dialog-chromium.js'
// </if>

// Declaration-merge the Brave-only members onto the upstream class type. The
// lit_mangler-injected template needs them (it is typed with
// `this: SettingsClearBrowsingDataDialogElement` via the upstream
// clear_browsing_data_dialog.html.ts import), and declaring them here rather
// than as members of the subclass below keeps the two types interchangeable --
// upstream's own code and tests still get the upstream class from
// `document.createElement()`, via the HTMLElementTagNameMap entry in the
// unrenamed clear_browsing_data_dialog.ts. That assignment goes both ways, so
// every signature here has to match its implementation exactly -- an inferred
// return type that is narrower than the one declared here breaks it.
declare module './clear_browsing_data_dialog-chromium.js' {
  interface SettingsClearBrowsingDataDialogElement {
    braveBrowserProxy_: BraveClearBrowsingDataDialogBrowserProxy
    isClearingBraveAdsData_: boolean
    tabNames_: string[]
    selectedTabIndex_: number
    braveRewardsEnabled_: boolean
    onExitSettingsModified_: boolean
    showBraveDataOptions_: boolean
    renderBraveTabs_(): TemplateResult
    isDeleteNowTabSelected_(): boolean
    isOnExitTabSelected_(): boolean
    onSelectedTabChanged_(e: CustomEvent<{ value: number }>): void
    onClearDataOnExitPageChange_(e: Event): void
    onSaveOnExitSettingsClick_(): void
    onClearBraveAdsDataClick_(e: Event): Promise<void>
  }
}

// <if expr="enable_ai_chat">
// Leo history is listed by ALL_BROWSING_DATATYPES_LIST, but has no browsing
// data counter behind it, so it needs a static sub-label -- and it must not be
// offered at all when Leo, or its history, is unavailable. Returns |options|
// itself when there is nothing to change, so Lit sees no property change.
function withLeoAssistantOption(options: BrowsingDataTypeOption[]) {
  const prefKey = getDataTypePrefName(BrowsingDataType.BRAVE_AI_CHAT)

  for (const [index, option] of options.entries()) {
    if (option.prefKey !== prefKey) {
      continue
    }

    if (!loadTimeData.getBoolean('isLeoAssistantAllowed')
        || !loadTimeData.getBoolean('isLeoAssistantHistoryAllowed')) {
      return options.toSpliced(index, 1)
    }

    if (option.subLabel === undefined) {
      return options.toSpliced(index, 1, {
        ...option,
        subLabel: loadTimeData.getString('aiChatClearHistoryDataSubLabel'),
      })
    }

    break
  }

  return options
}
// </if>

const TAB_INDEX_DELETE_NOW = 0
const TAB_INDEX_ON_EXIT = 1

class SettingsClearBrowsingDataDialogElement extends
    SettingsClearBrowsingDataDialogElementChromium {
  static override get properties() {
    return {
      ...super.properties,
      selectedTabIndex_: { type: Number },
      braveRewardsEnabled_: { type: Boolean },
      onExitSettingsModified_: { type: Boolean },
      // Never changes, but the template reads it, and every property a
      // template reads has to be declared.
      showBraveDataOptions_: { type: Boolean },
    }
  }

  override accessor tabNames_: string[] = [
    loadTimeData.getString('clearBrowsingData'),
    loadTimeData.getString('onExitPageTitle'),
  ]
  override accessor selectedTabIndex_: number = TAB_INDEX_DELETE_NOW
  override accessor braveRewardsEnabled_: boolean = false
  override accessor onExitSettingsModified_: boolean = false

  // Brave Origin builds ship without ads and rewards, so neither of the rows
  // in #braveDataOptions has anything to point at there.
  override accessor showBraveDataOptions_: boolean =
  // <if expr="enable_brave_ads or enable_brave_rewards">
      true
  // </if>
  // <if expr="not (enable_brave_ads or enable_brave_rewards)">
      false
  // </if>

  override braveBrowserProxy_: BraveClearBrowsingDataDialogBrowserProxy =
      BraveClearBrowsingDataDialogBrowserProxyImpl.getInstance()
  override isClearingBraveAdsData_: boolean = false

  constructor() {
    super()

    // The `...super.properties` spread above makes Lit re-run
    // createProperty() for the inherited properties too, against this
    // prototype. Finding no own accessor here to wrap, Lit gives each of them
    // a disconnected one backed by fresh storage, so the values written by
    // upstream's field initializers never reach them -- they read back
    // undefined until something reassigns them. These two lists are only
    // reassigned once prefs are initialized, which happens after the first
    // render, and both willUpdate() below and upstream's template index into
    // them, so seed them here. Redeclaring them as fields instead would give
    // these protected members a new declaring class, which breaks the
    // interchangeability the declaration merge above relies on.
    this.expandedBrowsingDataTypeOptionsList_ = []
    this.moreBrowsingDataTypeOptionsList_ = []
  }

  override firstUpdated(changedProperties: PropertyValues<this>) {
    super.firstUpdated(changedProperties)

    this.addWebUiListener(
        'brave-rewards-enabled-changed', (enabled: boolean) => {
          this.braveRewardsEnabled_ = enabled
        })
    this.braveBrowserProxy_.getBraveRewardsEnabled().then((enabled) => {
      this.braveRewardsEnabled_ = enabled
    })
  }

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)

    // Brave shows every data type up front, so fold upstream's collapsed
    // "more" options back into the expanded list. Emptying the more list also
    // keeps upstream's #showMoreButton hidden, and leaves updateCounterText_()
    // looking every counter up in the expanded list.
    if (this.moreBrowsingDataTypeOptionsList_.length > 0) {
      this.expandedBrowsingDataTypeOptionsList_ = [
        ...this.expandedBrowsingDataTypeOptionsList_,
        ...this.moreBrowsingDataTypeOptionsList_,
      ]
      this.moreBrowsingDataTypeOptionsList_ = []
    }

    // <if expr="enable_ai_chat">
    this.expandedBrowsingDataTypeOptionsList_ =
        withLeoAssistantOption(this.expandedBrowsingDataTypeOptionsList_)
    // </if>
  }

  // The lit_mangler can't inject a `.tabNames` binding itself: JSDOM lowercases
  // attribute names it hasn't already seen in the file being mangled, which
  // would leave cr-tabs without its names. Rendering the element from here
  // keeps the casing (and the type check) intact.
  override renderBraveTabs_(): TemplateResult {
    return html`
      <cr-tabs id="tabs" .tabNames="${this.tabNames_}"
          .selected="${this.selectedTabIndex_}"
          @selected-changed="${this.onSelectedTabChanged_}">
      </cr-tabs>`
  }

  override isDeleteNowTabSelected_() {
    return this.selectedTabIndex_ === TAB_INDEX_DELETE_NOW
  }

  override isOnExitTabSelected_() {
    return this.selectedTabIndex_ === TAB_INDEX_ON_EXIT
  }

  override onSelectedTabChanged_(e: CustomEvent<{ value: number }>) {
    this.selectedTabIndex_ = e.detail.value
  }

  override onClearDataOnExitPageChange_(e: Event) {
    this.onExitSettingsModified_ =
        (e.target as SettingsBraveClearBrowsingDataOnExitPageElement).isModified_
  }

  override onSaveOnExitSettingsClick_() {
    const onExitPage =
        this.shadowRoot.querySelector<
            SettingsBraveClearBrowsingDataOnExitPageElement>('#onExitTab')!
    const prefService = PrefService.getInstance()
    for (const change of onExitPage.getChangedSettings()) {
      prefService.setPrefValue(change.key, change.value)
    }
    this.$.deleteBrowsingDataDialog.close()
  }

  override async onClearBraveAdsDataClick_(e: Event): Promise<void> {
    // #clearBraveAdsData is an <a> for its link styling only; clearing happens
    // here rather than by navigating.
    e.preventDefault()

    if (this.isClearingBraveAdsData_) {
      return
    }
    this.isClearingBraveAdsData_ = true

    const success = await this.braveBrowserProxy_.clearBraveAdsData()

    this.isClearingBraveAdsData_ = false

    this.fire('browsing-data-deleted', {
      deletionConfirmationText: loadTimeData.getString(
          success ? 'clearBraveAdsDataToastLabel'
                  : 'clearBraveAdsDataErrorToastLabel'),
    })

    if (this.$.deleteBrowsingDataDialog.open) {
      this.$.deleteBrowsingDataDialog.close()
    }
  }
}

injectStyle(SettingsClearBrowsingDataDialogElement, css`
  :host {
    --cr-dialog-top-container-min-height: 0px;
    /* !important needed to override Chromium's default 8px checkbox margin */
    --settings-checkbox-margin-top: 4px !important;
  }

  cr-tabs {
    --cr-tabs-height: 62px;
    --cr-tabs-selected-color: var(--leo-color-text-interactive);
    --cr-tabs-selection-bar-radius: 3px;
    --cr-tabs-selection-bar-width: 4px;
    font: var(--leo-font-large-semibold);
  }

  #tabsDivider {
    height: 1px;
    background: var(--leo-color-divider-subtle);
    margin-inline: calc(-1 * var(--leo-spacing-2xl));
  }

  #checkboxContainer {
    /* !important needed to override Chromium's base container styles */
    border-radius: var(--leo-radius-xl) !important;
    background: var(--leo-color-container-highlight) !important;
    padding: calc(var(--leo-spacing-xl) - var(--leo-spacing-m) / 2)
        var(--leo-spacing-xl) !important;
  }

  .checkbox-title {
    font: var(--leo-font-default-regular);
  }

  settings-checkbox {
    --cr-checkbox-label-color: var(--leo-color-text-primary);
  }

  settings-checkbox #subLabel {
    font: var(--leo-font-small-regular);
    color: var(--leo-color-text-tertiary);
  }

  #deleteBrowsingDataDialog::part(body-container) {
    /* !important needed to override shadow DOM part styles from Chromium */
    max-height: none !important;
  }

  #deleteBrowsingDataDialog::part(dialog) {
    /* !important needed to override shadow DOM part styles from Chromium */
    max-height: 800px !important;
    /* !important needed to override shadow DOM part styles from Chromium */
    padding: 0 !important;
    /* Increase dialog width to accommodate time picker chips */
    width: 520px;
  }

  #deleteBrowsingDataDialog [slot=header]:has(#tabs) {
    /* !important needed to override slotted content padding from Chromium */
    padding: var(--leo-spacing-m) var(--leo-spacing-2xl)
        var(--leo-spacing-xl) !important;
  }

  #deleteBrowsingDataDialog
      [slot=header]:has(settings-clear-browsing-data-time-picker) {
    /* !important needed to override slotted content padding from Chromium */
    padding: 0 var(--leo-spacing-2xl) var(--leo-spacing-xl) !important;
  }

  #deleteBrowsingDataDialog [slot=body]:not(#braveDataOptions) {
    /* !important needed to override slotted content padding from Chromium */
    padding: 0 var(--leo-spacing-2xl) !important;
  }

  #deleteBrowsingDataDialog [slot=button-container] {
    padding-inline: var(--leo-spacing-2xl);
  }

  #clearBraveAdsData,
  #resetBraveRewardsData {
    display: flex;
    padding: var(--leo-spacing-m);
    align-items: center;
    gap: var(--leo-spacing-m);
    align-self: stretch;
    text-decoration: none;
  }

  #clearBraveAdsDataLabel,
  #resetBraveRewardsDataLabel {
    font: var(--leo-font-default-regular);
    color: var(--leo-color-text-primary);
    padding: 0 var(--leo-spacing-m);
    align-self: stretch;
    flex: 1;
  }

  #clearBraveAdsData leo-icon,
  #resetBraveRewardsData leo-icon {
    --leo-icon-size: 20px;
    --leo-icon-color: var(--leo-color-icon-default);
  }

  /* Higher specificity needed to override the [slot=body] padding rule
     above. */
  #deleteBrowsingDataDialog #braveDataOptions {
    display: flex;
    margin-top: var(--leo-spacing-xl);
    margin-inline: var(--leo-spacing-2xl);
    /* !important needed to override slotted content padding from Chromium */
    padding: var(--leo-spacing-s) !important;
    flex-direction: column;
    align-items: flex-start;
    gap: var(--leo-spacing-s);
    align-self: stretch;
    border-radius: var(--leo-radius-xl);
    background: var(--leo-color-container-highlight);
  }
`)

export { SettingsClearBrowsingDataDialogElement }
export * from './clear_browsing_data_dialog-chromium.js'

// Register the Brave subclass instead of the upstream class. The matching
// `customElements.define` in upstream clear_browsing_data_dialog.ts is patched
// out.
customElements.define(
    SettingsClearBrowsingDataDialogElement.is,
    SettingsClearBrowsingDataDialogElement)

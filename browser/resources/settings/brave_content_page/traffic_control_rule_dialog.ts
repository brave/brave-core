// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js'
import 'chrome://resources/cr_elements/cr_textarea/cr_textarea.js'
import 'chrome://resources/cr_elements/cr_toggle/cr_toggle.js'
import '../controls/settings_dropdown_menu.js'

import type { CrToggleElement } from 'chrome://resources/cr_elements/cr_toggle/cr_toggle.js'
import { I18nMixinLit } from 'chrome://resources/cr_elements/i18n_mixin_lit.js'
import {
  CrLitElement,
  PropertyValues,
} from 'chrome://resources/lit/v3_0/lit.rollup.js'

import { ContainersStrings } from '../brave_generated_resources_webui_strings.js'
import type {
  DropdownMenuOptionList,
  SettingsDropdownMenuElement,
} from '../controls/settings_dropdown_menu.js'
import type { Container } from '../containers.mojom-webui.js'
import type { TrafficRule } from '../traffic_control.mojom-webui.js'

import { getCss } from './traffic_control_rule_dialog.css.js'
import { getHtml } from './traffic_control_rule_dialog.html.js'
import { TrafficControlSettingsHandlerBrowserProxy } from './traffic_control_browser_proxy.js'
import {
  cloneRuleForEdit,
  kUnsetLabel,
  ruleOperationErrorMessage,
  urlFilterPatternsOf,
} from './traffic_control_utils.js'

// Sentinel dropdown values for Target container fields that are not concrete
// container ids. Distinct from "" which means open outside any container.
enum ContainerDropdownSentinel {
  kUnset = '__unset_container__',
  kTemporary = '__temporary_container__',
}

const SettingsBraveContentTrafficControlRuleDialogElementBase =
  I18nMixinLit(CrLitElement)

/**
 * Create / edit dialog for a single traffic control rule. Owns save/error
 * handling and notifies the parent via a `close` event when dismissed.
 */
export class SettingsBraveContentTrafficControlRuleDialogElement extends SettingsBraveContentTrafficControlRuleDialogElementBase {
  static get is() {
    return 'settings-brave-content-traffic-control-rule-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      rule: { type: Object },
      containersList: { type: Array },
      draftRule_: { type: Object },
      error_: { type: String },
    }
  }

  private browserProxy = TrafficControlSettingsHandlerBrowserProxy.getInstance()

  /**
   * Initial rule from the parent (new empty rule or clone for edit). Copied
   * into `draftRule_` so parent re-renders do not wipe in-progress edits.
   */
  accessor rule: TrafficRule = {
    id: '',
    enabled: true,
    condition: { urlFilter: '' },
    target: { containerId: null, temporaryContainer: false },
  }
  accessor containersList: Container[] = []
  accessor draftRule_: TrafficRule = cloneRuleForEdit(this.rule)
  accessor error_: string | undefined

  override willUpdate(changedProperties: PropertyValues<this>) {
    super.willUpdate(changedProperties)
    if (changedProperties.has('rule')) {
      this.draftRule_ = cloneRuleForEdit(this.rule)
      this.error_ = undefined
    }
  }

  private fireClose_() {
    this.fire('rule-dialog-close')
  }

  onDialogClose_() {
    this.fireClose_()
  }

  onCancelClick_() {
    this.fireClose_()
  }

  onKeydown_(e: KeyboardEvent) {
    // Ctrl/Cmd+Enter submits the dialog (Enter alone inserts a newline).
    if (e.key === 'Enter' && (e.ctrlKey || e.metaKey)) {
      e.preventDefault()
      this.onSaveClick_()
    }
  }

  onUrlFilterValueChanged_(e: CustomEvent<{ value: string }>) {
    this.draftRule_.condition.urlFilter = e.detail.value
    this.requestUpdate()
  }

  onRuleEnabledChange_(e: Event) {
    const toggle = e.currentTarget as CrToggleElement
    this.draftRule_.enabled = toggle.checked
    this.requestUpdate()
  }

  onContainerSettingsControlChange_(e: Event) {
    const dropdown = e.target as SettingsDropdownMenuElement
    this.applyContainerDropdownValue_(dropdown.getSelectedValue())
    this.requestUpdate()
  }

  containerMenuOptions_(): DropdownMenuOptionList {
    const options: DropdownMenuOptionList = [
      {
        name: kUnsetLabel,
        value: ContainerDropdownSentinel.kUnset,
      },
      {
        name: this.i18n(ContainersStrings.CXMENU_NO_CONTAINER),
        value: '',
      },
    ]
    options.push(
      ...this.containersList.map((c) => ({
        name: c.name,
        value: c.id,
      })),
    )
    options.push({
      name: this.i18n(ContainersStrings.CXMENU_NEW_TEMPORARY_CONTAINER),
      value: ContainerDropdownSentinel.kTemporary,
    })
    return options
  }

  // Maps Target container fields to the dropdown value.
  containerDropdownValue_(): string {
    const target = this.draftRule_.target
    if (target.temporaryContainer) {
      return ContainerDropdownSentinel.kTemporary
    }
    if (target.containerId === null) {
      return ContainerDropdownSentinel.kUnset
    }
    return target.containerId
  }

  // Updates only the container-related fields on the draft target, leaving any
  // other Target fields untouched.
  private applyContainerDropdownValue_(selected: string | null) {
    const target = this.draftRule_.target
    if (selected === ContainerDropdownSentinel.kTemporary) {
      target.containerId = null
      target.temporaryContainer = true
      return
    }
    if (selected === ContainerDropdownSentinel.kUnset || selected == null) {
      target.containerId = null
      target.temporaryContainer = false
      return
    }
    target.containerId = selected
    target.temporaryContainer = false
  }

  canSave_(): boolean {
    return urlFilterPatternsOf(this.draftRule_).length > 0
  }

  async onSaveClick_() {
    if (!this.canSave_()) {
      return
    }
    const isNew = !this.draftRule_.id
    const result = isNew
      ? await this.browserProxy.handler.addRule(this.draftRule_)
      : await this.browserProxy.handler.updateRule(this.draftRule_)
    if (result.error != null) {
      this.error_ = ruleOperationErrorMessage(result.error)
      return
    }
    this.fireClose_()
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-traffic-control-rule-dialog': SettingsBraveContentTrafficControlRuleDialogElement
  }
}

customElements.define(
  SettingsBraveContentTrafficControlRuleDialogElement.is,
  SettingsBraveContentTrafficControlRuleDialogElement,
)

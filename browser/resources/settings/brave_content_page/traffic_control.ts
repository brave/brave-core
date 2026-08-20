// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_button/cr_button.js'
import '//resources/cr_elements/cr_dialog/cr_dialog.js'
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js'
import '//resources/cr_elements/cr_input/cr_input.js'
import '//resources/cr_elements/cr_toggle/cr_toggle.js'
import '//resources/brave/leo.bundle.js'
import '../controls/settings_dropdown_menu.js'
import '../controls/settings_toggle_button.js'
import '../site_favicon.js'

import type { CrToggleElement } from '//resources/cr_elements/cr_toggle/cr_toggle.js'
import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import {
  CrLitElement,
  PropertyValues,
} from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { PrefServiceObserverMixinLit } from '/shared/settings/prefs2/pref_service_observer_mixin_lit.js'

import { TrafficControlStrings } from '../brave_generated_resources_webui_strings.js'
import type {
  DropdownMenuOptionList,
  SettingsDropdownMenuElement,
} from '../controls/settings_dropdown_menu.js'
import {
  RuleOperationError,
  TrafficRule,
} from '../traffic_control.mojom-webui.js'
import { skColorToHexColor } from 'chrome://resources/js/color_utils.js'
import type { Container } from '../containers.mojom-webui.js'
import { ContainersSettingsHandlerBrowserProxy } from './containers_browser_proxy.js'

import { getCss } from './traffic_control.css.js'
import { getHtml } from './traffic_control.html.js'
import { TrafficControlSettingsHandlerBrowserProxy } from './traffic_control_browser_proxy.js'

const SettingsBraveContentTrafficControlElementBase =
  PrefServiceObserverMixinLit(I18nMixinLit(CrLitElement))

/**
 * 'settings-brave-content-traffic-control' manages profile-scoped traffic rules.
 */
export class SettingsBraveContentTrafficControlElement extends
  SettingsBraveContentTrafficControlElementBase {
  static get is() {
    return 'settings-brave-content-traffic-control'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      enabledPref_: { type: Object },
      rulesList_: { type: Array },
      containersList_: { type: Array },
      containerMenuOptions_: { type: Array },
      editingRule_: { type: Object },
      deletingRule_: { type: Object },
      editDialogError_: { type: String },
      deleteDialogError_: { type: String },
    }
  }

  private browserProxy =
    TrafficControlSettingsHandlerBrowserProxy.getInstance()
  accessor enabledPref_: chrome.settingsPrivate.PrefObject<boolean> | undefined
  accessor rulesList_: TrafficRule[] = []
  accessor containersList_: Container[] = []
  accessor containerMenuOptions_: DropdownMenuOptionList = []
  accessor editingRule_: TrafficRule | undefined
  accessor deletingRule_: TrafficRule | undefined
  accessor editDialogError_: string | undefined
  accessor deleteDialogError_: string | undefined

  override connectedCallback() {
    super.connectedCallback()
    this.mirrorPref('brave.traffic_control.enabled', 'enabledPref_')
    this.browserProxy.handler.getRules().then(({ rules }) => {
      this.onRulesUpdated_(rules)
    })
    this.loadContainers_()
    this.updateContainerMenuOptions_()
    this.browserProxy.callbackRouter.onRulesChanged.addListener(
      this.onRulesUpdated_.bind(this),
    )
  }

  private loadContainers_() {
    const containersProxy = ContainersSettingsHandlerBrowserProxy.getInstance()
    containersProxy.handler.getContainers().then(({ containers }) => {
      this.onContainersUpdated_(containers)
    })
    containersProxy.callbackRouter.onContainersChanged.addListener(
      this.onContainersUpdated_.bind(this),
    )
  }

  private onContainersUpdated_(containers: Container[]) {
    this.containersList_ = containers
    this.updateContainerMenuOptions_()
  }

  private updateContainerMenuOptions_() {
    const options: DropdownMenuOptionList = [
      {
        name: this.i18n(
          TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_NO_CONTAINER_LABEL,
        ),
        value: '',
      },
    ]
    options.push(
      ...this.containersList_.map((c) => ({
        name: c.name,
        value: c.id,
      })),
    )
    this.containerMenuOptions_ = options
  }

  override updated(changedProperties: PropertyValues<this>) {
    super.updated(changedProperties)
    if (
      changedProperties.has('enabledPref_')
      && !(this.enabledPref_?.value ?? false)
    ) {
      this.editingRule_ = undefined
      this.deletingRule_ = undefined
    }
  }

  onRulesUpdated_(rules: TrafficRule[]) {
    this.rulesList_ = rules
  }

  hasContainer_(containerId: string | null): boolean {
    return !!containerId && this.containerFor_(containerId) !== undefined
  }

  containerIcon_(containerId: string | null): number {
    return this.containerFor_(containerId)?.icon ?? 0
  }

  containerName_(containerId: string | null): string {
    if (!containerId) {
      return this.i18n(
        TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_NO_CONTAINER_LABEL,
      )
    }
    return this.containerFor_(containerId)?.name ?? containerId
  }

  containerBackgroundColor_(containerId: string | null): string {
    const container = this.containerFor_(containerId)
    return container ? skColorToHexColor(container.backgroundColor) : ''
  }

  private containerFor_(containerId: string | null) {
    if (!containerId) {
      return undefined
    }
    return this.containersList_.find((c) => c.id === containerId)
  }

  onAddRuleClick_() {
    this.editDialogError_ = undefined
    this.editingRule_ = {
      id: '',
      enabled: true,
      urlFilter: '',
      target: {
        containerId: null,
      },
    }
  }

  onEditRuleClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    const rule = this.rulesList_.find((r) => r.id === id)
    if (!rule) {
      return
    }
    this.editDialogError_ = undefined
    this.editingRule_ = {
      id: rule.id,
      enabled: rule.enabled,
      urlFilter: rule.urlFilter,
      target: {
        containerId: rule.target.containerId,
      },
    }
  }

  onDeleteRuleClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    this.deletingRule_ = this.rulesList_.find((r) => r.id === id)
    this.deleteDialogError_ = undefined
  }

  onUrlFilterValueChanged_(e: CustomEvent<{ value: string }>) {
    if (!this.editingRule_) {
      return
    }
    this.editingRule_ = { ...this.editingRule_, urlFilter: e.detail.value }
  }

  onRuleEnabledChange_(e: Event) {
    if (!this.editingRule_) {
      return
    }
    const toggle = e.currentTarget as CrToggleElement
    this.editingRule_ = { ...this.editingRule_, enabled: toggle.checked }
  }

  onContainerSettingsControlChange_(e: Event) {
    if (!this.editingRule_) {
      return
    }
    const dropdown = e.target as SettingsDropdownMenuElement
    this.editingRule_ = {
      ...this.editingRule_,
      target: {
        containerId: dropdown.getSelectedValue() || null,
      },
    }
  }

  canSaveEditingRule_(): boolean {
    return !!this.editingRule_ && !!this.editingRule_.urlFilter.trim()
  }

  onEditDialogClose_() {
    this.editingRule_ = undefined
    this.editDialogError_ = undefined
  }

  onEditDialogCancelClick_() {
    this.onEditDialogClose_()
  }

  async onEditDialogSaveClick_() {
    if (!this.editingRule_ || !this.canSaveEditingRule_()) {
      return
    }
    const isNew = !this.editingRule_.id
    const result = isNew
      ? await this.browserProxy.handler.addRule(this.editingRule_)
      : await this.browserProxy.handler.updateRule(this.editingRule_)
    if (result.error != null) {
      this.editDialogError_ = this.errorMessage_(result.error)
      return
    }
    this.editingRule_ = undefined
    this.editDialogError_ = undefined
  }

  onDeleteDialogClose_() {
    this.deletingRule_ = undefined
    this.deleteDialogError_ = undefined
  }

  onDeleteDialogCancelClick_() {
    this.onDeleteDialogClose_()
  }

  async onDeleteDialogConfirmClick_() {
    if (!this.deletingRule_) {
      return
    }
    const result =
      await this.browserProxy.handler.removeRule(this.deletingRule_.id)
    if (result.error != null) {
      this.deleteDialogError_ = this.errorMessage_(result.error)
      return
    }
    this.deletingRule_ = undefined
    this.deleteDialogError_ = undefined
  }

  private errorMessage_(error: RuleOperationError): string {
    switch (error) {
      case RuleOperationError.kInvalidUrlFilter:
        return this.i18n(
          TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_ERROR_INVALID_URL_FILTER,
        )
      case RuleOperationError.kInvalidTarget:
        return this.i18n(
          TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_ERROR_INVALID_TARGET,
        )
      case RuleOperationError.kNotFound:
        return this.i18n(
          TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_ERROR_NOT_FOUND,
        )
      default:
        return this.i18n(
          TrafficControlStrings.SETTINGS_TRAFFIC_CONTROL_ERROR_INVALID_TARGET,
        )
    }
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-traffic-control':
      SettingsBraveContentTrafficControlElement
  }
}

customElements.define(
  SettingsBraveContentTrafficControlElement.is,
  SettingsBraveContentTrafficControlElement,
)

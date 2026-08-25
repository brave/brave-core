// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/cr_elements/cr_button/cr_button.js'
import '//resources/cr_elements/cr_dialog/cr_dialog.js'
import '//resources/cr_elements/cr_icon_button/cr_icon_button.js'
import '//resources/brave/leo.bundle.js'
import '../controls/settings_toggle_button.js'
import '../site_favicon.js'
import './traffic_control_rule_dialog.js'

import { I18nMixinLit } from '//resources/cr_elements/i18n_mixin_lit.js'
import {
  CrLitElement,
  PropertyValues,
} from 'chrome://resources/lit/v3_0/lit.rollup.js'
import { PrefServiceObserverMixinLit } from '/shared/settings/prefs2/pref_service_observer_mixin_lit.js'

import { ContainersStrings } from '../brave_generated_resources_webui_strings.js'
import type { TrafficRule } from '../traffic_control.mojom-webui.js'
import { skColorToHexColor } from 'chrome://resources/js/color_utils.js'
import type { Container } from '../containers.mojom-webui.js'
import { ContainersSettingsHandlerBrowserProxy } from './containers_browser_proxy.js'

import { getCss } from './traffic_control.css.js'
import { getHtml } from './traffic_control.html.js'
import { TrafficControlSettingsHandlerBrowserProxy } from './traffic_control_browser_proxy.js'
import {
  cloneRuleForEdit,
  createEmptyRule,
  firstUrlFilterOf,
  kUnsetLabel,
  ruleOperationErrorMessage,
  urlFilterListLabel,
} from './traffic_control_utils.js'

const SettingsBraveContentTrafficControlElementBase =
  PrefServiceObserverMixinLit(I18nMixinLit(CrLitElement))

/**
 * 'settings-brave-content-traffic-control' manages profile-scoped traffic rules.
 */
export class SettingsBraveContentTrafficControlElement extends SettingsBraveContentTrafficControlElementBase {
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
      editingRule_: { type: Object },
      deletingRule_: { type: Object },
      deleteDialogError_: { type: String },
    }
  }

  private browserProxy = TrafficControlSettingsHandlerBrowserProxy.getInstance()
  accessor enabledPref_: chrome.settingsPrivate.PrefObject<boolean> | undefined
  accessor rulesList_: TrafficRule[] = []
  accessor containersList_: Container[] = []
  accessor editingRule_: TrafficRule | undefined
  accessor deletingRule_: TrafficRule | undefined
  accessor deleteDialogError_: string | undefined

  override connectedCallback() {
    super.connectedCallback()
    this.mirrorPref('brave.traffic_control.enabled', 'enabledPref_')
    this.loadRules_()
    this.loadContainers_()
  }

  private loadRules_() {
    this.browserProxy.handler.getRules().then(({ rules }) => {
      this.onRulesUpdated_(rules)
    })
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

  private onRulesUpdated_(rules: TrafficRule[]) {
    this.rulesList_ = rules
  }

  private onContainersUpdated_(containers: Container[]) {
    this.containersList_ = containers
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

  firstUrlFilterOf_(rule: TrafficRule): string {
    return firstUrlFilterOf(rule)
  }

  urlFilterListLabel_(rule: TrafficRule): string {
    return urlFilterListLabel(rule)
  }

  hasSpecificContainer_(target: TrafficRule['target']): boolean {
    return (
      !!target.containerId
      && this.containerFor_(target.containerId) !== undefined
    )
  }

  containerIcon_(containerId: string | null): number {
    return this.containerFor_(containerId)?.icon ?? 0
  }

  containerBackgroundColor_(containerId: string | null): string {
    const container = this.containerFor_(containerId)
    return container ? skColorToHexColor(container.backgroundColor) : ''
  }

  targetLabel_(target: TrafficRule['target']): string {
    if (target.temporaryContainer) {
      return this.i18n(ContainersStrings.CXMENU_NEW_TEMPORARY_CONTAINER)
    }
    if (target.containerId === null) {
      return kUnsetLabel
    }
    if (target.containerId === '') {
      return this.i18n(ContainersStrings.CXMENU_NO_CONTAINER)
    }
    // Missing / deleted containers fall back to the same unset sentinel.
    return this.containerFor_(target.containerId)?.name ?? kUnsetLabel
  }

  private containerFor_(containerId: string | null) {
    if (!containerId) {
      return undefined
    }
    return this.containersList_.find((c) => c.id === containerId)
  }

  onAddRuleClick_() {
    this.editingRule_ = createEmptyRule()
  }

  onEditRuleClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    const rule = this.rulesList_.find((r) => r.id === id)
    if (!rule) {
      return
    }
    this.editingRule_ = cloneRuleForEdit(rule)
  }

  onDeleteRuleClick_(e: Event) {
    const id = (e.currentTarget as HTMLElement).dataset['id']
    this.deletingRule_ = this.rulesList_.find((r) => r.id === id)
    this.deleteDialogError_ = undefined
  }

  onRuleDialogClose_() {
    this.editingRule_ = undefined
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
    const result = await this.browserProxy.handler.removeRule(
      this.deletingRule_.id,
    )
    if (result.error != null) {
      this.deleteDialogError_ = ruleOperationErrorMessage(result.error)
      return
    }
    this.deletingRule_ = undefined
    this.deleteDialogError_ = undefined
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'settings-brave-content-traffic-control': SettingsBraveContentTrafficControlElement
  }
}

customElements.define(
  SettingsBraveContentTrafficControlElement.is,
  SettingsBraveContentTrafficControlElement,
)

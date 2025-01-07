// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import './brave_tor_bridges_dialog.js'
import './brave_tor_snowflake_install_failed_dialog.js'

import type {CrInputElement} from 'chrome://resources/cr_elements/cr_input/cr_input.js'
import type {CrRadioGroupElement} from 'chrome://resources/cr_elements/cr_radio_group/cr_radio_group.js'
import type {SettingsToggleButtonElement} from '../controls/settings_toggle_button.js'
import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js'
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {loadTimeData} from '../i18n_setup.js'
import {RouteObserverMixin} from '../router.js'

import type {RequestBridgesDialog} from './brave_tor_bridges_dialog.js'

import {
  BraveTorBrowserProxy,
  BraveTorBrowserProxyImpl
} from './brave_tor_browser_proxy.js'

import {getTemplate} from './brave_tor_subpage.html.js'

interface SettingsBraveTorPageElement {
  $: {
    bridgesGroup: CrRadioGroupElement,
  }
}

const SettingBraveTorPageElementBase =
  I18nMixin(RouteObserverMixin(WebUiListenerMixin(PrefsMixin(PolymerElement))))

const Usage = Object.freeze({
  NOT_USED: 0,
  USE_BUILT_IN: 1,
  USE_REQUESTED: 2,
  USE_PROVIDED: 3,
})

/**
 * 'settings-brave-tor-page' is the settings page containing
 * brave's Tor settings.
 */
class SettingsBraveTorPageElement extends SettingBraveTorPageElementBase {
  static get is() {
    return 'settings-brave-tor-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      loadedConfig_: Object,

      useBridges_: {
        type: Number,
        notify: true
      },

      useBridgesValue_: {
        type: String,
        computed: 'computeUseBridgesValue_(useBridges_)',
        notify: true,
      },

      builtInBridgesTypes_: {
        type: Array,
        readOnly: true,
        value: [
          { name: "obfs4", value: 1 },
          { name: "Snowflake", value: 0 },
          { name: "meek-azure", value: 2 }
        ]
      },

      builtinBridges_: {
        type: Number,
        notify: true,
      },

      requestedBridges_: {
        type: String,
        value: '',
        notify: true,
      },

      providedBridges_: {
        type: String,
        value: '',
        notify: true
      },

      isUsingBridgesPref_: {
        type: Object,
        value() {
          return {}
        },
        notify: true,
      },

      shouldShowBridgesGroup_: {
        type: Boolean,
        value: false,
        computed: 'computeShouldShowBridgesGroup_(isUsingBridgesPref_.value, torEnabledPref_.value)'
      },

      requestedBridgesPlaceholder_: {
        type: String,
        value: '',
        computed: 'computeRequestedBridgesPlaceholder_(useBridges_, providedBridges_)'
      },

      providedBridgesPlaceholder_: {
        type: String,
        value: '',
        computed: 'computeProvidedBridgesPlaceholder_(useBridges_, providedBridges_)'
      },

      disableTorOption_: Boolean,

      torEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      },

      torSnowflakeExtensionEnabledPref_: {
        type: Object,
        value() {
          return {
            key: '',
            type: chrome.settingsPrivate.PrefType.BOOLEAN,
            value: false,
          }
        },
      },

      torSnowflakeExtensionAllowed_: {
        type: Boolean,
        value: false,
      },

      showRequestBridgesDialog_: Boolean,

      isConfigChanged_: {
        type: Boolean,
        computed: 'computeIsConfigChanged_(useBridges_, builtinBridges_, requestedBridges_, providedBridges_, loadedConfig_, shouldShowBridgesGroup_, torEnabledPref_.value)',
        value: false,
        notify: true
      },

      showTorSnowflakeInstallFailed_: Boolean,
    }
  }

  static get observers() {
    return [
      'isUsingBridgesChanged_(isUsingBridgesPref_.value)'
    ]
  }

  private loadedConfig_: any
  private useBridges_: number
  private builtinBridges_: number
  private requestedBridges_: string
  private providedBridges_: string
  private isUsingBridgesPref_: chrome.settingsPrivate.PrefObject
  private shouldShowBridgesGroup_: boolean
  private requestedBridgesPlaceholder_: string
  private disableTorOption_: boolean
  private torEnabledPref_: chrome.settingsPrivate.PrefObject
  private torSnowflakeExtensionEnabledPref_: chrome.settingsPrivate.PrefObject
  private torSnowflakeExtensionAllowed_: boolean
  private showRequestBridgesDialog_: boolean
  private isConfigChanged_: boolean
  private showTorSnowflakeInstallFailed_: boolean

  private browserProxy_: BraveTorBrowserProxy =
    BraveTorBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()

    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing.torSubpage = this.shadowRoot
    }

    this.browserProxy_.getBridgesConfig().then((config: any) => {
      this.loadedConfig_ = config
      this.isUsingBridgesPref_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: config.use_bridges !== Usage.NOT_USED,
      }
      this.useBridges_ = config.use_bridges
      this.builtinBridges_ = config.use_builtin_bridges
      this.requestedBridges_ = config.requested_bridges.join('\n')
      this.providedBridges_ = config.provided_bridges.join('\n')
    })

    // PrefControlMixin checks for a pref being valid, so have to fake it,
    // same as upstream.
    this.addWebUiListener('tor-enabled-changed', (enabled: boolean) => {
      this.setTorEnabledPref_(enabled)
    })
    this.browserProxy_.isTorEnabled().then((enabled: boolean) => {
      this.setTorEnabledPref_(enabled)
    })
    this.browserProxy_.isTorManaged().then((managed: boolean) => {
      this.disableTorOption_ = managed
    })

    if (loadTimeData.getBoolean('enable_extensions')) {
      this.browserProxy_.
        isSnowflakeExtensionAllowed().then((allowed: boolean) => {
          this.torSnowflakeExtensionAllowed_ = allowed
        })
      this.addWebUiListener('tor-snowflake-extension-enabled',
        (enabled: boolean) => {
          this.setTorSnowflakeExtensionEnabledPref_(enabled)
      })
      this.browserProxy_.
        isSnowflakeExtensionEnabled().then((enabled:boolean) => {
          this.setTorSnowflakeExtensionEnabledPref_(enabled)
      })
    }
  }

  override currentRouteChanged() {
    // This is intentional. currentRouteChanged() should be overridden.
  }

  private onSlotClick_(event: Event) {
    event.stopPropagation()
  }

  private getCurrentConfig_() {
    const bridgesConfig = {
      use_bridges: this.useBridges_,
      use_builtin_bridges: this.builtinBridges_,
      requested_bridges: this.requestedBridges_.
        split(/\r\n|\r|\n/).filter(Boolean),
      provided_bridges: this.providedBridges_.
        split(/\r\n|\r|\n/).filter(Boolean)
    }
    return bridgesConfig
  }

  private computeUseBridgesValue_() {
    switch (this.useBridges_) {
      case Usage.NOT_USED:
      default:
        return ''
      case Usage.USE_BUILT_IN:
        return 'useBuiltIn'
      case Usage.USE_REQUESTED:
        return 'useRequested'
      case Usage.USE_PROVIDED:
        return 'useProvided'
    }
  }

  private onUseBridgesValueChanged_(event: CustomEvent) {
    switch (event.detail.value) {
      case 'useBuiltIn':
        this.useBridges_ = Usage.USE_BUILT_IN
        break
      case 'useRequested':
        this.useBridges_ = Usage.USE_REQUESTED
        break
      case 'useProvided':
        this.useBridges_ = Usage.USE_PROVIDED
        break
    }
  }

  private isUsingBridgesChanged_(value: boolean) {
    if (value) {
      if (this.loadedConfig_.use_bridges !== Usage.NOT_USED) {
        this.useBridges_ = this.loadedConfig_.use_bridges
      } else {
        this.useBridges_ = Usage.USE_BUILT_IN
      }
    } else {
      this.useBridges_ = Usage.NOT_USED
    }
  }

  private onBuiltInBridgesSelect_(event: Event) {
    const target = event.target as CrInputElement
    if (target.invalid) {
      return
    }

    this.builtinBridges_ = Number(target.value)
  }

  private computeIsConfigChanged_() {
    const isObject = (object: any) => {
      return object != null && typeof object === 'object'
    }

    const matches = (object1: Object, object2: Object) => {
      const keys1 = Object.keys(object1)
      const keys2 = Object.keys(object2)
      if (keys1.length !== keys2.length) {
        return false
      }

      for (const key of keys1) {
        const val1 = object1[key as keyof Object]
        const val2 = object2[key as keyof Object]
        const areObjects = isObject(val1) && isObject(val2)
        if ((areObjects && !matches(val1, val2)) ||
          (!areObjects && val1 !== val2)) {
          return false
        }
      }
      return true
    }

    if (!this.loadedConfig_ || !this.torEnabledPref_.value) {
      return false
    }

    const currentConfig = this.getCurrentConfig_()
    if ((currentConfig.use_bridges === Usage.USE_PROVIDED &&
      currentConfig.provided_bridges.length === 0) ||
      (currentConfig.use_bridges === Usage.USE_REQUESTED &&
        currentConfig.requested_bridges.length === 0)) {
      return false
    }

    return !matches(this.getCurrentConfig_(), this.loadedConfig_)
  }

  private computeRequestedBridgesPlaceholder_() {
    if (this.useBridges_ === Usage.USE_REQUESTED) {
      return this.i18n('torRequestedBridgesPlaceholder')
    }
    return ''
  }

  private computeProvidedBridgesPlaceholder_() {
    if (this.useBridges_ === Usage.USE_PROVIDED) {
      return this.i18n('torProvidedBridgesPlaceholder')
    }
    return ''
  }

  private computeShouldShowBridgesGroup_() {
    return this.isUsingBridgesPref_.value && this.torEnabledPref_.value
  }

  private builtInTypeEqual_(item: any, selection: any) {
    return item === selection
  }

  private usageEqual_(usage: any, current: any) {
    return usage === current
  }

  private setTorEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.torEnabledPref_ = pref
  }

  private onTorEnabledChange_(event: Event) {
    event.stopPropagation()
    this.browserProxy_.
      setTorEnabled((event.target as SettingsToggleButtonElement).checked)
  }

  private onUseBridgesChange_(event: Event) {
    event.stopPropagation()
    setTimeout(() => {
      this.$.bridgesGroup.scrollIntoView()
    })
  }

  private setBridgesConfig_(event: Event) {
    event.stopPropagation()
    this.loadedConfig_ = this.getCurrentConfig_()
    this.browserProxy_.setBridgesConfig(this.loadedConfig_)
  }

  private requestBridges_() {
    this.showRequestBridgesDialog_ = true
  }

  private showRequestBridgesDialogClosed_(event: Event) {
    this.showRequestBridgesDialog_ = false
    const currentTarget = event.currentTarget as RequestBridgesDialog
    if (currentTarget.bridges_) {
      this.requestedBridges_ = currentTarget.bridges_.join('\n')
    }
  }

  private setTorSnowflakeExtensionEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.torSnowflakeExtensionEnabledPref_ = pref
  }

  private onTorSnowflakeExtensionChange_(event: Event) {
    event.stopPropagation()
    const enabled = (event.target as SettingsToggleButtonElement).checked
    this.browserProxy_.enableSnowflakeExtension(enabled).
      catch((reason: String) => {
        console.log(reason)
        this.setTorSnowflakeExtensionEnabledPref_(false)
        this.showTorSnowflakeInstallFailed_ = true
      })
  }

  private torSnowflakeInstallFailedDialogClosed_() {
    this.showTorSnowflakeInstallFailed_ = false
  }
}

customElements.define(
  SettingsBraveTorPageElement.is, SettingsBraveTorPageElement)

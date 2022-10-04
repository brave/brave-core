// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUIListenerMixin } from 'chrome://resources/js/web_ui_listener_mixin.js'
import { I18nMixin } from 'chrome://resources/js/i18n_mixin.js';
import { RouteObserverMixin } from '../router.js'
import { PrefsMixin } from '../prefs/prefs_mixin.js'
import { BraveTorBrowserProxyImpl } from './brave_tor_browser_proxy.js'
import './brave_tor_bridges_dialog.js'
import {getTemplate} from './brave_tor_subpage.html.js'

const SettingBraveTorPageElementBase = I18nMixin(RouteObserverMixin(WebUIListenerMixin(PrefsMixin(PolymerElement))))

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

      showRequestBridgesDialog_: Boolean,

      isConfigChanged_: {
        type: Boolean,
        computed: 'computeIsConfigChanged_(useBridges_, builtinBridges_, requestedBridges_, providedBridges_, loadedConfig_, shouldShowBridgesGroup_, torEnabledPref_.value)',
        value: false,
        notify: true
      }
    }
  }

  static get observers() {
    return [
      'isUsingBridgesChanged_(isUsingBridgesPref_.value)'
    ]
  }

  browserProxy_ = BraveTorBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.browserProxy_.getBridgesConfig().then((config) => {
      this.loadedConfig_ = config
      this.isUsingBridgesPref_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: config.use_bridges != Usage.NOT_USED,
      }
      this.useBridges_ = config.use_bridges
      this.builtinBridges_ = config.use_builtin_bridges
      this.requestedBridges_ = config.requested_bridges.join('\n')
      this.providedBridges_ = config.provided_bridges.join('\n')
    })

    // PrefControlMixin checks for a pref being valid, so have to fake it,
    // same as upstream.
    this.addWebUIListener('tor-enabled-changed', enabled => {
      this.setTorEnabledPref_(enabled)
    })
    this.browserProxy_.isTorEnabled().then(enabled => {
      this.setTorEnabledPref_(enabled)
    })
    this.browserProxy_.isTorManaged().then(managed => {
      this.disableTorOption_ = managed
    })
  }

  onSlotClick_(e) {
    e.stopPropagation()
  }

  getCurrentConfig_() {
    const bridgesConfig = {
      use_bridges: this.useBridges_,
      use_builtin_bridges: Number.parseInt(this.builtinBridges_, 10),
      requested_bridges: this.requestedBridges_.split(/\r\n|\r|\n/).filter(Boolean),
      provided_bridges: this.providedBridges_.split(/\r\n|\r|\n/).filter(Boolean),
    }
    return bridgesConfig
  }

  computeUseBridgesValue_() {
    switch (this.useBridges_) {
      case Usage.NOT_USED:
        return ''
      case Usage.USE_BUILT_IN:
        return 'useBuiltIn'
      case Usage.USE_REQUESTED:
        return 'useRequested'
      case Usage.USE_PROVIDED:
        return 'useProvided'
    }
  }

  onUseBridgesValueChanged_(event) {
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

  isUsingBridgesChanged_(value) {
    if (value) {
      if (this.loadedConfig_.use_bridges != Usage.NOT_USED) {
        this.useBridges_ = this.loadedConfig_.use_bridges
      } else {
        this.useBridges_ = Usage.USE_BUILT_IN
      }
    } else {
      this.useBridges_ = Usage.NOT_USED
    }
  }

  onBuiltInBridgesSelect_(event) {
    this.builtinBridges_ = Number(event.target.value)
  }

  computeIsConfigChanged_() {
    const isObject = (object) => {
      return object != null && typeof object === 'object'
    }

    const matches = (object1, object2) => {
      const keys1 = Object.keys(object1)
      const keys2 = Object.keys(object2)
      if (keys1.length !== keys2.length) {
        return false
      }

      for (const key of keys1) {
        const val1 = object1[key]
        const val2 = object2[key]
        const areObjects = isObject(val1) && isObject(val2)
        if ((areObjects && !matches(val1, val2)) ||
          (!areObjects && val1 !== val2)) {
          return false
        }
      }
      return true
    }

    if (!this.loadedConfig_ || !this.torEnabledPref_.value)
      return false

    const currentConfig = this.getCurrentConfig_()
    if ((currentConfig.use_bridges == Usage.USE_PROVIDED &&
      currentConfig.provided_bridges.length == 0) ||
      (currentConfig.use_bridges == Usage.USE_REQUESTED &&
        currentConfig.requested_bridges.length == 0)) {
      return false
    }

    return !matches(this.getCurrentConfig_(), this.loadedConfig_)
  }

  computeRequestedBridgesPlaceholder_() {
    if (this.useBridges_ == Usage.USE_REQUESTED) {
      return this.i18n('torRequestedBridgesPlaceholder')
    }
    return ''
  }

  computeProvidedBridgesPlaceholder_() {
    if (this.useBridges_ == Usage.USE_PROVIDED) {
      return this.i18n('torProvidedBridgesPlaceholder')
    }
    return ''
  }

  computeShouldShowBridgesGroup_() {
    return this.isUsingBridgesPref_.value && this.torEnabledPref_.value
  }

  builtInTypeEqual_(item, selection) {
    return item === selection
  }

  usageEqual_(usage, current) {
    return usage === current
  }

  setTorEnabledPref_(enabled) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.torEnabledPref_ = pref
  }

  onTorEnabledChange_(e) {
    e.stopPropagation()
    this.browserProxy_.setTorEnabled(e.target.checked)
  }

  onUseBridgesChange_(e) {
    e.stopPropagation()
    setTimeout(() => {
      this.$.bridgesGroup.scrollIntoView()
    })
  }

  setBridgesConfig_(e) {
    e.stopPropagation()
    this.loadedConfig_ = this.getCurrentConfig_()
    this.browserProxy_.setBridgesConfig(this.loadedConfig_)
  }

  requestBridges_() {
    this.showRequestBridgesDialog_ = true
  }

  showRequestBridgesDialogClosed_(event) {
    this.showRequestBridgesDialog_ = false
    if (event.currentTarget.bridges_) {
      this.requestedBridges_ = event.currentTarget.bridges_.join('\n')
    }
  }

  currentRouteChanged() {
    // This is intentional. currentRouteChanged() should be overridden.
  }
}

customElements.define(
  SettingsBraveTorPageElement.is, SettingsBraveTorPageElement)

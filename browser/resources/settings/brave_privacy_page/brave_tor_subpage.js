// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { html, PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { WebUIListenerMixin } from 'chrome://resources/js/web_ui_listener_mixin.js';
import { RouteObserverMixin } from '../router.js';
import { PrefsMixin } from '../prefs/prefs_mixin.js';
import { BraveTorBrowserProxyImpl } from './brave_tor_browser_proxy.js'
import './brave_tor_bridges_dialog.js'

const SettingBraveTorPageElementBase = RouteObserverMixin(WebUIListenerMixin(PrefsMixin(PolymerElement)))

/**
 * 'settings-brave-tor-page' is the settings page containing
 * brave's Tor settings.
 */
class SettingsBraveTorPageElement extends SettingBraveTorPageElementBase {
  static get is() {
    return 'settings-brave-tor-subpage'
  }

  static get template() {
    return html`{__html_template__}`
  }

  static get properties() {
    return {
      loadedConfig_: Object,
      
      useBridges_: Number,

      builtInBridgesTypes_: {
        type: Array,
        value: [{ name: "snowflake", value: 0 },
        { name: "obfs4", value: 1 },
        { name: "meek-azure", value: 2 }]
      },

      builtinBridges_: Number,
      requestedBridges_: Array,
      providedBridges_: Array,

      isUsingBridges_: {
        type: Boolean,
        notify: true,
        value: false
      },

      showRequestBridgesDialog_: Boolean,
    }
  }

  browserProxy_ = BraveTorBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.browserProxy_.getBridgesConfig().then((config) => {
      this.loadedConfig_ = config
      this.UpdateUseBridges_(config.use_bridges)
      this.builtinBridges_ = config.use_builtin_bridges
      this.requestedBridges_ = config.requested_bridges
      this.providedBridges_ = config.bridges

      this.$.builtInBridgesType.value = this.builtinBridges_

      switch (this.useBridges_) {
        case 0:
        case 1:
          this.$.brigdesGroup.selected = 'useBuiltIn'
          break
        case 2:
          this.$.brigdesGroup.selected = 'useRequestedBridges'
          break
        case 3:
          this.$.brigdesGroup.selected = 'useProvidedBridges'
          break
      }

      this.$.requestedBridges.value = this.requestedBridges_.join('\n')
      this.$.providedBridges.value = this.providedBridges_.join('\n')
    })

    this.$.useBuiltIn.expanded = true
    this.$.useRequestedBridges.expanded = true
    this.$.useProvidedBridges.expanded = true

    this.$.providedBridges.oninput = (event) => {
      this.providedBridges_ = this.onProvidedBridgesChange_(event)
      this.handleChanged_()
    }
  }

  getCurrentConfig_() {
    const bridgesConfig = {
      use_bridges: this.useBridges_,
      use_builtin_bridges: Number.parseInt(this.builtinBridges_, 10),
      requested_bridges: this.requestedBridges_,
      bridges: this.providedBridges_,
    }
    return bridgesConfig
  }

  UpdateUseBridges_(value) {
    this.useBridges_ = value
    this.isUsingBridges_ = value != 0
  }

  onUseBridgesChange_() {
    if (!this.$.useBridges.checked) {
      this.UpdateUseBridges_(0)
    } else {
      this.onUsageSelect_()
    }
    this.handleChanged_()
  }

  onUsageSelect_() {
    switch (this.$.brigdesGroup.selected) {
      case 'useBuiltIn':
        this.UpdateUseBridges_(1)
        this.OnBuiltInBridgesSelect_()
        break
      case 'useRequestedBridges':
        this.UpdateUseBridges_(2)
        break
      case 'useProvidedBridges':
        this.UpdateUseBridges_(3)
        break
    }
    this.handleChanged_()
  }

  OnBuiltInBridgesSelect_() {
    this.builtinBridges_ = this.$.builtInBridgesType.value
    this.handleChanged_()
  }

  onProvidedBridgesChange_(event) {
    return event.target.value.split(/\r\n|\r|\n/);
  }

  handleChanged_() {
    const matches = (obj, source) =>
      Object.keys(source).every(key => obj.hasOwnProperty(key) && obj[key] === source[key])

    const equals = matches(this.getCurrentConfig_(), this.loadedConfig_)
    this.$.apply.hidden = equals
  }

  setBridgesConfig_() {
    this.loadedConfig_ = this.getCurrentConfig_()
    this.$.apply.hidden = true

    this.browserProxy_.setBridgesConfig(this.loadedConfig_)
  }

  requestBridges_() {
    this.showRequestBridgesDialog_ = true
  }

  showRequestBridgesDialogClosed_(event) {
    this.showRequestBridgesDialog_ = false
    this.requestedBridges_ = event.currentTarget.bridges_
    this.$.requestedBridges.value = this.requestedBridges_.join('\n')
    this.handleChanged_()
  }
}

customElements.define(
  SettingsBraveTorPageElement.is, SettingsBraveTorPageElement)

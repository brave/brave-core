// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import {BraveWalletBrowserProxyImpl} from './brave_wallet_browser_proxy.m.js';

Polymer({
  is: 'add-wallet-network-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    networks: {
      type: Array,
      value() {
        return [];
      },
    },

    rpcUrls: {
      type: Array,
      value() {
        return [{value: ''}];
      }
    },
    iconUrls: {
      type: Array,
      value() {
        return [{value: ''}];
      }
    },
    blockUrls: {
      type: Array,
      value() {
        return [{value: ''}];
      }
    },
    isRpcPlusButtonDisabled_: {
      type: Boolean,
      value: true,
    },
    isIconPlusButtonDisabled_: {
      type: Boolean,
      value: true,
    },
    isBlockPlusButtonDisabled_: {
      type: Boolean,
      value: true,
    },
    isSubmitButtonEnabled_: {
      type: Boolean,
      value: false,
    },

    chainIdValue_: Number,
    invalidChainIdMessage_: String,
    chainIdInvalid_: {
      type: Boolean,
      value: false,
    },

    chainNameValue_: String,
    chainNameInvalid_: {
      type: Boolean,
      value: false,
    },
    isCurrencyErrorHidden_: {
      type: Boolean,
      value: true,
    },
    isSubmissionErrorHidden_: {
      type: Boolean,
      value: true,
    },
    submissionErrorMessage_: {
      type: String,
      value: true
    },
    selected: {
      type: Object,
      value: {}
    },
    currencyNameValue_: String,
    currencySymbolValue_: String,
    currencyDecimalsValue_: Number
  },
  
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveWalletBrowserProxyImpl.getInstance();
  },

  ready: function() {
    if (Object.keys(this.selected).length === 0)
      return
    this.chainIdValue_ = parseInt(this.selected.chainId, 16) | 0
    this.chainNameValue_ = this.selected.chainName
    this.currencyNameValue_ = this.selected.nativeCurrency.name
    this.currencySymbolValue_ = this.selected.nativeCurrency.symbol
    const decimals = this.selected.nativeCurrency.decimals
    if (decimals) {
      this.currencyDecimalsValue_ = decimals
    }
    this.rpcUrls = this.selected.rpcUrls.map(element => { return {value: element}})
    if (this.selected.iconUrls.length)
      this.iconUrls = this.selected.iconUrls.map(element => { return {value: element}})
    if (this.selected.blockExplorerUrls.length)
      this.blockUrls = this.selected.blockExplorerUrls.map(element => { return {value: element}})
    this.isSubmitButtonEnabled_ = true
    this.updatePlusButtonState('rpc')
    this.updatePlusButtonState('icon')
    this.updatePlusButtonState('block')
  },

  validateURL: function(value) {
    const url_ = value
    if (url_.trim() == '') {
      return false;
    }
    let url;
    try {
      url = new URL(url_.trim());
    } catch (e) {
      return false;
    }

    return url.protocol === "http:" || url.protocol === "https:"
  },
  /** @private */
  chainIdChanged_: function(event) {
    const value = event.target.value
    this.chainIdInvalid_ = value <= 0
    if (this.chainIdInvalid_) {
      this.invalidChainIdMessage_ = this.i18n('walletAddNetworkInvalidChainId')
    }
    this.updateSubmitButtonState_()
  },

  chainNameChanged_: function(event) {
    const element = event.target
    this.chainNameInvalid_ = element.value.trim() === ''
    this.updateSubmitButtonState_()
  },

  isInvalidInputForList_: function(value, list) {
    if (value.trim() === '') {
      return (list === 'icon' || list === 'block') ? false : true
    }
    return !this.validateURL(value)
  },
  getTailOfUrlsArray: function(list) {
    if (list === 'rpc') {
      return this.rpcUrls.at(0).value
    } else if (list === 'icon') {
      return this.iconUrls.at(0).value
    } else if (list === 'block') {
      return this.blockUrls.at(0).value
    }
  },
  updatePlusButtonState: function(list) {
    const inputElementValue = this.getTailOfUrlsArray(list)
    const disabled = (!inputElementValue || inputElementValue.trim() === '')
    if (list === 'rpc') {
      this.isRpcPlusButtonDisabled_ = disabled
    }  else if (list === 'icon') {
      this.isIconPlusButtonDisabled_ = disabled
    } else if (list === 'block') {
      this.isBlockPlusButtonDisabled_ = disabled
    }
  },
  updateSubmitButtonState_: function() {
    for (const input of this.shadowRoot.querySelectorAll('.mandatory')) {
      if (input && (input.invalid || !input.value || input.value.trim() === '')) {
        this.isSubmitButtonEnabled_ = false
        return;
      }
    }
    for (const input of this.shadowRoot.querySelectorAll('.wallet-input')) {
      if (input && input.invalid) {
        this.isSubmitButtonEnabled_ = false
        return;
      }
    }
    if (this.chainIdValue_ <= 0) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    if (this.chainNameValue_ === '') {
      this.isSubmitButtonEnabled_ = false
      return;
    }
    
    if (!this.rpcUrls.find(element => element.value !== '')) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    this.isSubmitButtonEnabled_ = true
  },
  nativeCurrencyChanged_: function(event) {
    this.isCurrencyErrorHidden_ = true
  },
  // Called for any change in the urls inputs
  // validates value of focused one and shows error if the value is invalid
  // calls update for Plus buton state
  urlChangedImpl_: function(element, list) {
    element.invalid = this.isInvalidInputForList_(element.value, list)
    this.updateSubmitButtonState_()
    const empty = element.value.trim() === ''
    if (list == 'rpc' && element.invalid) {
      const text = empty ? this.i18n('walletAddNetworkMandarotyFieldError')
                         : this.i18n('walletAddNetworkInvalidURLInput')
      element.setAttribute('error-message', text)
    }
    if (!element.invalid || empty) {
      this.updatePlusButtonState(list)
    }
  },
  urlChangedIcons_: function(event) {
    return this.urlChangedImpl_(event.target, 'icon')
  },
  urlChangedRpc_: function(event) {
    return this.urlChangedImpl_(event.target, 'rpc')
  },
  urlChangedBlock_: function(event) {
    return this.urlChangedImpl_(event.target, 'block')
  },
  onAddRpcUrlTap_: function(item) {
    this.splice('rpcUrls', 0, 0, {value: ''});
    this.isRpcPlusButtonDisabled_ = true
  },
  onAddIconUrlTap_: function(item) {
    this.splice('iconUrls', 0, 0, {value: ''});
    this.isIconPlusButtonDisabled_ = true
  },
  onAddBlockUrlTap_: function(item) {
    this.splice('blockUrls', 0, 0, {value: ''});
    this.isBlockPlusButtonDisabled_ = true
  },
  transformListForSerializaion_: function(list) {
    return list.reduce((filtered, item) => {
      if (item && item.value.trim() !== '') {
        if (!filtered) {
         filtered = []
        }
        filtered.push(item.value)
      }
      return filtered
    }, null)
  },
  showCurrencyError: function() {
    this.isSubmissionErrorHidden_ = true
    this.isCurrencyErrorHidden_ = false
  },
  setSubmissionResult: function(success, errorMessage) {
    this.isCurrencyErrorHidden_ = this.isSubmissionErrorHidden_ = true
    if (!success) {
      this.isSubmissionErrorHidden_ = false
      this.submissionErrorMessage_ = errorMessage
    }
  },
  addNewNetwork: function(payload) {
    this.browserProxy_.addEthereumChain(JSON.stringify(payload))
      .then(([success, errorMessage]) => {
        this.setSubmissionResult(success, errorMessage)
        if (success) {
          this.fire('close');
          return
        }
      })
  },
  getHexNumber: function(value) {
    return '0x' + Number(this.chainIdValue_).toString(16)
  },
  onAddNetworkTap_: function(item) {
    let payload = Object({
      chainId: this.getHexNumber(),
      chainName: this.chainNameValue_,
    })
    const nativeCurrency = Object({
      name: this.currencyNameValue_.trim(),
      symbol: this.currencySymbolValue_.trim(),
      decimals: parseInt(this.currencyDecimalsValue_)
    })
    if ((nativeCurrency.name || nativeCurrency.symbol || nativeCurrency.decimals)) {
      if (!nativeCurrency.name || !nativeCurrency.symbol || !nativeCurrency.decimals) {
        this.showCurrencyError()
        return;
      }
      payload.nativeCurrency = nativeCurrency;
    }
    const rpcUrls = this.transformListForSerializaion_(this.rpcUrls)
    if (rpcUrls) {
      payload.rpcUrls = rpcUrls
    }
    const blockExplorerUrls = this.transformListForSerializaion_(this.blockUrls)
    if (blockExplorerUrls) {
      payload.blockExplorerUrls = blockExplorerUrls
    }
    const iconUrls = this.transformListForSerializaion_(this.iconUrls)
    if (iconUrls) {
      payload.iconUrls = iconUrls
    }
    if (this.networks.find(element => { return element.chainId === payload.chainId })) {
      if (!window.confirm(this.i18n('walletAddNetworkDialogReplaceNetwork'))) {
        return
      }
      this.browserProxy_.removeEthereumChain(payload.chainId).then((success) => {
        if (!success)
          return;
        this.addNewNetwork(payload)
      })
      return
    }
    this.addNewNetwork(payload)
  }
});

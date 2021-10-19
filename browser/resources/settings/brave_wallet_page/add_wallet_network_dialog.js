// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.m.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.m.js';
import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {I18nBehavior} from 'chrome://resources/js/i18n_behavior.m.js';
import { BraveWalletBrowserProxyImpl } from './brave_wallet_browser_proxy.m.js';

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

    isSubmitButtonEnabled_: {
      type: Boolean,
      value: false,
    },
    selected: {
      type: Object,
      value: {}
    }
  },
  
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = BraveWalletBrowserProxyImpl.getInstance();
  },

  ready: function() {
    if (Object.keys(this.selected).length === 0)
      return
    this.$$('#chainId').value = this.selected.chainId
    this.$$('#chainName').value = this.selected.chainName
    this.$$('#currencyName').value = this.selected.nativeCurrency.name
    this.$$('#currencySymbol').value = this.selected.nativeCurrency.symbol
    const decimals = this.selected.nativeCurrency.decimals
    if (decimals) {
      this.$$('#currencyDecimals').value = decimals
    }
    this.rpcUrls = this.selected.rpcUrls.map(element => { return {value: element}})
    if (this.selected.iconUrls.length)
      this.iconUrls = this.selected.iconUrls.map(element => { return {value: element}})
    if (this.selected.blockExplorerUrls.length)
      this.blockUrls = this.selected.blockExplorerUrls.map(element => { return {value: element}})
    this.isSubmitButtonEnabled_ = true
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
  isValidHexValue: function(value) {
    var parsed = parseInt(value, 16);
    const processed = value.replace('0x', '').toLowerCase()
    return (parsed.toString(16) === processed) && value.startsWith('0x');
  },
  /** @private */
  chainIdChanged_: function(event) {
    let element = (this.$$('#chainId'))
    const value = element.value
    element.invalid = !this.isValidHexValue(value)
    const empty = value.trim() === ''
    if (element.invalid) {
      const text = empty ? this.i18n('walletAddNetworkMandarotyFieldError')
                         : this.i18n('walletAddNetworkInvalidChainId')
      element.setAttribute('error-message', text)
    }
    this.updateSubmitButtonState_()
  },

  chainNameChanged_: function(event) {
    let element = (this.$$('#chainName'))
    element.invalid = element.value.trim() === ''
    this.updateSubmitButtonState_()
  },

  isInvalidInputForList_: function(value, list) {
    if (value.trim() === '') {
      return (list === 'icon' || list === 'block') ? false : true
    }
    return !this.validateURL(value)
  },
  
  nameChangedImpl_: function(value, list) {
    const elementId = list + '-urls-list'
    const selector = '#' + elementId + ' > cr-input:focus-within'
    let element = (this.$$(selector));
    element.invalid = this.isInvalidInputForList_(value, list)
    this.updateSubmitButtonState_()
    const empty = value.trim() === ''
    if (list == 'rpc' && element.invalid) {
      const text = empty ? this.i18n('walletAddNetworkMandarotyFieldError')
                         : this.i18n('walletAddNetworkInvalidURLInput')
      element.setAttribute('error-message', text)
    }
    if (!element.invalid || empty) {
      this.updatePlusButtonState_(list);
    }
  },

  updatePlusButtonState_: function(list) {
    let buttonElement = (this.$$('#' + list + '-plus-button'));
    const elementId = list + '-urls-list'
    const elementSelector = '#' + elementId + ' > cr-input'
    let inputElement = (this.$$(elementSelector));
    if (!inputElement.value || inputElement.value.trim() === '') {
      buttonElement.disabled = true
    } else  {
      buttonElement.disabled = inputElement.invalid
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
    let chainIdElement = (this.$$('#chainId'))
    if (chainIdElement.value === '') {
      this.isSubmitButtonEnabled_ = false
      return;
    }
    
    let chainNameElement = (this.$$('#chainName'))
    if (chainNameElement.value === '') {
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
    this.shadowRoot.querySelector('#currency-error').hidden = true
  },
  nameChangedIcons_: function(event) {
    return this.nameChangedImpl_(event.model.item.value, 'icon')
  },
  nameChangedRpc_: function(event) {
    return this.nameChangedImpl_(event.model.item.value, 'rpc')
  },
  nameChangedBlock_: function(event) {
    return this.nameChangedImpl_(event.model.item.value, 'block')
  },
  onAddRpcUrlTap_: function(item) {
    this.splice('rpcUrls', 0, 0, {value: ''});
    let buttonElement = (this.$$('#rpc-plus-button'));
    buttonElement.disabled = true
  },
  onAddIconUrlTap_: function(item) {
    this.splice('iconUrls', 0, 0, {value: ''});
    let buttonElement = (this.$$('#icon-plus-button'));
    buttonElement.disabled = true
  },
  onAddBlockUrlTap_: function(item) {
    this.splice('blockUrls', 0, 0, {value: ''});
    let buttonElement = (this.$$('#block-plus-button'));
    buttonElement.disabled = true
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
  setSubmissionResult: function(success, errorId) {
    this.shadowRoot.querySelector('#currency-error').hidden = this.shadowRoot.querySelector('#submission-error').hidden = true;
    if (!success) {
      this.shadowRoot.querySelector(errorId).hidden = false
    }
  },
  addNewNetwork: function(payload) {
    this.browserProxy_.addEthereumChain(JSON.stringify(payload))
      .then(success => {
        this.setSubmissionResult(success, '#submission-error')
        if (success) {
          this.fire('close');
          return
        }
      })
  },
  onAddNetworkTap_: function(item) {
    let payload = Object({
      chainId: this.$$('#chainId').value,
      chainName: this.$$('#chainName').value,
    })
    const nativeCurrency = Object({
      name: this.$$('#currencyName').value.trim(),
      symbol: this.$$('#currencySymbol').value.trim(),
      decimals: parseInt(this.$$('#currencyDecimals').value)
    })
    if ((nativeCurrency.name || nativeCurrency.symbol || nativeCurrency.decimals)) {
      if (!nativeCurrency.name || !nativeCurrency.symbol || !nativeCurrency.decimals) {
        this.setSubmissionResult(false, '#currency-error')
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

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_button/cr_button.js';
import 'chrome://resources/cr_elements/cr_dialog/cr_dialog.js';
import 'chrome://resources/cr_elements/cr_input/cr_input.js';
import 'chrome://resources/cr_elements/cr_searchable_drop_down/cr_searchable_drop_down.js';
import { Polymer, html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import { I18nBehavior } from 'chrome://resources/cr_elements/i18n_behavior.js';
import { BraveWalletBrowserProxyImpl } from './brave_wallet_browser_proxy.m.js';

Polymer({
  is: 'add-wallet-network-dialog',

  _template: html`{__html_template__}`,

  behaviors: [
    I18nBehavior
  ],

  properties: {
    addNewAllowed: {
      type: Boolean,
      value: false
    },
    coin: {
      type: Number,
      value: 0
    },
    networks: {
      type: Array,
      value() {
        return [];
      },
    },
    selectedRpcUrl: {
      type: String,
      value: ''
    },
    rpcUrls: {
      type: Array,
      value() {
        return [{ value: '' }];
      }
    },
    iconUrls: {
      type: Array,
      value() {
        return [{ value: '' }];
      }
    },
    blockUrls: {
      type: Array,
      value() {
        return [{ value: '' }];
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

    chainIdValue_: String,
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
    currencyDecimalsValue_: Number,
    prepopulatedNetworks_: {
      type: Array,
      value: []
    },
    searchValue_: {
      type: String,
      value: '',
      observer: 'onSearchValueChanged_',
    },
    searchItems_: {
      type: Array,
      value: []
    },
  },

  browserProxy_: null,

  /** @override */
  created: function () {
    this.browserProxy_ = BraveWalletBrowserProxyImpl.getInstance();
  },

  ready: function () {
    this.updatePrepopulatedNetworks()

    if (Object.keys(this.selected).length === 0)
      return
    this.chainIdValue_ = this.selected.chainId
    this.chainNameValue_ = this.selected.chainName
    this.currencyNameValue_ = this.selected.nativeCurrency.name
    this.currencySymbolValue_ = this.selected.nativeCurrency.symbol
    const decimals = this.selected.nativeCurrency.decimals
    if (decimals) {
      this.currencyDecimalsValue_ = decimals
    }
    this.rpcUrls = this.selected.rpcUrls.map(value => { return { value } })
    this.selectedRpcUrl = this.selected.rpcUrls[this.selected.activeRpcEndpointIndex] || this.selected.rpcUrls[0]
    if (this.selected.iconUrls.length)
      this.iconUrls = this.selected.iconUrls.map(value => { return { value } })
    if (this.selected.blockExplorerUrls.length)
      this.blockUrls = this.selected.blockExplorerUrls.map(value => { return { value } })
    this.isSubmitButtonEnabled_ = true
    this.updatePlusButtonState('rpc')
    this.updatePlusButtonState('icon')
    this.updatePlusButtonState('block')
  },

  connectedCallback: function () {
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing['addWalletNetworkDialog'] = this.shadowRoot
    }
  },

  disconnectedCallback: function () {
    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      delete window.testing['addWalletNetworkDialog']
    }
  },

  updatePrepopulatedNetworks() {
    if (!this.addNewAllowed)
      return
    this.browserProxy_.getPrepopulatedNetworksList().then(payload => {
      if (!payload)
        return

      this.prepopulatedNetworks_ = payload
      this.prepopulatedNetworks_.forEach(item => item.searchString = `${item.chainId}(${BigInt(item.chainId)}) ${item.chainName}`)
      this.searchItems_ = this.prepopulatedNetworks_.map(item => item.searchString)
    })
  },

  validateURL: function (value) {
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
  chainIdChanged_: function (event) {
    const value = event.target.value
    this.chainIdInvalid_ = value.length === 0
    if (this.chainIdInvalid_) {
      this.invalidChainIdMessage_ = this.i18n('walletAddNetworkInvalidChainId')
    }
    this.updateSubmitButtonState_()
  },

  chainNameChanged_: function (event) {
    const element = event.target
    this.chainNameInvalid_ = element.value.trim() === ''
    this.updateSubmitButtonState_()
  },

  isInvalidInputForList_: function (value, list) {
    if (value.trim() === '') {
      return (list === 'icon' || list === 'block') ? false : true
    }
    return !this.validateURL(value)
  },
  getTailOfUrlsArray: function (list) {
    if (list === 'rpc') {
      return this.rpcUrls.at(0).value
    } else if (list === 'icon') {
      return this.iconUrls.at(0).value
    } else if (list === 'block') {
      return this.blockUrls.at(0).value
    }
  },
  updatePlusButtonState: function (list) {
    const inputElementValue = this.getTailOfUrlsArray(list)
    const disabled = (!inputElementValue || inputElementValue.trim() === '')
    if (list === 'rpc') {
      this.isRpcPlusButtonDisabled_ = disabled
    } else if (list === 'icon') {
      this.isIconPlusButtonDisabled_ = disabled
    } else if (list === 'block') {
      this.isBlockPlusButtonDisabled_ = disabled
    }
  },
  updateSubmitButtonState_: function () {
    for (const input of this.shadowRoot.querySelectorAll('.mandatory')) {
      if (input && (input.invalid || !input.value || (input.value.trim && input.value.trim() === ''))) {
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
    if (this.chainIdValue_.length === 0) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    if (this.chainNameValue_ === '') {
      this.isSubmitButtonEnabled_ = false
      return;
    }
    if (!this.hasValidRPCUrls()) {
      this.isSubmitButtonEnabled_ = false
      return;
    }

    this.isSubmitButtonEnabled_ = true
  },
  nativeCurrencyChanged_: function (event) {
    this.isCurrencyErrorHidden_ = true
  },
  // Called for any change in the urls inputs
  // validates value of focused one and shows error if the value is invalid
  // calls update for Plus buton state
  urlChangedImpl_: function (element, list) {
    element.invalid = this.isInvalidInputForList_(element.value, list)
    this.updateSubmitButtonState_()
    const empty = element.value.trim() === ''
    if (list == 'rpc' && element.invalid) {
      const text = empty && !this.hasValidRPCUrls() ? this.i18n('walletAddNetworkMandarotyFieldError')
        : this.i18n('walletAddNetworkInvalidURLInput')
      element.setAttribute('error-message', text)
    }
    if (!element.invalid || empty) {
      this.updatePlusButtonState(list)
    }
  },
  hasValidRPCUrls: function () {
    return this.rpcUrls.find(element => this.validateURL(element.value))
  },
  urlChangedIcons_: function (event) {
    return this.urlChangedImpl_(event.target, 'icon')
  },
  urlChangedRpc_: function (event) {
    return this.urlChangedImpl_(event.target, 'rpc')
  },
  urlChangedBlock_: function (event) {
    return this.urlChangedImpl_(event.target, 'block')
  },
  onAddRpcUrlTap_: function (item) {
    this.splice('rpcUrls', 0, 0, { value: '' });
    this.isRpcPlusButtonDisabled_ = true
  },
  onAddIconUrlTap_: function (item) {
    this.splice('iconUrls', 0, 0, { value: '' });
    this.isIconPlusButtonDisabled_ = true
  },
  onAddBlockUrlTap_: function (item) {
    this.splice('blockUrls', 0, 0, { value: '' });
    this.isBlockPlusButtonDisabled_ = true
  },
  transformListForSerializaion_: function (list) {
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
  showCurrencyError: function () {
    this.isSubmissionErrorHidden_ = true
    this.isCurrencyErrorHidden_ = false
  },
  setSubmissionResult: function (success, errorMessage) {
    this.isCurrencyErrorHidden_ = this.isSubmissionErrorHidden_ = true
    if (!success) {
      this.isSubmissionErrorHidden_ = false
      this.submissionErrorMessage_ = errorMessage
    }
  },
  addNewNetwork: function (payload) {
    this.browserProxy_.addChain(payload)
      .then(([success, errorMessage]) => {
        this.setSubmissionResult(success, errorMessage)
        if (success) {
          this.fire('close');
          return
        }
      })
  },
  getHexNumber: function (value) {
    if (!isNaN(Number(value)))
      return '0x' + Number(value).toString(16)
    return value
  },
  onAddNetworkTap_: function (item) {
    let payload = Object({
      chainId: this.getHexNumber(this.chainIdValue_),
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
    payload.coin = this.coin
    payload.rpcUrls = this.transformListForSerializaion_(this.rpcUrls)
    payload.activeRpcEndpointIndex = payload.rpcUrls.findIndex(it => it === this.selectedRpcUrl)
    if (payload.activeRpcEndpointIndex < 0)
      payload.activeRpcEndpointIndex = 0
    payload.blockExplorerUrls = this.transformListForSerializaion_(this.blockUrls)
    payload.iconUrls = this.transformListForSerializaion_(this.iconUrls)
    if (this.networks.find(element => { return element.chainId === payload.chainId })) {
      if (!window.confirm(this.i18n('walletAddNetworkDialogReplaceNetwork'))) {
        return
      }
      this.browserProxy_.removeChain(payload.chainId, this.coin).then((success) => {
        if (!success)
          return;
        this.addNewNetwork(payload)
      })
      return
    }
    this.addNewNetwork(payload)
  },
  onSearchValueChanged_(newValue, oldValue) {
    if (!newValue)
      return
    const found = this.prepopulatedNetworks_.find(item => item.searchString === newValue)
    if (found) {
      this.chainIdValue_ = found.chainId
      this.invalidChainIdMessage_ = ''
      this.chainIdInvalid_ = false
      this.chainNameValue_ = found.chainName
      this.chainNameInvalid_ = false
      this.currencyNameValue_ = found.nativeCurrency.name
      this.currencySymbolValue_ = found.nativeCurrency.symbol
      const decimals = found.nativeCurrency.decimals
      if (decimals) {
        this.currencyDecimalsValue_ = decimals
      }
      this.rpcUrls = found.rpcUrls.map(value => { return { value } })
      this.selectedRpcUrl = found.rpcUrls[found.activeRpcEndpointIndex] || found.rpcUrls[0]
      if (found.iconUrls.length)
        this.iconUrls = found.iconUrls.map(value => { return { value } })
      if (found.blockExplorerUrls.length)
        this.blockUrls = found.blockExplorerUrls.map(value => { return { value } })
      this.isSubmitButtonEnabled_ = true
      this.updatePlusButtonState('rpc')
      this.updatePlusButtonState('icon')
      this.updatePlusButtonState('block')

      this.searchValue_ = ''
    }
  }
});

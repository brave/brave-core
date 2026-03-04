// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($, $Object) {

if (window.isSecureContext) {
  function post(method, payload) {
    let postMessage = $(function(message) {
      return $.postNativeMessage('$<message_handler>', message);
    });

    return new Promise($((resolve, reject) => {
      postMessage({
        "securityToken": SECURITY_TOKEN,
        "method": method,
        "args": JSON.stringify(payload)
      })
      .then(resolve, (errorJSON) => {
        /* remove `Error: ` prefix. errorJSON=`Error: {code: 1, errorMessage: "Internal error"}` */
        const errorJSONString = new String(errorJSON);
        const errorJSONStringSliced = errorJSONString.slice(errorJSONString.indexOf('{'));
        try {
          reject(JSON.parse(errorJSONStringSliced))
        } catch(e) {
          reject(errorJSON)
        }
      })
    }));
  }
  
  // CIP-30 API object factory - creates the API returned by enable()
  function createCardanoApi() {
    const api = {};
    $Object.defineProperties(api, {
      getNetworkId: { value: $(function() { return post('getNetworkId', {}); }), writable: false, enumerable: true },
      getUtxos: { value: $(function(amount, paginate) { return post('getUtxos', { amount, paginate }); }), writable: false, enumerable: true },
      getBalance: { value: $(function() { return post('getBalance', {}); }), writable: false, enumerable: true },
      getUsedAddresses: { value: $(function() { return post('getUsedAddresses', {}); }), writable: false, enumerable: true },
      getUnusedAddresses: { value: $(function() { return post('getUnusedAddresses', {}); }), writable: false, enumerable: true },
      getChangeAddress: { value: $(function() { return post('getChangeAddress', {}); }), writable: false, enumerable: true },
      getRewardAddresses: { value: $(function() { return post('getRewardAddresses', {}); }), writable: false, enumerable: true },
      signTx: { value: $(function(tx, partialSign) { return post('signTx', { tx, partialSign }); }), writable: false, enumerable: true },
      signData: { value: $(function(addr, payload) { return post('signData', { addr, payload }); }), writable: false, enumerable: true },
      submitTx: { value: $(function(tx) { return post('submitTx', { tx }); }), writable: false, enumerable: true },
      getCollateral: { value: $(function(amount) { return post('getCollateral', { amount }); }), writable: false, enumerable: true },
    });
    return $Object.freeze(api);
  }

  // CIP-30 Cardano Provider object
  const cardanoProvider = {};
  $Object.defineProperties(cardanoProvider, {
    name: {
      value: "Brave",
      writable: false,
      enumerable: true,
    },
    apiVersion: {
      value: "1",
      writable: false,
      enumerable: true,
    },
    supportedExtensions: {
      value: [],
      writable: false,
      enumerable: true,
    },
    icon: {
      value: "data:image/png;base64,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",
      writable: false,
      enumerable: true,
    },
    isEnabled: {
      value: $(function() {
        return post('isEnabled', {});
      }),
      writable: false,
      enumerable: true,
    },
    enable: {
      value: $(function() {
        return post('enable', {}).then(function() {
          return createCardanoApi();
        });
      }),
      writable: false,
      enumerable: true,
    },
  });

  // Freeze the provider object to prevent tampering
  $Object.freeze(cardanoProvider);

  // Create window.cardano if it doesn't exist
  if (!window.cardano) {
    const cardanoNamespace = {value: {}};
    $Object.defineProperty(window, 'cardano', cardanoNamespace);
  }

  // Inject Brave Wallet provider at window.cardano.brave
  const braveProvider = {
    value: cardanoProvider,
    enumerable: true,
    writable: false,
    configurable: false,
  };
  $Object.defineProperty(window.cardano, 'brave', braveProvider);
}

});

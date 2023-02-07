// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

window.__firefox__.execute(function($) {
  
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
        try {
          reject(JSON.parse(errorJSON))
        } catch(e) {
          reject(errorJSON)
        }
      })
    }));
  }
  
  Object.defineProperty(window, 'ethereum', {
    value: {
      chainId: undefined,
      networkVersion: undefined,
      selectedAddress: undefined,
      isMetaMask: true,
      isBraveWallet: true,
      request: $(function (args) /* -> Promise<unknown> */  {
        return post('request', args)
      }),
      isConnected: $(function() /* -> bool */ {
        return true;
      }),
      enable: $(function() /* -> void */ {
        return post('enable', {})
      }),
      // ethereum.sendAsync(payload: JsonRpcRequest, callback: JsonRpcCallback): void;
      sendAsync: $(function(payload, callback) {
        post('sendAsync', payload)
          .then((response) => {
            callback(null, response)
          })
          .catch((response) => {
            callback(response, null)
          })
      }),
      /*
      Available overloads for send:
        ethereum.send(payload: JsonRpcRequest, callback: JsonRpcCallback): void;
        ethereum.send(method: string, params?: Array<unknown>): Promise<JsonRpcResponse>;
      */
      send: $(function(
        methodOrPayload /* : string or JsonRpcRequest */,
        paramsOrCallback /*  : Array<unknown> or JsonRpcCallback */
      ) {
        var payload = {
          method: '',
          params: {}
        }
        if (typeof methodOrPayload === 'string') {
          payload.method = methodOrPayload
          payload.params = paramsOrCallback
          return post('send', payload)
        } else {
          payload.params = methodOrPayload
          if (paramsOrCallback != undefined) {
            post('send', payload)
              .then((response) => {
                paramsOrCallback(null, response)
              })
              .catch((response) => {
                paramsOrCallback(response, null)
              })
          } else {
            // Unsupported usage of send
            throw TypeError('Insufficient number of arguments.')
          }
        }
      }),
      isUnlocked: $(function() /* -> Promise<boolean> */ {
        return post('isUnlocked', {})
      }),
    }
  });
}
  
});

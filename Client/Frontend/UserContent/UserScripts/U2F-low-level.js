// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

try {
    var sameOrigin = (window.top.location.origin == window.location.origin)
    if (!sameOrigin) {
        window.u2f = undefined
        return
    }
} catch (e) {
    window.u2f = undefined
    return
}

Object.defineProperty(window.u2f, 'receiveChannel', {
  value: new MessageChannel()
})

Object.defineProperty(window.u2f, 'sendChannel', {
  value: new MessageChannel()
})

Object.defineProperty($<u2f-internal>, 'low_level_id', {
  value: 1
})

Object.defineProperty(window, 'js_api_version', {
  value: 1
})

// Some implementations of low level legacy APIs use these constants
// we define the constants here to maintain consistency
Object.defineProperty($<u2f-internal>, 'ErrorCodes', {
  value: {
    'U2F_REGISTER_REQUEST': 'u2f_register_request',
    'U2F_SIGN_REQUEST': 'u2f_sign_request',
    'U2F_REGISTER_RESPONSE': 'u2f_register_response',
    'U2F_SIGN_RESPONSE': 'u2f_sign_response'
  }
})

Object.defineProperty($<u2f-internal>, 'MessageTypes', {
  value: {
    'OK': 0,
    'OTHER_ERROR': 1,
    'BAD_REQUEST': 2,
    'CONFIGURATION_UNSUPPORTED': 3,
    'DEVICE_INELIGIBLE': 4,
    'TIMEOUT': 5
  }
})

Object.defineProperty($<u2f-internal>, 'postLowLevelRegister', {
  value: function (requestId, fromNative, version, registerationData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f-internal>.caller[handle]
      caller.$<u2f-internal>.postLowLevelRegister(requestId, false, version, registerationData, clientData, errorCode, errorMessage);
      return;
    }
    var response = {}
    var registerResponse = {}

    if (errorCode > 1) {
      registerResponse = {
        'errorCode': errorCode,
        'errorMessage': errorMessage
      }
    } else {
      registerResponse = new RegisterResponse(version, registerationData, clientData)
    }

    response = {
      type: $<u2f-internal>.MessageTypes.U2F_REGISTER_RESPONSE,
      requestId: requestId,
      responseData: registerResponse
    }
    window.u2f.sendChannel.port1.postMessage(response)
  }
})

Object.defineProperty($<u2f-internal>, 'postLowLevelSign', {
  value: function (requestId, fromNative, keyHandle, signatureData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f-internal>.caller[handle]
      caller.window.$<u2f-internal>.postLowLevelSign(handle, false, version, registerationData, clientData, errorCode, errorMessage);
      return;
    }

    var response = {}
    var registerResponse = {}

    if (errorCode > 1) {
      signResponse = {
        'errorCode': errorCode,
        'errorMessage': errorMessage
      }
    } else {
      signResponse = new SignResponse(keyHandle, signatureData, clientData)
    }

    response = {
      type: $<u2f-internal>.MessageTypes.U2F_SIGN_RESPONSE,
      requestId: requestId,
      responseData: signResponse
    }
    window.u2f.sendChannel.port1.postMessage(response)
  }
})

Object.defineProperty($<u2f-internal>, 'getPortSingleton_', {
  value: function (callback) {
    window.u2f.sendChannel.port1.onmessage = u2f.responseHandler_
    window.u2f.sendChannel.port2.onmessage = u2f.responseHandler_
    window.top.$<u2f-internal>.caller[handle] = window

    callback(window.u2f.receiveChannel.port1)
  }
})

window.u2f.receiveChannel.port2.onmessage = function (e) {
  const handle = $<u2f-internal>.low_level_id++
  webkit.messageHandlers.$<handler>.postMessage({ name: 'fido-low-level', handle: handle, data: JSON.stringify(e.data) })
}

Object.defineProperty(window.u2f.receiveChannel.port2.onmessage, 'toString', {
    value: function () {
        return defaultU2FString
    }
})

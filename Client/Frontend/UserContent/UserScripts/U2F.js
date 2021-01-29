// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// The $<> is used to replace the var name at runtime with a random string.

// FIDO2 - WebAuthn
Object.defineProperty(window, '$<webauthn>', {
  value: {}
})

Object.defineProperty(window, '$<webauthn-internal>', {
  value: {}
})

// Default string for toString outputs
const defaultU2FString = "function () { [native code] }";

// We use the define property method to avoid the properties from being changed
// by default configurable, enumerable and writable properties of the object
// are false
Object.defineProperty($<webauthn-internal>, 'id', {
  value: 0
})

// Used to handle calls from iframes
Object.defineProperty($<webauthn-internal>, 'caller', {
  value: []
 })

Object.defineProperty($<webauthn-internal>, 'reject', {
  value: []
})

Object.defineProperty($<webauthn-internal>, 'resolve', {
  value: []
})

Object.defineProperty($<webauthn-internal>, 'data', {
  value: {}
})

// FIDO - High Level API
Object.defineProperty(window, '$<u2f>', {
  value: {}
})

Object.defineProperty(window, '$<u2f-internal>', {
  value: {}
})

Object.defineProperty($<u2f-internal>, 'id', {
  value: 0
})

Object.defineProperty($<u2f-internal>, 'resolve', {
  value: []
})

Object.defineProperty($<u2f-internal>, 'caller', {
  value: []
})

Object.defineProperty(window, 'base64ToArrayBuffer', {
  value: function (base64) {
    var binary_string = window.atob(base64)
    var len = binary_string.length
    var bytes = new Uint8Array(len)
    for (var i = 0; i < len; i++) {
      bytes[i] = binary_string.charCodeAt(i)
    }
    return bytes.buffer
  }
})

Object.defineProperty(window, 'webSafe64', {
  value: function (base64) {
    return base64.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
  }
})

class $<pkc> {
  constructor (id, response) {
    this.rawId = window.base64ToArrayBuffer(id)
    this.id = window.webSafe64(id)
    this.response = response
    this.clientExtensionResults = {}
    this.type = "public-key"
  }

  getClientExtensionResults() {
    return this.clientExtensionResults;
  }
}

class $<attest> {
  constructor (attestationObject, clientDataJSON) {
    this.attestationObject = window.base64ToArrayBuffer(attestationObject)
    this.clientDataJSON = window.base64ToArrayBuffer(clientDataJSON)
  }
}

class $<assert> {
  constructor (authenticatorData, clientDataJSON, signature, userHandle) {
    this.authenticatorData = window.base64ToArrayBuffer(authenticatorData)
    this.clientDataJSON = window.base64ToArrayBuffer(clientDataJSON)
    this.signature = window.base64ToArrayBuffer(signature)
    this.userHandle = window.base64ToArrayBuffer(userHandle)
  }
}

class registerResponse {
  constructor (version, registrationData, clientData) {
    this.version = version
    this.registrationData = registrationData
    this.clientData = clientData
  }
}

Object.defineProperty(registerResponse, 'toString', {
    value: function () {
        return ""
    }
})

class signResponse {
  constructor (keyHandle, signatureData, clientData) {
    this.keyHandle = keyHandle
    this.signatureData = signatureData
    this.clientData = clientData
  }
}

Object.defineProperty(signResponse, 'toString', {
    value: function () {
        return ""
    }
})

Object.defineProperty($<webauthn-internal>, 'postCreate', {
  value:
    function (handle, fromNative, id, attestationObject, clientDataJSON, errorName, errorDescription) {
      if (fromNative) {
        caller = window.top.$<webauthn-internal>.caller[handle]
        caller.$<webauthn-internal>.postCreate(handle, false, id, attestationObject, clientDataJSON, errorName, errorDescription);
        return;
      }
      if (errorName) {
        $<webauthn-internal>.reject[handle](new DOMException(atob(errorDescription), atob(errorName)))
        return
      }
      response = new $<attest>(attestationObject, clientDataJSON)
      data = new $<pkc>(id, response)
      $<webauthn-internal>.resolve[handle](data)
    }
})

Object.defineProperty($<webauthn-internal>, 'postGet', {
  value: function (handle, fromNative, id, authenticatorData, clientDataJSON, signature, userHandle, errorName, errorDescription) {
    if (fromNative) {
      caller = window.top.$<webauthn-internal>.caller[handle]
      caller.$<webauthn-internal>.postGet(handle, false, id, authenticatorData, clientDataJSON, signature, userHandle, errorName, errorDescription);
      return;
    }
    if (errorName) {
      $<webauthn-internal>.reject[handle](new DOMException(atob(errorDescription), atob(errorName)))
      return
    }
    response = new $<assert>(authenticatorData, clientDataJSON, signature, userHandle)
    data = new $<pkc>(id, response)
    $<webauthn-internal>.resolve[handle](data)
  }
})

Object.defineProperty($<u2f-internal>, 'postSign', {
  value: function (handle, fromNative, keyHandle, signatureData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f-internal>.caller[handle]
      caller.$<u2f-internal>.postSign(handle, false, keyHandle, signatureData, clientData, errorCode, errorMessage);
      return;
    }
    if (errorCode > 1) {
      errorData = {
        'errorCode': errorCode,
        'errorMessage': atob(errorMessage)
      }
      $<u2f-internal>.resolve[handle](errorData)
      return
    }
    response = new signResponse(keyHandle, signatureData, clientData)
    $<u2f-internal>.resolve[handle](response)
  }
})

Object.defineProperty($<u2f-internal>, 'postRegister', {
  value: function (handle, fromNative, version, registerationData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f-internal>.caller[handle]
      caller.$<u2f-internal>.postRegister(handle, false, version, registerationData, clientData, errorCode, errorMessage);
      return;
    }
    if (errorCode > 1) {
      errorData = {
        'errorCode': errorCode,
        'errorMessage': atob(errorMessage)
      }
      $<u2f-internal>.resolve[handle](errorData)
      return
    }
    response = new registerResponse(version, registerationData, clientData)
    $<u2f-internal>.resolve[handle](response)
  }
})

Object.defineProperty(window, 'stringifyArrayCleaner', {
  value: function (k, v) {
    // in some cases v instanceof ArrayBuffer is false
    if (v && v.constructor.name ===  "ArrayBuffer") {
      return btoa(String.fromCharCode.apply(null, new Uint8Array(v)))
    } else if (v instanceof Uint8Array) {
      return btoa(String.fromCharCode.apply(null, v))
    }
    return v
  }
})

Object.defineProperty($<webauthn>, 'create', {
  value: function (args) {
    const cleanedArgs = JSON.stringify(args, stringifyArrayCleaner)
    const handle = $<webauthn-internal>.id++
    return new Promise(
      function (resolve, reject) {
        $<webauthn-internal>.reject[handle] = reject
        $<webauthn-internal>.resolve[handle] = resolve
        window.top.$<webauthn-internal>.caller[handle] = window
        webkit.messageHandlers.$<handler>.postMessage({ name: 'fido2-create', data: cleanedArgs, handle: handle })
      }
    )
  }
})

Object.defineProperty($<webauthn>.create, 'toString', {
    value: function () {
        return defaultU2FString
    }
})

Object.defineProperty($<webauthn>, 'get', {
  value: function (args) {
    const cleanedArgs = JSON.stringify(args, stringifyArrayCleaner)
    const handle = $<webauthn-internal>.id++
    return new Promise(
      function (resolve, reject) {
        const handle = $<webauthn-internal>.id++
        $<webauthn-internal>.reject[handle] = reject
        $<webauthn-internal>.resolve[handle] = resolve
        window.top.$<webauthn-internal>.caller[handle] = window
        webkit.messageHandlers.$<handler>.postMessage({ name: 'fido2-get', data: cleanedArgs, handle: handle })
      }
    )
  }
})

Object.defineProperty($<webauthn>.get, 'toString', {
    value: function () {
        return defaultU2FString
    }
})

Object.defineProperty($<u2f>, 'sign', {
  value: function (appId, challenge, registeredKeys, callback) {
    if (typeof callback != "function") {
      return
    }
    return new Promise(
      function (resolve, reject) {
        const handle = $<u2f-internal>.id++
        $<u2f-internal>.resolve[handle] = callback
        window.top.$<u2f-internal>.caller[handle] = window
        webkit.messageHandlers.$<handler>.postMessage({ name: 'fido-sign', appId: appId, challenge: challenge, keys: JSON.stringify(registeredKeys), handle: handle })
      }
    )
  }
})

Object.defineProperty($<u2f>.sign, 'toString', {
    value: function () {
        return defaultU2FString
    }
})
                      
// From: https://developer.mozilla.org/en-US/docs/Web/API/PublicKeyCredential/isUserVerifyingPlatformAuthenticatorAvailable
// We currently don't support a 'user-verifying platform authenticator'
Object.defineProperty($<pkc>, 'isUserVerifyingPlatformAuthenticatorAvailable', {
  value: function () {
    return new Promise(
      function (resolve, reject) {
        resolve(false);
      }
    )
  }
})


Object.defineProperty($<u2f>, 'register', {
  value: function (appId, registerRequests, registeredKeys, responseHandler) {
    if (typeof responseHandler != "function") {
      return
    }
    return new Promise(
      function (resolve, reject) {
        const handle = $<u2f-internal>.id++
        $<u2f-internal>.resolve[handle] = responseHandler
        window.top.$<u2f-internal>.caller[handle] = window
        webkit.messageHandlers.$<handler>.postMessage({ name: 'fido-register', appId: appId, requests: JSON.stringify(registerRequests), keys: JSON.stringify(registeredKeys), handle: handle })
      })
  }
})

Object.defineProperty($<u2f>.register, 'toString', {
    value: function () {
        return defaultU2FString
    }
})

// FIDO2 APIs are not available in 3p iframes
Object.defineProperty($<webauthn-internal>, 'undefineU2F', {
  value: function () {
    $<webauthn> = undefined;
    $<u2f> = undefined;
    $<attest> = undefined;
    $<assert> = undefined;
  }
})

Object.defineProperty(window, 'PublicKeyCredential', {
  value: $<pkc>
})

Object.defineProperty(window, 'AuthenticatorAttestationResponse', {
  value: $<attest>
})

Object.defineProperty(window, 'AuthenticatorAssertionResponse', {
  value: $<assert>
})

try {
    var sameOrigin = (window.top.location.origin == window.location.origin)
    if (!sameOrigin) {
        $<webauthn-internal>.undefineU2F();
    }
} catch (e) {
    $<webauthn-internal>.undefineU2F();
}

// Hook $<webauthn> to navigator.credentials
Object.defineProperty(navigator, 'credentials', {
  value: $<webauthn>
})

// Hook $<u2f> to window.u2f
Object.defineProperty(window, 'u2f', {
   value: $<u2f>
})

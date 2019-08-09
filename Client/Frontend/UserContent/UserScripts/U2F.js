// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// The $<> is used to replace the var name at runtime with a random string.

// FIDO2 - WebAuthn
var $<webauthn> = {}

// We use the define property method to avoid the properties from being changed
// by default configurable, enumerable and writable properties of the object
// are false
Object.defineProperty($<webauthn>, 'id', {
  value: 0
})

// Used to handle calls from iframes
Object.defineProperty($<webauthn>, 'caller', {
  value: []
 })

Object.defineProperty($<webauthn>, 'reject', {
  value: []
})

Object.defineProperty($<webauthn>, 'resolve', {
  value: []
})

Object.defineProperty($<webauthn>, 'data', {
  value: {}
})

// FIDO - High Level API
var $<u2f> = {}

Object.defineProperty($<u2f>, 'id', {
  value: 0
})

Object.defineProperty($<u2f>, 'resolve', {
  value: []
})

Object.defineProperty($<u2f>, 'caller', {
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

class $<u2fregister> {
  constructor (version, registrationData, clientData) {
    this.version = version
    this.registrationData = registrationData
    this.clientData = clientData
  }
}

class $<u2fsign> {
  constructor (keyHandle, signatureData, clientData) {
    this.keyHandle = keyHandle
    this.signatureData = signatureData
    this.clientData = clientData
  }
}

Object.defineProperty($<webauthn>, 'postCreate', {
  value:
    function (handle, fromNative, id, attestationObject, clientDataJSON, errorName, errorDescription) {
      if (fromNative) {
        caller = window.top.$<webauthn>.caller[handle]
        caller.$<webauthn>.postCreate(handle, false, id, attestationObject, clientDataJSON, errorName, errorDescription);
        return;
      }
      if (errorName) {
        $<webauthn>.reject[handle](new DOMException(atob(errorDescription), atob(errorName)))
        return
      }
      response = new $<attest>(attestationObject, clientDataJSON)
      data = new $<pkc>(id, response)
      $<webauthn>.resolve[handle](data)
    }
})

Object.defineProperty($<webauthn>, 'postGet', {
  value: function (handle, fromNative, id, authenticatorData, clientDataJSON, signature, userHandle, errorName, errorDescription) {
    if (fromNative) {
      caller = window.top.$<webauthn>.caller[handle]
      caller.$<webauthn>.postGet(handle, false, id, authenticatorData, clientDataJSON, signature, userHandle, errorName, errorDescription);
      return;
    }
    if (errorName) {
      $<webauthn>.reject[handle](new DOMException(atob(errorDescription), atob(errorName)))
      return
    }
    response = new $<assert>(authenticatorData, clientDataJSON, signature, userHandle)
    data = new $<pkc>(id, response)
    $<webauthn>.resolve[handle](data)
  }
})

Object.defineProperty($<u2f>, 'postSign', {
  value: function (handle, fromNative, keyHandle, signatureData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f>.caller[handle]
      caller.$<u2f>.postSign(handle, false, keyHandle, signatureData, clientData, errorCode, errorMessage);
      return;
    }
    if (errorCode > 1) {
      errorData = {
        'errorCode': errorCode,
        'errorMessage': atob(errorMessage)
      }
      $<u2f>.resolve[handle](errorData)
      return
    }
    response = new $<u2fsign>(keyHandle, signatureData, clientData)
    $<u2f>.resolve[handle](response)
  }
})

Object.defineProperty($<u2f>, 'postRegister', {
  value: function (handle, fromNative, version, registerationData, clientData, errorCode, errorMessage) {
    if (fromNative) {
      caller = window.top.$<u2f>.caller[handle]
      caller.$<u2f>.postRegister(handle, false, version, registerationData, clientData, errorCode, errorMessage);
      return;
    }
    if (errorCode > 1) {
      errorData = {
        'errorCode': errorCode,
        'errorMessage': atob(errorMessage)
      }
      $<u2f>.resolve[handle](errorData)
      return
    }
    response = new $<u2fregister>(version, registerationData, clientData)
    $<u2f>.resolve[handle](response)
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
    const handle = $<webauthn>.id++
    return new Promise(
      function (resolve, reject) {
        $<webauthn>.reject[handle] = reject
        $<webauthn>.resolve[handle] = resolve
        window.top.$<webauthn>.caller[handle] = window
        webkit.messageHandlers.U2F.postMessage({ name: 'fido2-create', data: cleanedArgs, handle: handle })
      }
    )
  }
})

Object.defineProperty($<webauthn>, 'get', {
  value: function (args) {
    const cleanedArgs = JSON.stringify(args, stringifyArrayCleaner)
    const handle = $<webauthn>.id++
    return new Promise(
      function (resolve, reject) {
        const handle = $<webauthn>.id++
        $<webauthn>.reject[handle] = reject
        $<webauthn>.resolve[handle] = resolve
        window.top.$<webauthn>.caller[handle] = window
        webkit.messageHandlers.U2F.postMessage({ name: 'fido2-get', data: cleanedArgs, handle: handle })
      }
    )
  }
})

Object.defineProperty($<u2f>, 'sign', {
  value: function (appId, challenge, registeredKeys, callback) {
    return new Promise(
      function (resolve, reject) {
        const handle = $<u2f>.id++
        $<u2f>.resolve[handle] = callback
        window.top.$<u2f>.caller[handle] = window
        webkit.messageHandlers.U2F.postMessage({ name: 'fido-sign', appId: appId, challenge: challenge, keys: JSON.stringify(registeredKeys), handle: handle })
      }
    )
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
    return new Promise(
      function (resolve, reject) {
        const handle = $<u2f>.id++
        $<u2f>.resolve[handle] = responseHandler
        window.top.$<u2f>.caller[handle] = window
        webkit.messageHandlers.U2F.postMessage({ name: 'fido-register', appId: appId, requests: JSON.stringify(registerRequests), keys: JSON.stringify(registeredKeys), handle: handle })
      })
  }
})

// FIDO2 APIs are not available in 3p iframes
Object.defineProperty(window, 'undefineU2F', {
  value: function () {
    $<webauthn> = undefined;
    $<u2f> = undefined;
    $<attest> = undefined;
    $<assert> = undefined;
    $<u2fsign> = undefined;
    $<u2fregister> = undefined;
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

Object.defineProperty($<u2f>, 'SignResponse', {
  value: $<u2fsign>
})

Object.defineProperty($<u2f>, 'RegisterResponse', {
  value: $<u2fregister>
})

try {
    var sameOrigin = (window.top.location.origin == window.location.origin)
    if (!sameOrigin) {
        window.undefineU2F();
    }
} catch (e) {
    window.undefineU2F();
}

// Hook $<webauthn> to navigator.credentials
Object.defineProperty(navigator, 'credentials', {
  value: $<webauthn>
})

// Hook $<u2f> to window.u2f
Object.defineProperty(window, 'u2f', {
   value: $<u2f>
})

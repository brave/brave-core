console.log(document.location.href);
/******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, { enumerable: true, get: getter });
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 			Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 		}
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// create a fake namespace object
/******/ 	// mode & 1: value is a module id, require it
/******/ 	// mode & 2: merge all properties of value into the ns
/******/ 	// mode & 4: return value when already ns object
/******/ 	// mode & 8|1: behave like require
/******/ 	__webpack_require__.t = function(value, mode) {
/******/ 		if(mode & 1) value = __webpack_require__(value);
/******/ 		if(mode & 8) return value;
/******/ 		if((mode & 4) && typeof value === 'object' && value && value.__esModule) return value;
/******/ 		var ns = Object.create(null);
/******/ 		__webpack_require__.r(ns);
/******/ 		Object.defineProperty(ns, 'default', { enumerable: true, value: value });
/******/ 		if(mode & 2 && typeof value != 'string') for(var key in value) __webpack_require__.d(ns, key, function(key) { return value[key]; }.bind(null, key));
/******/ 		return ns;
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "./";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = 35);
/******/ })
/************************************************************************/
/******/ ([
/* 0 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
var _exportNames = {};
exports["default"] = void 0;

var _defineProperty2 = _interopRequireDefault(__webpack_require__(4));

var _constants = __webpack_require__(2);

Object.keys(_constants).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;
  exports[key] = _constants[key];
});

var _node = __webpack_require__(45);

var _types = __webpack_require__(17);

Object.keys(_types).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  if (Object.prototype.hasOwnProperty.call(_exportNames, key)) return;
  exports[key] = _types[key];
});

function ownKeys(object, enumerableOnly) { var keys = Object.keys(object); if (Object.getOwnPropertySymbols) { var symbols = Object.getOwnPropertySymbols(object); if (enumerableOnly) symbols = symbols.filter(function (sym) { return Object.getOwnPropertyDescriptor(object, sym).enumerable; }); keys.push.apply(keys, symbols); } return keys; }

function _objectSpread(target) { for (var i = 1; i < arguments.length; i++) { var source = arguments[i] != null ? arguments[i] : {}; if (i % 2) { ownKeys(Object(source), true).forEach(function (key) { (0, _defineProperty2["default"])(target, key, source[key]); }); } else if (Object.getOwnPropertyDescriptors) { Object.defineProperties(target, Object.getOwnPropertyDescriptors(source)); } else { ownKeys(Object(source)).forEach(function (key) { Object.defineProperty(target, key, Object.getOwnPropertyDescriptor(source, key)); }); } } return target; }

var TrezorConnect = {
  manifest: _node.manifest,
  init: function init(settings) {
    return (0, _node.init)(settings);
  },
  getSettings: _node.getSettings,
  on: function on(type, fn) {
    _node.eventEmitter.on(type, fn);
  },
  off: function off(type, fn) {
    _node.eventEmitter.removeListener(type, fn);
  },
  removeAllListeners: function removeAllListeners() {
    _node.eventEmitter.removeAllListeners();
  },
  uiResponse: _node.uiResponse,
  // methods
  blockchainGetAccountBalanceHistory: function blockchainGetAccountBalanceHistory(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainGetAccountBalanceHistory'
    }, params));
  },
  blockchainGetCurrentFiatRates: function blockchainGetCurrentFiatRates(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainGetCurrentFiatRates'
    }, params));
  },
  blockchainGetFiatRatesForTimestamps: function blockchainGetFiatRatesForTimestamps(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainGetFiatRatesForTimestamps'
    }, params));
  },
  blockchainDisconnect: function blockchainDisconnect(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainDisconnect'
    }, params));
  },
  blockchainEstimateFee: function blockchainEstimateFee(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainEstimateFee'
    }, params));
  },
  blockchainGetTransactions: function blockchainGetTransactions(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainGetTransactions'
    }, params));
  },
  blockchainSetCustomBackend: function blockchainSetCustomBackend(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainSetCustomBackend'
    }, params));
  },
  blockchainSubscribe: function blockchainSubscribe(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainSubscribe'
    }, params));
  },
  blockchainSubscribeFiatRates: function blockchainSubscribeFiatRates(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainSubscribeFiatRates'
    }, params));
  },
  blockchainUnsubscribe: function blockchainUnsubscribe(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainUnsubscribe'
    }, params));
  },
  blockchainUnsubscribeFiatRates: function blockchainUnsubscribeFiatRates(params) {
    return (0, _node.call)(_objectSpread({
      method: 'blockchainUnsubscribeFiatRates'
    }, params));
  },
  customMessage: function customMessage(params) {
    return (0, _node.customMessage)(params);
  },
  requestLogin: function requestLogin(params) {
    return (0, _node.requestLogin)(params);
  },
  cardanoGetAddress: function cardanoGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'cardanoGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  cardanoGetPublicKey: function cardanoGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'cardanoGetPublicKey'
    }, params));
  },
  cardanoSignTransaction: function cardanoSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'cardanoSignTransaction'
    }, params));
  },
  cipherKeyValue: function cipherKeyValue(params) {
    return (0, _node.call)(_objectSpread({
      method: 'cipherKeyValue'
    }, params));
  },
  composeTransaction: function composeTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'composeTransaction'
    }, params));
  },
  debugLinkDecision: function debugLinkDecision(params) {
    return (0, _node.call)(_objectSpread({
      method: 'debugLinkDecision'
    }, params));
  },
  debugLinkGetState: function debugLinkGetState(params) {
    return (0, _node.call)(_objectSpread({
      method: 'debugLinkGetState'
    }, params));
  },
  ethereumGetAddress: function ethereumGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'ethereumGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  ethereumGetPublicKey: function ethereumGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'ethereumGetPublicKey'
    }, params));
  },
  ethereumSignMessage: function ethereumSignMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'ethereumSignMessage'
    }, params));
  },
  ethereumSignTransaction: function ethereumSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'ethereumSignTransaction'
    }, params));
  },
  ethereumVerifyMessage: function ethereumVerifyMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'ethereumVerifyMessage'
    }, params));
  },
  getAccountInfo: function getAccountInfo(params) {
    return (0, _node.call)(_objectSpread({
      method: 'getAccountInfo'
    }, params));
  },
  getAddress: function getAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'getAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  getDeviceState: function getDeviceState(params) {
    return (0, _node.call)(_objectSpread({
      method: 'getDeviceState'
    }, params));
  },
  getFeatures: function getFeatures(params) {
    return (0, _node.call)(_objectSpread({
      method: 'getFeatures'
    }, params));
  },
  getPublicKey: function getPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'getPublicKey'
    }, params));
  },
  liskGetAddress: function liskGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'liskGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  liskGetPublicKey: function liskGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'liskGetPublicKey'
    }, params));
  },
  liskSignMessage: function liskSignMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'liskSignMessage'
    }, params));
  },
  liskSignTransaction: function liskSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'liskSignTransaction'
    }, params));
  },
  liskVerifyMessage: function liskVerifyMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'liskVerifyMessage'
    }, params));
  },
  nemGetAddress: function nemGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'nemGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  nemSignTransaction: function nemSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'nemSignTransaction'
    }, params));
  },
  pushTransaction: function pushTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'pushTransaction'
    }, params));
  },
  rippleGetAddress: function rippleGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'rippleGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  rippleSignTransaction: function rippleSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'rippleSignTransaction'
    }, params));
  },
  signMessage: function signMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'signMessage'
    }, params));
  },
  signTransaction: function signTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'signTransaction'
    }, params));
  },
  stellarGetAddress: function stellarGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'stellarGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  stellarSignTransaction: function stellarSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'stellarSignTransaction'
    }, params));
  },
  tezosGetAddress: function tezosGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'tezosGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  tezosGetPublicKey: function tezosGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'tezosGetPublicKey'
    }, params));
  },
  tezosSignTransaction: function tezosSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'tezosSignTransaction'
    }, params));
  },
  eosGetPublicKey: function eosGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'eosGetPublicKey'
    }, params));
  },
  eosSignTransaction: function eosSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'eosSignTransaction'
    }, params));
  },
  binanceGetAddress: function binanceGetAddress(params) {
    var useEventListener = _node.eventEmitter.listenerCount(_constants.UI.ADDRESS_VALIDATION) > 0;
    return (0, _node.call)(_objectSpread({
      method: 'binanceGetAddress'
    }, params, {
      useEventListener: useEventListener
    }));
  },
  binanceGetPublicKey: function binanceGetPublicKey(params) {
    return (0, _node.call)(_objectSpread({
      method: 'binanceGetPublicKey'
    }, params));
  },
  binanceSignTransaction: function binanceSignTransaction(params) {
    return (0, _node.call)(_objectSpread({
      method: 'binanceSignTransaction'
    }, params));
  },
  verifyMessage: function verifyMessage(params) {
    return (0, _node.call)(_objectSpread({
      method: 'verifyMessage'
    }, params));
  },
  resetDevice: function resetDevice(params) {
    return (0, _node.call)(_objectSpread({
      method: 'resetDevice'
    }, params));
  },
  wipeDevice: function wipeDevice(params) {
    return (0, _node.call)(_objectSpread({
      method: 'wipeDevice'
    }, params));
  },
  applyFlags: function applyFlags(params) {
    return (0, _node.call)(_objectSpread({
      method: 'applyFlags'
    }, params));
  },
  applySettings: function applySettings(params) {
    return (0, _node.call)(_objectSpread({
      method: 'applySettings'
    }, params));
  },
  backupDevice: function backupDevice(params) {
    return (0, _node.call)(_objectSpread({
      method: 'backupDevice'
    }, params));
  },
  changePin: function changePin(params) {
    return (0, _node.call)(_objectSpread({
      method: 'changePin'
    }, params));
  },
  firmwareUpdate: function firmwareUpdate(params) {
    return (0, _node.call)(_objectSpread({
      method: 'firmwareUpdate'
    }, params));
  },
  recoveryDevice: function recoveryDevice(params) {
    return (0, _node.call)(_objectSpread({
      method: 'recoveryDevice'
    }, params));
  },
  dispose: function dispose() {
    (0, _node.dispose)();
  },
  cancel: _node.cancel,
  renderWebUSBButton: function renderWebUSBButton(className) {
    (0, _node.renderWebUSBButton)(className);
  },
  disableWebUSB: function disableWebUSB() {
    (0, _node.disableWebUSB)();
  }
};
var _default = TrezorConnect;
exports["default"] = _default;

/***/ }),
/* 1 */
/***/ (function(module, exports) {

function _interopRequireDefault(obj) {
  return obj && obj.__esModule ? obj : {
    "default": obj
  };
}

module.exports = _interopRequireDefault;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 2 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireWildcard = __webpack_require__(3);

exports.__esModule = true;
exports.UI = exports.TRANSPORT = exports.POPUP = exports.IFRAME = exports.ERRORS = exports.DEVICE = exports.BLOCKCHAIN = exports.BLOCKCHAIN_EVENT = exports.RESPONSE_EVENT = exports.TRANSPORT_EVENT = exports.DEVICE_EVENT = exports.UI_EVENT = exports.CORE_EVENT = void 0;

var BLOCKCHAIN = _interopRequireWildcard(__webpack_require__(37));

exports.BLOCKCHAIN = BLOCKCHAIN;

var DEVICE = _interopRequireWildcard(__webpack_require__(38));

exports.DEVICE = DEVICE;

var ERRORS = _interopRequireWildcard(__webpack_require__(9));

exports.ERRORS = ERRORS;

var IFRAME = _interopRequireWildcard(__webpack_require__(8));

exports.IFRAME = IFRAME;

var POPUP = _interopRequireWildcard(__webpack_require__(11));

exports.POPUP = POPUP;

var TRANSPORT = _interopRequireWildcard(__webpack_require__(44));

exports.TRANSPORT = TRANSPORT;

var UI = _interopRequireWildcard(__webpack_require__(12));

exports.UI = UI;
var CORE_EVENT = 'CORE_EVENT';
exports.CORE_EVENT = CORE_EVENT;
var UI_EVENT = 'UI_EVENT';
exports.UI_EVENT = UI_EVENT;
var DEVICE_EVENT = 'DEVICE_EVENT';
exports.DEVICE_EVENT = DEVICE_EVENT;
var TRANSPORT_EVENT = 'TRANSPORT_EVENT';
exports.TRANSPORT_EVENT = TRANSPORT_EVENT;
var RESPONSE_EVENT = 'RESPONSE_EVENT';
exports.RESPONSE_EVENT = RESPONSE_EVENT;
var BLOCKCHAIN_EVENT = 'BLOCKCHAIN_EVENT';
exports.BLOCKCHAIN_EVENT = BLOCKCHAIN_EVENT;

/***/ }),
/* 3 */
/***/ (function(module, exports, __webpack_require__) {

var _typeof = __webpack_require__(36)["default"];

function _getRequireWildcardCache(nodeInterop) {
  if (typeof WeakMap !== "function") return null;
  var cacheBabelInterop = new WeakMap();
  var cacheNodeInterop = new WeakMap();
  return (_getRequireWildcardCache = function _getRequireWildcardCache(nodeInterop) {
    return nodeInterop ? cacheNodeInterop : cacheBabelInterop;
  })(nodeInterop);
}

function _interopRequireWildcard(obj, nodeInterop) {
  if (!nodeInterop && obj && obj.__esModule) {
    return obj;
  }

  if (obj === null || _typeof(obj) !== "object" && typeof obj !== "function") {
    return {
      "default": obj
    };
  }

  var cache = _getRequireWildcardCache(nodeInterop);

  if (cache && cache.has(obj)) {
    return cache.get(obj);
  }

  var newObj = {};
  var hasPropertyDescriptor = Object.defineProperty && Object.getOwnPropertyDescriptor;

  for (var key in obj) {
    if (key !== "default" && Object.prototype.hasOwnProperty.call(obj, key)) {
      var desc = hasPropertyDescriptor ? Object.getOwnPropertyDescriptor(obj, key) : null;

      if (desc && (desc.get || desc.set)) {
        Object.defineProperty(newObj, key, desc);
      } else {
        newObj[key] = obj[key];
      }
    }
  }

  newObj["default"] = obj;

  if (cache) {
    cache.set(obj, newObj);
  }

  return newObj;
}

module.exports = _interopRequireWildcard;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 4 */
/***/ (function(module, exports) {

function _defineProperty(obj, key, value) {
  if (key in obj) {
    Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    });
  } else {
    obj[key] = value;
  }

  return obj;
}

module.exports = _defineProperty;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 5 */
/***/ (function(module, exports, __webpack_require__) {

module.exports = __webpack_require__(46);


/***/ }),
/* 6 */
/***/ (function(module, exports) {

function asyncGeneratorStep(gen, resolve, reject, _next, _throw, key, arg) {
  try {
    var info = gen[key](arg);
    var value = info.value;
  } catch (error) {
    reject(error);
    return;
  }

  if (info.done) {
    resolve(value);
  } else {
    Promise.resolve(value).then(_next, _throw);
  }
}

function _asyncToGenerator(fn) {
  return function () {
    var self = this,
        args = arguments;
    return new Promise(function (resolve, reject) {
      var gen = fn.apply(self, args);

      function _next(value) {
        asyncGeneratorStep(gen, resolve, reject, _next, _throw, "next", value);
      }

      function _throw(err) {
        asyncGeneratorStep(gen, resolve, reject, _next, _throw, "throw", err);
      }

      _next(undefined);
    });
  };
}

module.exports = _asyncToGenerator;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 7 */
/***/ (function(module, exports) {

function _setPrototypeOf(o, p) {
  module.exports = _setPrototypeOf = Object.setPrototypeOf || function _setPrototypeOf(o, p) {
    o.__proto__ = p;
    return o;
  };

  module.exports["default"] = module.exports, module.exports.__esModule = true;
  return _setPrototypeOf(o, p);
}

module.exports = _setPrototypeOf;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 8 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.CALL = exports.ERROR = exports.INIT = exports.LOADED = exports.BOOTSTRAP = void 0;
// Message called from iframe.html inline script before "window.onload" event. This is first message from iframe to window.opener.
var BOOTSTRAP = 'iframe-bootstrap'; // Message from iframe.js to window.opener, called after "window.onload" event. This is second message from iframe to window.opener.

exports.BOOTSTRAP = BOOTSTRAP;
var LOADED = 'iframe-loaded'; // Message from window.opener to iframe.js

exports.LOADED = LOADED;
var INIT = 'iframe-init'; // Error message from iframe.js to window.opener. Could be thrown during iframe initialization process

exports.INIT = INIT;
var ERROR = 'iframe-error'; // Message from window.opener to iframe. Call method

exports.ERROR = ERROR;
var CALL = 'iframe-call';
exports.CALL = CALL;

/***/ }),
/* 9 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.NO_COIN_INFO = exports.BACKEND_NO_URL = exports.backendNotSupported = exports.WEBUSB_ERROR_MESSAGE = exports.INVALID_PIN_ERROR_MESSAGE = exports.WRONG_PREVIOUS_SESSION_ERROR_MESSAGE = exports.INVALID_STATE = exports.CALL_OVERRIDE = exports.INITIALIZATION_FAILED = exports.DEVICE_USED_ELSEWHERE = exports.PERMISSIONS_NOT_GRANTED = exports.POPUP_CLOSED = exports.INVALID_PARAMETERS = exports.DEVICE_CALL_IN_PROGRESS = exports.DEVICE_NOT_FOUND = exports.WRONG_TRANSPORT_CONFIG = exports.NO_TRANSPORT = exports.MANAGEMENT_NOT_ALLOWED = exports.MANIFEST_NOT_SET = exports.BROWSER_NOT_SUPPORTED = exports.POPUP_TIMEOUT = exports.IFRAME_TIMEOUT = exports.IFRAME_INITIALIZED = exports.IFRAME_BLOCKED = exports.NO_IFRAME = exports.invalidParameter = exports.TrezorError = void 0;

var _inheritsLoose2 = _interopRequireDefault(__webpack_require__(10));

var _wrapNativeSuper2 = _interopRequireDefault(__webpack_require__(39));

var TrezorError = /*#__PURE__*/function (_Error) {
  (0, _inheritsLoose2["default"])(TrezorError, _Error);

  function TrezorError(code, message) {
    var _this;

    _this = _Error.call(this, message) || this;
    _this.code = code;
    _this.message = message;
    return _this;
  }

  return TrezorError;
}( /*#__PURE__*/(0, _wrapNativeSuper2["default"])(Error));

exports.TrezorError = TrezorError;

var invalidParameter = function invalidParameter(message) {
  return new TrezorError('Connect_InvalidParameter', message);
}; // level 100 error during initialization


exports.invalidParameter = invalidParameter;
var NO_IFRAME = new TrezorError(100, 'TrezorConnect not yet initialized');
exports.NO_IFRAME = NO_IFRAME;
var IFRAME_BLOCKED = new TrezorError('iframe_blocked', 'TrezorConnect iframe was blocked');
exports.IFRAME_BLOCKED = IFRAME_BLOCKED;
var IFRAME_INITIALIZED = new TrezorError(101, 'TrezorConnect has been already initialized');
exports.IFRAME_INITIALIZED = IFRAME_INITIALIZED;
var IFRAME_TIMEOUT = new TrezorError(102, 'Iframe timeout');
exports.IFRAME_TIMEOUT = IFRAME_TIMEOUT;
var POPUP_TIMEOUT = new TrezorError(103, 'Popup timeout');
exports.POPUP_TIMEOUT = POPUP_TIMEOUT;
var BROWSER_NOT_SUPPORTED = new TrezorError(104, 'Browser not supported');
exports.BROWSER_NOT_SUPPORTED = BROWSER_NOT_SUPPORTED;
var MANIFEST_NOT_SET = new TrezorError(105, 'Manifest not set. Read more at https://github.com/trezor/connect/blob/develop/docs/index.md');
exports.MANIFEST_NOT_SET = MANIFEST_NOT_SET;
var MANAGEMENT_NOT_ALLOWED = new TrezorError(105, 'Management method not allowed for this configuration');
exports.MANAGEMENT_NOT_ALLOWED = MANAGEMENT_NOT_ALLOWED;
var NO_TRANSPORT = new TrezorError(500, 'Transport is missing');
exports.NO_TRANSPORT = NO_TRANSPORT;
var WRONG_TRANSPORT_CONFIG = new TrezorError(5002, 'Wrong config response'); // config_signed

exports.WRONG_TRANSPORT_CONFIG = WRONG_TRANSPORT_CONFIG;
var DEVICE_NOT_FOUND = new TrezorError(501, 'Device not found'); // export const DEVICE_CALL_IN_PROGRESS: TrezorError = new TrezorError(502, "Device call in progress.");

exports.DEVICE_NOT_FOUND = DEVICE_NOT_FOUND;
var DEVICE_CALL_IN_PROGRESS = new TrezorError(503, 'Device call in progress');
exports.DEVICE_CALL_IN_PROGRESS = DEVICE_CALL_IN_PROGRESS;
var INVALID_PARAMETERS = new TrezorError(504, 'Invalid parameters');
exports.INVALID_PARAMETERS = INVALID_PARAMETERS;
var POPUP_CLOSED = new Error('Popup closed');
exports.POPUP_CLOSED = POPUP_CLOSED;
var PERMISSIONS_NOT_GRANTED = new TrezorError(403, 'Permissions not granted');
exports.PERMISSIONS_NOT_GRANTED = PERMISSIONS_NOT_GRANTED;
var DEVICE_USED_ELSEWHERE = new TrezorError(700, 'Device is used in another window');
exports.DEVICE_USED_ELSEWHERE = DEVICE_USED_ELSEWHERE;
var INITIALIZATION_FAILED = new TrezorError('Failure_Initialize', 'Initialization failed');
exports.INITIALIZATION_FAILED = INITIALIZATION_FAILED;
var CALL_OVERRIDE = new TrezorError('Failure_ActionOverride', 'override');
exports.CALL_OVERRIDE = CALL_OVERRIDE;
var INVALID_STATE = new TrezorError('Failure_PassphraseState', 'Passphrase is incorrect'); // a slight hack
// this error string is hard-coded
// in both bridge and extension

exports.INVALID_STATE = INVALID_STATE;
var WRONG_PREVIOUS_SESSION_ERROR_MESSAGE = 'wrong previous session';
exports.WRONG_PREVIOUS_SESSION_ERROR_MESSAGE = WRONG_PREVIOUS_SESSION_ERROR_MESSAGE;
var INVALID_PIN_ERROR_MESSAGE = 'PIN invalid';
exports.INVALID_PIN_ERROR_MESSAGE = INVALID_PIN_ERROR_MESSAGE;
var WEBUSB_ERROR_MESSAGE = 'NetworkError: Unable to claim interface.'; // BlockBook

exports.WEBUSB_ERROR_MESSAGE = WEBUSB_ERROR_MESSAGE;

var backendNotSupported = function backendNotSupported(coinName) {
  return new TrezorError('backend_error', "BlockchainLink support not found for " + coinName);
};

exports.backendNotSupported = backendNotSupported;
var BACKEND_NO_URL = new TrezorError('Backend_init', 'Url not found');
exports.BACKEND_NO_URL = BACKEND_NO_URL;
var NO_COIN_INFO = invalidParameter('Coin not found.');
exports.NO_COIN_INFO = NO_COIN_INFO;

/***/ }),
/* 10 */
/***/ (function(module, exports, __webpack_require__) {

var setPrototypeOf = __webpack_require__(7);

function _inheritsLoose(subClass, superClass) {
  subClass.prototype = Object.create(superClass.prototype);
  subClass.prototype.constructor = subClass;
  setPrototypeOf(subClass, superClass);
}

module.exports = _inheritsLoose;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 11 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.CLOSE_WINDOW = exports.CANCEL_POPUP_REQUEST = exports.CLOSED = exports.HANDSHAKE = exports.EXTENSION_USB_PERMISSIONS = exports.ERROR = exports.INIT = exports.LOADED = exports.BOOTSTRAP = void 0;
// Message called from popup.html inline script before "window.onload" event. This is first message from popup to window.opener.
var BOOTSTRAP = 'popup-bootstrap'; // Message from popup.js to window.opener, called after "window.onload" event. This is second message from popup to window.opener.

exports.BOOTSTRAP = BOOTSTRAP;
var LOADED = 'popup-loaded'; // Message from window.opener to popup.js. Send settings to popup. This is first message from window.opener to popup.

exports.LOADED = LOADED;
var INIT = 'popup-init'; // Error message from popup to window.opener. Could be thrown during popup initialization process (POPUP.INIT)

exports.INIT = INIT;
var ERROR = 'popup-error'; // Message to webextensions, opens "trezor-usb-permission.html" within webextension

exports.ERROR = ERROR;
var EXTENSION_USB_PERMISSIONS = 'open-usb-permissions'; // Message called from both [popup > iframe] then [iframe > popup] in this exact order.
// Firstly popup call iframe to resolve popup promise in Core
// Then iframe reacts to POPUP.HANDSHAKE message and sends ConnectSettings, transport information and requested method details back to popup

exports.EXTENSION_USB_PERMISSIONS = EXTENSION_USB_PERMISSIONS;
var HANDSHAKE = 'popup-handshake'; // Event emitted from PopupManager at the end of popup closing process.
// Sent from popup thru window.opener to an iframe because message channel between popup and iframe is no longer available

exports.HANDSHAKE = HANDSHAKE;
var CLOSED = 'popup-closed'; // Message called from iframe to popup, it means that popup will not be needed (example: Blockchain methods are not using popup at all)
// This will close active popup window and/or clear opening process in PopupManager (maybe popup wasn't opened yet)

exports.CLOSED = CLOSED;
var CANCEL_POPUP_REQUEST = 'ui-cancel-popup-request'; // Message called from inline element in popup.html (window.closeWindow), this is used only with webextensions to properly handle popup close event

exports.CANCEL_POPUP_REQUEST = CANCEL_POPUP_REQUEST;
var CLOSE_WINDOW = 'window.close';
exports.CLOSE_WINDOW = CLOSE_WINDOW;

/***/ }),
/* 12 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.ADDRESS_VALIDATION = exports.BUNDLE_PROGRESS = exports.LOGIN_CHALLENGE_RESPONSE = exports.LOGIN_CHALLENGE_REQUEST = exports.CUSTOM_MESSAGE_RESPONSE = exports.CUSTOM_MESSAGE_REQUEST = exports.CHANGE_SETTINGS = exports.RECEIVE_WORD = exports.RECEIVE_FEE = exports.RECEIVE_ACCOUNT = exports.CHANGE_ACCOUNT = exports.RECEIVE_DEVICE = exports.RECEIVE_PASSPHRASE = exports.RECEIVE_PIN = exports.RECEIVE_CONFIRMATION = exports.RECEIVE_PERMISSION = exports.REQUEST_WORD = exports.REQUEST_BUTTON = exports.INSUFFICIENT_FUNDS = exports.UPDATE_CUSTOM_FEE = exports.SELECT_FEE = exports.SELECT_ACCOUNT = exports.SELECT_DEVICE = exports.SET_OPERATION = exports.LOADING = exports.CONNECT = exports.INVALID_PASSPHRASE_ACTION = exports.INVALID_PASSPHRASE = exports.REQUEST_PASSPHRASE_ON_DEVICE = exports.REQUEST_PASSPHRASE = exports.INVALID_PIN = exports.REQUEST_PIN = exports.REQUEST_CONFIRMATION = exports.REQUEST_PERMISSION = exports.CLOSE_UI_WINDOW = exports.REQUEST_UI_WINDOW = exports.DEVICE_NEEDS_BACKUP = exports.FIRMWARE_PROGRESS = exports.FIRMWARE_NOT_INSTALLED = exports.FIRMWARE_NOT_COMPATIBLE = exports.FIRMWARE_NOT_SUPPORTED = exports.FIRMWARE_OUTDATED = exports.FIRMWARE_OLD = exports.SEEDLESS = exports.INITIALIZE = exports.REQUIRE_MODE = exports.NOT_IN_BOOTLOADER = exports.BOOTLOADER = exports.TRANSPORT = void 0;
var TRANSPORT = 'ui-no_transport';
exports.TRANSPORT = TRANSPORT;
var BOOTLOADER = 'ui-device_bootloader_mode';
exports.BOOTLOADER = BOOTLOADER;
var NOT_IN_BOOTLOADER = 'ui-device_not_in_bootloader_mode';
exports.NOT_IN_BOOTLOADER = NOT_IN_BOOTLOADER;
var REQUIRE_MODE = 'ui-device_require_mode';
exports.REQUIRE_MODE = REQUIRE_MODE;
var INITIALIZE = 'ui-device_not_initialized';
exports.INITIALIZE = INITIALIZE;
var SEEDLESS = 'ui-device_seedless';
exports.SEEDLESS = SEEDLESS;
var FIRMWARE_OLD = 'ui-device_firmware_old';
exports.FIRMWARE_OLD = FIRMWARE_OLD;
var FIRMWARE_OUTDATED = 'ui-device_firmware_outdated';
exports.FIRMWARE_OUTDATED = FIRMWARE_OUTDATED;
var FIRMWARE_NOT_SUPPORTED = 'ui-device_firmware_unsupported';
exports.FIRMWARE_NOT_SUPPORTED = FIRMWARE_NOT_SUPPORTED;
var FIRMWARE_NOT_COMPATIBLE = 'ui-device_firmware_not_compatible';
exports.FIRMWARE_NOT_COMPATIBLE = FIRMWARE_NOT_COMPATIBLE;
var FIRMWARE_NOT_INSTALLED = 'ui-device_firmware_not_installed';
exports.FIRMWARE_NOT_INSTALLED = FIRMWARE_NOT_INSTALLED;
var FIRMWARE_PROGRESS = 'ui-firmware-progress';
exports.FIRMWARE_PROGRESS = FIRMWARE_PROGRESS;
var DEVICE_NEEDS_BACKUP = 'ui-device_needs_backup';
exports.DEVICE_NEEDS_BACKUP = DEVICE_NEEDS_BACKUP;
var REQUEST_UI_WINDOW = 'ui-request_window';
exports.REQUEST_UI_WINDOW = REQUEST_UI_WINDOW;
var CLOSE_UI_WINDOW = 'ui-close_window';
exports.CLOSE_UI_WINDOW = CLOSE_UI_WINDOW;
var REQUEST_PERMISSION = 'ui-request_permission';
exports.REQUEST_PERMISSION = REQUEST_PERMISSION;
var REQUEST_CONFIRMATION = 'ui-request_confirmation';
exports.REQUEST_CONFIRMATION = REQUEST_CONFIRMATION;
var REQUEST_PIN = 'ui-request_pin';
exports.REQUEST_PIN = REQUEST_PIN;
var INVALID_PIN = 'ui-invalid_pin';
exports.INVALID_PIN = INVALID_PIN;
var REQUEST_PASSPHRASE = 'ui-request_passphrase';
exports.REQUEST_PASSPHRASE = REQUEST_PASSPHRASE;
var REQUEST_PASSPHRASE_ON_DEVICE = 'ui-request_passphrase_on_device';
exports.REQUEST_PASSPHRASE_ON_DEVICE = REQUEST_PASSPHRASE_ON_DEVICE;
var INVALID_PASSPHRASE = 'ui-invalid_passphrase';
exports.INVALID_PASSPHRASE = INVALID_PASSPHRASE;
var INVALID_PASSPHRASE_ACTION = 'ui-invalid_passphrase_action';
exports.INVALID_PASSPHRASE_ACTION = INVALID_PASSPHRASE_ACTION;
var CONNECT = 'ui-connect';
exports.CONNECT = CONNECT;
var LOADING = 'ui-loading';
exports.LOADING = LOADING;
var SET_OPERATION = 'ui-set_operation';
exports.SET_OPERATION = SET_OPERATION;
var SELECT_DEVICE = 'ui-select_device';
exports.SELECT_DEVICE = SELECT_DEVICE;
var SELECT_ACCOUNT = 'ui-select_account';
exports.SELECT_ACCOUNT = SELECT_ACCOUNT;
var SELECT_FEE = 'ui-select_fee';
exports.SELECT_FEE = SELECT_FEE;
var UPDATE_CUSTOM_FEE = 'ui-update_custom_fee';
exports.UPDATE_CUSTOM_FEE = UPDATE_CUSTOM_FEE;
var INSUFFICIENT_FUNDS = 'ui-insufficient_funds';
exports.INSUFFICIENT_FUNDS = INSUFFICIENT_FUNDS;
var REQUEST_BUTTON = 'ui-button';
exports.REQUEST_BUTTON = REQUEST_BUTTON;
var REQUEST_WORD = 'ui-request_word';
exports.REQUEST_WORD = REQUEST_WORD;
var RECEIVE_PERMISSION = 'ui-receive_permission';
exports.RECEIVE_PERMISSION = RECEIVE_PERMISSION;
var RECEIVE_CONFIRMATION = 'ui-receive_confirmation';
exports.RECEIVE_CONFIRMATION = RECEIVE_CONFIRMATION;
var RECEIVE_PIN = 'ui-receive_pin';
exports.RECEIVE_PIN = RECEIVE_PIN;
var RECEIVE_PASSPHRASE = 'ui-receive_passphrase';
exports.RECEIVE_PASSPHRASE = RECEIVE_PASSPHRASE;
var RECEIVE_DEVICE = 'ui-receive_device';
exports.RECEIVE_DEVICE = RECEIVE_DEVICE;
var CHANGE_ACCOUNT = 'ui-change_account';
exports.CHANGE_ACCOUNT = CHANGE_ACCOUNT;
var RECEIVE_ACCOUNT = 'ui-receive_account';
exports.RECEIVE_ACCOUNT = RECEIVE_ACCOUNT;
var RECEIVE_FEE = 'ui-receive_fee';
exports.RECEIVE_FEE = RECEIVE_FEE;
var RECEIVE_WORD = 'ui-receive_word';
exports.RECEIVE_WORD = RECEIVE_WORD;
var CHANGE_SETTINGS = 'ui-change_settings';
exports.CHANGE_SETTINGS = CHANGE_SETTINGS;
var CUSTOM_MESSAGE_REQUEST = 'ui-custom_request';
exports.CUSTOM_MESSAGE_REQUEST = CUSTOM_MESSAGE_REQUEST;
var CUSTOM_MESSAGE_RESPONSE = 'ui-custom_response';
exports.CUSTOM_MESSAGE_RESPONSE = CUSTOM_MESSAGE_RESPONSE;
var LOGIN_CHALLENGE_REQUEST = 'ui-login_challenge_request';
exports.LOGIN_CHALLENGE_REQUEST = LOGIN_CHALLENGE_REQUEST;
var LOGIN_CHALLENGE_RESPONSE = 'ui-login_challenge_response';
exports.LOGIN_CHALLENGE_RESPONSE = LOGIN_CHALLENGE_RESPONSE;
var BUNDLE_PROGRESS = 'ui-bundle_progress';
exports.BUNDLE_PROGRESS = BUNDLE_PROGRESS;
var ADDRESS_VALIDATION = 'ui-address_validation';
exports.ADDRESS_VALIDATION = ADDRESS_VALIDATION;

/***/ }),
/* 13 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.



var R = typeof Reflect === 'object' ? Reflect : null
var ReflectApply = R && typeof R.apply === 'function'
  ? R.apply
  : function ReflectApply(target, receiver, args) {
    return Function.prototype.apply.call(target, receiver, args);
  }

var ReflectOwnKeys
if (R && typeof R.ownKeys === 'function') {
  ReflectOwnKeys = R.ownKeys
} else if (Object.getOwnPropertySymbols) {
  ReflectOwnKeys = function ReflectOwnKeys(target) {
    return Object.getOwnPropertyNames(target)
      .concat(Object.getOwnPropertySymbols(target));
  };
} else {
  ReflectOwnKeys = function ReflectOwnKeys(target) {
    return Object.getOwnPropertyNames(target);
  };
}

function ProcessEmitWarning(warning) {
  if (console && console.warn) console.warn(warning);
}

var NumberIsNaN = Number.isNaN || function NumberIsNaN(value) {
  return value !== value;
}

function EventEmitter() {
  EventEmitter.init.call(this);
}
module.exports = EventEmitter;
module.exports.once = once;

// Backwards-compat with node 0.10.x
EventEmitter.EventEmitter = EventEmitter;

EventEmitter.prototype._events = undefined;
EventEmitter.prototype._eventsCount = 0;
EventEmitter.prototype._maxListeners = undefined;

// By default EventEmitters will print a warning if more than 10 listeners are
// added to it. This is a useful default which helps finding memory leaks.
var defaultMaxListeners = 10;

function checkListener(listener) {
  if (typeof listener !== 'function') {
    throw new TypeError('The "listener" argument must be of type Function. Received type ' + typeof listener);
  }
}

Object.defineProperty(EventEmitter, 'defaultMaxListeners', {
  enumerable: true,
  get: function() {
    return defaultMaxListeners;
  },
  set: function(arg) {
    if (typeof arg !== 'number' || arg < 0 || NumberIsNaN(arg)) {
      throw new RangeError('The value of "defaultMaxListeners" is out of range. It must be a non-negative number. Received ' + arg + '.');
    }
    defaultMaxListeners = arg;
  }
});

EventEmitter.init = function() {

  if (this._events === undefined ||
      this._events === Object.getPrototypeOf(this)._events) {
    this._events = Object.create(null);
    this._eventsCount = 0;
  }

  this._maxListeners = this._maxListeners || undefined;
};

// Obviously not all Emitters should be limited to 10. This function allows
// that to be increased. Set to zero for unlimited.
EventEmitter.prototype.setMaxListeners = function setMaxListeners(n) {
  if (typeof n !== 'number' || n < 0 || NumberIsNaN(n)) {
    throw new RangeError('The value of "n" is out of range. It must be a non-negative number. Received ' + n + '.');
  }
  this._maxListeners = n;
  return this;
};

function _getMaxListeners(that) {
  if (that._maxListeners === undefined)
    return EventEmitter.defaultMaxListeners;
  return that._maxListeners;
}

EventEmitter.prototype.getMaxListeners = function getMaxListeners() {
  return _getMaxListeners(this);
};

EventEmitter.prototype.emit = function emit(type) {
  var args = [];
  for (var i = 1; i < arguments.length; i++) args.push(arguments[i]);
  var doError = (type === 'error');

  var events = this._events;
  if (events !== undefined)
    doError = (doError && events.error === undefined);
  else if (!doError)
    return false;

  // If there is no 'error' event listener then throw.
  if (doError) {
    var er;
    if (args.length > 0)
      er = args[0];
    if (er instanceof Error) {
      // Note: The comments on the `throw` lines are intentional, they show
      // up in Node's output if this results in an unhandled exception.
      throw er; // Unhandled 'error' event
    }
    // At least give some kind of context to the user
    var err = new Error('Unhandled error.' + (er ? ' (' + er.message + ')' : ''));
    err.context = er;
    throw err; // Unhandled 'error' event
  }

  var handler = events[type];

  if (handler === undefined)
    return false;

  if (typeof handler === 'function') {
    ReflectApply(handler, this, args);
  } else {
    var len = handler.length;
    var listeners = arrayClone(handler, len);
    for (var i = 0; i < len; ++i)
      ReflectApply(listeners[i], this, args);
  }

  return true;
};

function _addListener(target, type, listener, prepend) {
  var m;
  var events;
  var existing;

  checkListener(listener);

  events = target._events;
  if (events === undefined) {
    events = target._events = Object.create(null);
    target._eventsCount = 0;
  } else {
    // To avoid recursion in the case that type === "newListener"! Before
    // adding it to the listeners, first emit "newListener".
    if (events.newListener !== undefined) {
      target.emit('newListener', type,
                  listener.listener ? listener.listener : listener);

      // Re-assign `events` because a newListener handler could have caused the
      // this._events to be assigned to a new object
      events = target._events;
    }
    existing = events[type];
  }

  if (existing === undefined) {
    // Optimize the case of one listener. Don't need the extra array object.
    existing = events[type] = listener;
    ++target._eventsCount;
  } else {
    if (typeof existing === 'function') {
      // Adding the second element, need to change to array.
      existing = events[type] =
        prepend ? [listener, existing] : [existing, listener];
      // If we've already got an array, just append.
    } else if (prepend) {
      existing.unshift(listener);
    } else {
      existing.push(listener);
    }

    // Check for listener leak
    m = _getMaxListeners(target);
    if (m > 0 && existing.length > m && !existing.warned) {
      existing.warned = true;
      // No error code for this since it is a Warning
      // eslint-disable-next-line no-restricted-syntax
      var w = new Error('Possible EventEmitter memory leak detected. ' +
                          existing.length + ' ' + String(type) + ' listeners ' +
                          'added. Use emitter.setMaxListeners() to ' +
                          'increase limit');
      w.name = 'MaxListenersExceededWarning';
      w.emitter = target;
      w.type = type;
      w.count = existing.length;
      ProcessEmitWarning(w);
    }
  }

  return target;
}

EventEmitter.prototype.addListener = function addListener(type, listener) {
  return _addListener(this, type, listener, false);
};

EventEmitter.prototype.on = EventEmitter.prototype.addListener;

EventEmitter.prototype.prependListener =
    function prependListener(type, listener) {
      return _addListener(this, type, listener, true);
    };

function onceWrapper() {
  if (!this.fired) {
    this.target.removeListener(this.type, this.wrapFn);
    this.fired = true;
    if (arguments.length === 0)
      return this.listener.call(this.target);
    return this.listener.apply(this.target, arguments);
  }
}

function _onceWrap(target, type, listener) {
  var state = { fired: false, wrapFn: undefined, target: target, type: type, listener: listener };
  var wrapped = onceWrapper.bind(state);
  wrapped.listener = listener;
  state.wrapFn = wrapped;
  return wrapped;
}

EventEmitter.prototype.once = function once(type, listener) {
  checkListener(listener);
  this.on(type, _onceWrap(this, type, listener));
  return this;
};

EventEmitter.prototype.prependOnceListener =
    function prependOnceListener(type, listener) {
      checkListener(listener);
      this.prependListener(type, _onceWrap(this, type, listener));
      return this;
    };

// Emits a 'removeListener' event if and only if the listener was removed.
EventEmitter.prototype.removeListener =
    function removeListener(type, listener) {
      var list, events, position, i, originalListener;

      checkListener(listener);

      events = this._events;
      if (events === undefined)
        return this;

      list = events[type];
      if (list === undefined)
        return this;

      if (list === listener || list.listener === listener) {
        if (--this._eventsCount === 0)
          this._events = Object.create(null);
        else {
          delete events[type];
          if (events.removeListener)
            this.emit('removeListener', type, list.listener || listener);
        }
      } else if (typeof list !== 'function') {
        position = -1;

        for (i = list.length - 1; i >= 0; i--) {
          if (list[i] === listener || list[i].listener === listener) {
            originalListener = list[i].listener;
            position = i;
            break;
          }
        }

        if (position < 0)
          return this;

        if (position === 0)
          list.shift();
        else {
          spliceOne(list, position);
        }

        if (list.length === 1)
          events[type] = list[0];

        if (events.removeListener !== undefined)
          this.emit('removeListener', type, originalListener || listener);
      }

      return this;
    };

EventEmitter.prototype.off = EventEmitter.prototype.removeListener;

EventEmitter.prototype.removeAllListeners =
    function removeAllListeners(type) {
      var listeners, events, i;

      events = this._events;
      if (events === undefined)
        return this;

      // not listening for removeListener, no need to emit
      if (events.removeListener === undefined) {
        if (arguments.length === 0) {
          this._events = Object.create(null);
          this._eventsCount = 0;
        } else if (events[type] !== undefined) {
          if (--this._eventsCount === 0)
            this._events = Object.create(null);
          else
            delete events[type];
        }
        return this;
      }

      // emit removeListener for all listeners on all events
      if (arguments.length === 0) {
        var keys = Object.keys(events);
        var key;
        for (i = 0; i < keys.length; ++i) {
          key = keys[i];
          if (key === 'removeListener') continue;
          this.removeAllListeners(key);
        }
        this.removeAllListeners('removeListener');
        this._events = Object.create(null);
        this._eventsCount = 0;
        return this;
      }

      listeners = events[type];

      if (typeof listeners === 'function') {
        this.removeListener(type, listeners);
      } else if (listeners !== undefined) {
        // LIFO order
        for (i = listeners.length - 1; i >= 0; i--) {
          this.removeListener(type, listeners[i]);
        }
      }

      return this;
    };

function _listeners(target, type, unwrap) {
  var events = target._events;

  if (events === undefined)
    return [];

  var evlistener = events[type];
  if (evlistener === undefined)
    return [];

  if (typeof evlistener === 'function')
    return unwrap ? [evlistener.listener || evlistener] : [evlistener];

  return unwrap ?
    unwrapListeners(evlistener) : arrayClone(evlistener, evlistener.length);
}

EventEmitter.prototype.listeners = function listeners(type) {
  return _listeners(this, type, true);
};

EventEmitter.prototype.rawListeners = function rawListeners(type) {
  return _listeners(this, type, false);
};

EventEmitter.listenerCount = function(emitter, type) {
  if (typeof emitter.listenerCount === 'function') {
    return emitter.listenerCount(type);
  } else {
    return listenerCount.call(emitter, type);
  }
};

EventEmitter.prototype.listenerCount = listenerCount;
function listenerCount(type) {
  var events = this._events;

  if (events !== undefined) {
    var evlistener = events[type];

    if (typeof evlistener === 'function') {
      return 1;
    } else if (evlistener !== undefined) {
      return evlistener.length;
    }
  }

  return 0;
}

EventEmitter.prototype.eventNames = function eventNames() {
  return this._eventsCount > 0 ? ReflectOwnKeys(this._events) : [];
};

function arrayClone(arr, n) {
  var copy = new Array(n);
  for (var i = 0; i < n; ++i)
    copy[i] = arr[i];
  return copy;
}

function spliceOne(list, index) {
  for (; index + 1 < list.length; index++)
    list[index] = list[index + 1];
  list.pop();
}

function unwrapListeners(arr) {
  var ret = new Array(arr.length);
  for (var i = 0; i < ret.length; ++i) {
    ret[i] = arr[i].listener || arr[i];
  }
  return ret;
}

function once(emitter, name) {
  return new Promise(function (resolve, reject) {
    function errorListener(err) {
      emitter.removeListener(name, resolver);
      reject(err);
    }

    function resolver() {
      if (typeof emitter.removeListener === 'function') {
        emitter.removeListener('error', errorListener);
      }
      resolve([].slice.call(arguments));
    };

    eventTargetAgnosticAddListener(emitter, name, resolver, { once: true });
    if (name !== 'error') {
      addErrorHandlerIfEventEmitter(emitter, errorListener, { once: true });
    }
  });
}

function addErrorHandlerIfEventEmitter(emitter, handler, flags) {
  if (typeof emitter.on === 'function') {
    eventTargetAgnosticAddListener(emitter, 'error', handler, flags);
  }
}

function eventTargetAgnosticAddListener(emitter, name, listener, flags) {
  if (typeof emitter.on === 'function') {
    if (flags.once) {
      emitter.once(name, listener);
    } else {
      emitter.on(name, listener);
    }
  } else if (typeof emitter.addEventListener === 'function') {
    // EventTarget does not have `error` event semantics like Node
    // EventEmitters, we do not listen for `error` events here.
    emitter.addEventListener(name, function wrapListener(arg) {
      // IE does not have builtin `{ once: true }` support so we
      // have to do it manually.
      if (flags.once) {
        emitter.removeEventListener(name, wrapListener);
      }
      listener(arg);
    });
  } else {
    throw new TypeError('The "emitter" argument must be of type EventEmitter. Received type ' + typeof emitter);
  }
}


/***/ }),
/* 14 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.getOrigin = exports.httpRequest = void 0;

var _regenerator = _interopRequireDefault(__webpack_require__(5));

var _asyncToGenerator2 = _interopRequireDefault(__webpack_require__(6));

__webpack_require__(50);

var httpRequest = /*#__PURE__*/function () {
  var _ref = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee(url, type) {
    var response, txt;
    return _regenerator["default"].wrap(function _callee$(_context) {
      while (1) {
        switch (_context.prev = _context.next) {
          case 0:
            if (type === void 0) {
              type = 'text';
            }

            _context.next = 3;
            return fetch(url, {
              credentials: 'same-origin'
            });

          case 3:
            response = _context.sent;

            if (!response.ok) {
              _context.next = 23;
              break;
            }

            if (!(type === 'json')) {
              _context.next = 12;
              break;
            }

            _context.next = 8;
            return response.text();

          case 8:
            txt = _context.sent;
            return _context.abrupt("return", JSON.parse(txt));

          case 12:
            if (!(type === 'binary')) {
              _context.next = 18;
              break;
            }

            _context.next = 15;
            return response.arrayBuffer();

          case 15:
            return _context.abrupt("return", _context.sent);

          case 18:
            _context.next = 20;
            return response.text();

          case 20:
            return _context.abrupt("return", _context.sent);

          case 21:
            _context.next = 24;
            break;

          case 23:
            throw new Error("httpRequest error: " + url + " " + response.statusText);

          case 24:
          case "end":
            return _context.stop();
        }
      }
    }, _callee);
  }));

  return function httpRequest(_x, _x2) {
    return _ref.apply(this, arguments);
  };
}();

exports.httpRequest = httpRequest;

var getOrigin = function getOrigin(url) {
  if (url.indexOf('file://') === 0) return 'file://'; // eslint-disable-next-line no-irregular-whitespace, no-useless-escape

  var parts = url.match(/^.+\:\/\/[^\/]+/);
  return Array.isArray(parts) && parts.length > 0 ? parts[0] : 'unknown';
};

exports.getOrigin = getOrigin;

/***/ }),
/* 15 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.create = create;
exports.createAsync = createAsync;
exports.resolveTimeoutPromise = resolveTimeoutPromise;
exports.rejectTimeoutPromise = rejectTimeoutPromise;

var _regenerator = _interopRequireDefault(__webpack_require__(5));

var _asyncToGenerator2 = _interopRequireDefault(__webpack_require__(6));

function create(arg, device) {
  var localResolve = function localResolve(t) {};

  var localReject = function localReject(e) {};

  var id; // eslint-disable-next-line no-async-promise-executor

  var promise = new Promise( /*#__PURE__*/function () {
    var _ref = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee(resolve, reject) {
      return _regenerator["default"].wrap(function _callee$(_context) {
        while (1) {
          switch (_context.prev = _context.next) {
            case 0:
              localResolve = resolve;
              localReject = reject;

              if (!(typeof arg === 'function')) {
                _context.next = 11;
                break;
              }

              _context.prev = 3;
              _context.next = 6;
              return arg();

            case 6:
              _context.next = 11;
              break;

            case 8:
              _context.prev = 8;
              _context.t0 = _context["catch"](3);
              reject(_context.t0);

            case 11:
              if (typeof arg === 'string') id = arg;

            case 12:
            case "end":
              return _context.stop();
          }
        }
      }, _callee, null, [[3, 8]]);
    }));

    return function (_x, _x2) {
      return _ref.apply(this, arguments);
    };
  }());
  return {
    id: id,
    device: device,
    resolve: localResolve,
    reject: localReject,
    promise: promise
  };
}

function createAsync(innerFn) {
  var localResolve = function localResolve(t) {};

  var localReject = function localReject(e) {};

  var promise = new Promise(function (resolve, reject) {
    localResolve = resolve;
    localReject = reject;
  });

  var inner = /*#__PURE__*/function () {
    var _ref2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee2() {
      return _regenerator["default"].wrap(function _callee2$(_context2) {
        while (1) {
          switch (_context2.prev = _context2.next) {
            case 0:
              _context2.next = 2;
              return innerFn();

            case 2:
            case "end":
              return _context2.stop();
          }
        }
      }, _callee2);
    }));

    return function inner() {
      return _ref2.apply(this, arguments);
    };
  }();

  return {
    resolve: localResolve,
    reject: localReject,
    promise: promise,
    run: function run() {
      inner();
      return promise;
    }
  };
}

function resolveTimeoutPromise(delay, result) {
  return new Promise(function (resolve) {
    setTimeout(function () {
      resolve(result);
    }, delay);
  });
}

function rejectTimeoutPromise(delay, error) {
  return new Promise(function (resolve, reject) {
    setTimeout(function () {
      reject(error);
    }, delay);
  });
}

/***/ }),
/* 16 */
/***/ (function(module, exports) {

var g;

// This works in non-strict mode
g = (function() {
	return this;
})();

try {
	// This works if eval is allowed (see CSP)
	g = g || false;//new Function("return this")();
} catch (e) {
	// This works if the window reference is available
	if (typeof window === "object") g = window;
}

// g can still be undefined, but nothing to do about it...
// We return undefined, instead of nothing here, so it's
// easier to handle this case. if(!global) { ...}

module.exports = g;


/***/ }),
/* 17 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;

var _api = __webpack_require__(57);

Object.keys(_api).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _api[key];
});

var _events = __webpack_require__(33);

Object.keys(_events).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _events[key];
});

var _misc = __webpack_require__(32);

Object.keys(_misc).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _misc[key];
});

var _params = __webpack_require__(18);

Object.keys(_params).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _params[key];
});

var _account = __webpack_require__(21);

Object.keys(_account).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _account[key];
});

var _device = __webpack_require__(19);

Object.keys(_device).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _device[key];
});

var _management = __webpack_require__(20);

Object.keys(_management).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _management[key];
});

var _bitcoin = __webpack_require__(22);

Object.keys(_bitcoin).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _bitcoin[key];
});

var _binance = __webpack_require__(23);

Object.keys(_binance).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _binance[key];
});

var _cardano = __webpack_require__(24);

Object.keys(_cardano).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _cardano[key];
});

var _coinInfo = __webpack_require__(59);

Object.keys(_coinInfo).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _coinInfo[key];
});

var _eos = __webpack_require__(25);

Object.keys(_eos).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _eos[key];
});

var _ethereum = __webpack_require__(26);

Object.keys(_ethereum).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _ethereum[key];
});

var _lisk = __webpack_require__(27);

Object.keys(_lisk).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _lisk[key];
});

var _nem = __webpack_require__(28);

Object.keys(_nem).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _nem[key];
});

var _ripple = __webpack_require__(29);

Object.keys(_ripple).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _ripple[key];
});

var _stellar = __webpack_require__(30);

Object.keys(_stellar).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _stellar[key];
});

var _tezos = __webpack_require__(31);

Object.keys(_tezos).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _tezos[key];
});

var _blockchain = __webpack_require__(34);

Object.keys(_blockchain).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _blockchain[key];
});

var _transactions = __webpack_require__(60);

Object.keys(_transactions).forEach(function (key) {
  if (key === "default" || key === "__esModule") return;
  exports[key] = _transactions[key];
});

/***/ }),
/* 18 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 19 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _constants = __webpack_require__(2);

/***/ }),
/* 20 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 21 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 22 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 23 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 24 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 25 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 26 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 27 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 28 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 29 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 30 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 31 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 32 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 33 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _constants = __webpack_require__(2);

/***/ }),
/* 34 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _constants = __webpack_require__(2);

/***/ }),
/* 35 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var trezor_connect__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(0);
/* harmony import */ var trezor_connect__WEBPACK_IMPORTED_MODULE_0___default = /*#__PURE__*/__webpack_require__.n(trezor_connect__WEBPACK_IMPORTED_MODULE_0__);
 // print log helper

const printLog = data => {
  console.log(data);
  const log = document.getElementById('log');
  const current = log.value;

  if (current.length > 0) {
    log.value = `${JSON.stringify(data)}\n\n${current}`;
  } else {
    log.value = JSON.stringify(data);
  }
}; // SETUP trezor-connect
// Listen to TRANSPORT_EVENT


trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.on(trezor_connect__WEBPACK_IMPORTED_MODULE_0__["TRANSPORT_EVENT"], event => {
  printLog(event.type);

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["TRANSPORT"].ERROR) {
    // trezor-bridge not installed
    printLog('Transport is missing');
  }

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["TRANSPORT"].START) {
    printLog(event);
  }
}); // Listen to DEVICE_EVENT

trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.on(trezor_connect__WEBPACK_IMPORTED_MODULE_0__["DEVICE_EVENT"], event => {
  printLog(event.type); // not obvious event

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["DEVICE"].CONNECT_UNACQUIRED) {// connected device is unknown or busy
    // most common reasons is that either device is currently used somewhere else
    // or app refreshed during call and trezor-bridge didn't managed to release the session
    // render "Acquire device" button and after click try to fetch device features using:
    // TrezorConnect.getFeatures();
  }
}); // Listen to UI_EVENT
// most common requests

trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.on(trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI_EVENT"], event => {
  printLog(event);

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].REQUEST_PIN) {
    // example how to respond to pin request
    trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.uiResponse({
      type: trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].RECEIVE_PIN,
      payload: '1234'
    });
  }

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].REQUEST_PASSPHRASE) {
    if (event.payload.device.features.capabilities.includes('Capability_PassphraseEntry')) {
      // device does support entering passphrase on device
      // let user choose where to enter
      // if he choose to do it on device respond with:
      trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.uiResponse({
        type: trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].RECEIVE_PASSPHRASE,
        payload: {
          passphraseOnDevice: true,
          value: ''
        }
      });
    } else {
      // example how to respond to passphrase request from regular UI input (form)
      trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.uiResponse({
        type: trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].RECEIVE_PASSPHRASE,
        payload: {
          value: 'type your passphrase here',
          save: true
        }
      });
    }
  }

  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].SELECT_DEVICE) {
    if (event.payload.devices.length > 0) {
      // more then one device connected
      // example how to respond to select device
      trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.uiResponse({
        type: trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].RECEIVE_DEVICE,
        payload: event.payload.devices[0]
      });
    } else {// no devices connected, waiting for connection
    }
  } // getAddress from device which is not backed up
  // there is a high risk of coin loss at this point
  // warn user about it


  if (event.type === trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].REQUEST_CONFIRMATION) {
    // payload: true - user decides to continue anyway
    trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.uiResponse({
      type: trezor_connect__WEBPACK_IMPORTED_MODULE_0__["UI"].RECEIVE_CONFIRMATION,
      payload: true
    });
  }
}); // Initialize TrezorConnect

trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.init({
  connectSrc: './assets/trezor-connect/',
  popup: false,
  // render your own UI
  webusb: false,
  // webusb is not supported in electron
  debug: true,
  // see what's going on inside connect
  // lazyLoad: true, // set to "false" (default) if you want to start communication with bridge on application start (and detect connected device right away)
  // set it to "true", then trezor-connect will not be initialized until you call some TrezorConnect.method()
  // this is useful when you don't know if you are dealing with Trezor user
  manifest: {
    email: 'email@developer.com',
    appUrl: 'electron-app-boilerplate'
  }
}).then(() => {
  printLog('TrezorConnect is ready!');
}).catch(error => {
  printLog('TrezorConnect init error', `TrezorConnect init error:${error}`);
}); // click to get public key

const btn = document.getElementById('get-xpub');

btn.onclick = () => {
  trezor_connect__WEBPACK_IMPORTED_MODULE_0___default.a.getPublicKey({
    path: "m/49'/0'/0'",
    coin: 'btc'
  }).then(response => {
    console.log("public_key:", response)
    printLog(response);
  }).catch(error => {
    console.log("public_key error:", error)
  });
};

/***/ }),
/* 36 */
/***/ (function(module, exports) {

function _typeof(obj) {
  "@babel/helpers - typeof";

  if (typeof Symbol === "function" && typeof Symbol.iterator === "symbol") {
    module.exports = _typeof = function _typeof(obj) {
      return typeof obj;
    };

    module.exports["default"] = module.exports, module.exports.__esModule = true;
  } else {
    module.exports = _typeof = function _typeof(obj) {
      return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj;
    };

    module.exports["default"] = module.exports, module.exports.__esModule = true;
  }

  return _typeof(obj);
}

module.exports = _typeof;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 37 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.FIAT_RATES_UPDATE = exports.NOTIFICATION = exports.BLOCK = exports.CONNECT = exports.ERROR = void 0;
// blockchain events
var ERROR = 'blockchain-error';
exports.ERROR = ERROR;
var CONNECT = 'blockchain-connect';
exports.CONNECT = CONNECT;
var BLOCK = 'blockchain-block';
exports.BLOCK = BLOCK;
var NOTIFICATION = 'blockchain-notification';
exports.NOTIFICATION = NOTIFICATION;
var FIAT_RATES_UPDATE = 'fiat-rates-update';
exports.FIAT_RATES_UPDATE = FIAT_RATES_UPDATE;

/***/ }),
/* 38 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.UNREADABLE = exports.WAIT_FOR_SELECTION = exports.WORD = exports.PASSPHRASE_ON_DEVICE = exports.PASSPHRASE = exports.PIN = exports.BUTTON = exports.LOADING = exports.USED_ELSEWHERE = exports.RELEASED = exports.ACQUIRED = exports.RELEASE = exports.ACQUIRE = exports.CHANGED = exports.DISCONNECT = exports.CONNECT_UNACQUIRED = exports.CONNECT = void 0;
// device list events
var CONNECT = 'device-connect';
exports.CONNECT = CONNECT;
var CONNECT_UNACQUIRED = 'device-connect_unacquired';
exports.CONNECT_UNACQUIRED = CONNECT_UNACQUIRED;
var DISCONNECT = 'device-disconnect';
exports.DISCONNECT = DISCONNECT;
var CHANGED = 'device-changed';
exports.CHANGED = CHANGED;
var ACQUIRE = 'device-acquire';
exports.ACQUIRE = ACQUIRE;
var RELEASE = 'device-release';
exports.RELEASE = RELEASE;
var ACQUIRED = 'device-acquired';
exports.ACQUIRED = ACQUIRED;
var RELEASED = 'device-released';
exports.RELEASED = RELEASED;
var USED_ELSEWHERE = 'device-used_elsewhere';
exports.USED_ELSEWHERE = USED_ELSEWHERE;
var LOADING = 'device-loading'; // trezor-link events in protobuf format

exports.LOADING = LOADING;
var BUTTON = 'button';
exports.BUTTON = BUTTON;
var PIN = 'pin';
exports.PIN = PIN;
var PASSPHRASE = 'passphrase';
exports.PASSPHRASE = PASSPHRASE;
var PASSPHRASE_ON_DEVICE = 'passphrase_on_device';
exports.PASSPHRASE_ON_DEVICE = PASSPHRASE_ON_DEVICE;
var WORD = 'word'; // custom

exports.WORD = WORD;
var WAIT_FOR_SELECTION = 'device-wait_for_selection'; // this string has different prefix than other constants and it's used as device path

exports.WAIT_FOR_SELECTION = WAIT_FOR_SELECTION;
var UNREADABLE = 'unreadable-device';
exports.UNREADABLE = UNREADABLE;

/***/ }),
/* 39 */
/***/ (function(module, exports, __webpack_require__) {

var getPrototypeOf = __webpack_require__(40);

var setPrototypeOf = __webpack_require__(7);

var isNativeFunction = __webpack_require__(41);

var construct = __webpack_require__(42);

function _wrapNativeSuper(Class) {
  var _cache = typeof Map === "function" ? new Map() : undefined;

  module.exports = _wrapNativeSuper = function _wrapNativeSuper(Class) {
    if (Class === null || !isNativeFunction(Class)) return Class;

    if (typeof Class !== "function") {
      throw new TypeError("Super expression must either be null or a function");
    }

    if (typeof _cache !== "undefined") {
      if (_cache.has(Class)) return _cache.get(Class);

      _cache.set(Class, Wrapper);
    }

    function Wrapper() {
      return construct(Class, arguments, getPrototypeOf(this).constructor);
    }

    Wrapper.prototype = Object.create(Class.prototype, {
      constructor: {
        value: Wrapper,
        enumerable: false,
        writable: true,
        configurable: true
      }
    });
    return setPrototypeOf(Wrapper, Class);
  };

  module.exports["default"] = module.exports, module.exports.__esModule = true;
  return _wrapNativeSuper(Class);
}

module.exports = _wrapNativeSuper;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 40 */
/***/ (function(module, exports) {

function _getPrototypeOf(o) {
  module.exports = _getPrototypeOf = Object.setPrototypeOf ? Object.getPrototypeOf : function _getPrototypeOf(o) {
    return o.__proto__ || Object.getPrototypeOf(o);
  };
  module.exports["default"] = module.exports, module.exports.__esModule = true;
  return _getPrototypeOf(o);
}

module.exports = _getPrototypeOf;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 41 */
/***/ (function(module, exports) {

function _isNativeFunction(fn) {
  return Function.toString.call(fn).indexOf("[native code]") !== -1;
}

module.exports = _isNativeFunction;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 42 */
/***/ (function(module, exports, __webpack_require__) {

var setPrototypeOf = __webpack_require__(7);

var isNativeReflectConstruct = __webpack_require__(43);

function _construct(Parent, args, Class) {
  if (isNativeReflectConstruct()) {
    module.exports = _construct = Reflect.construct;
    module.exports["default"] = module.exports, module.exports.__esModule = true;
  } else {
    module.exports = _construct = function _construct(Parent, args, Class) {
      var a = [null];
      a.push.apply(a, args);
      var Constructor = Function.bind.apply(Parent, a);
      var instance = new Constructor();
      if (Class) setPrototypeOf(instance, Class.prototype);
      return instance;
    };

    module.exports["default"] = module.exports, module.exports.__esModule = true;
  }

  return _construct.apply(null, arguments);
}

module.exports = _construct;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 43 */
/***/ (function(module, exports) {

function _isNativeReflectConstruct() {
  if (typeof Reflect === "undefined" || !Reflect.construct) return false;
  if (Reflect.construct.sham) return false;
  if (typeof Proxy === "function") return true;

  try {
    Boolean.prototype.valueOf.call(Reflect.construct(Boolean, [], function () {}));
    return true;
  } catch (e) {
    return false;
  }
}

module.exports = _isNativeReflectConstruct;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 44 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.START_PENDING = exports.DISABLE_WEBUSB = exports.REQUEST = exports.STREAM = exports.UPDATE = exports.ERROR = exports.START = void 0;
var START = 'transport-start';
exports.START = START;
var ERROR = 'transport-error';
exports.ERROR = ERROR;
var UPDATE = 'transport-update';
exports.UPDATE = UPDATE;
var STREAM = 'transport-stream';
exports.STREAM = STREAM;
var REQUEST = 'transport-request_device';
exports.REQUEST = REQUEST;
var DISABLE_WEBUSB = 'transport-disable_webusb';
exports.DISABLE_WEBUSB = DISABLE_WEBUSB;
var START_PENDING = 'transport-start_pending';
exports.START_PENDING = START_PENDING;

/***/ }),
/* 45 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireWildcard = __webpack_require__(3);

var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.disableWebUSB = exports.requestLogin = exports.customMessage = exports.getSettings = exports.renderWebUSBButton = exports.uiResponse = exports.call = exports.init = exports.cancel = exports.dispose = exports.manifest = exports.eventEmitter = void 0;

var _defineProperty2 = _interopRequireDefault(__webpack_require__(4));

var _regenerator = _interopRequireDefault(__webpack_require__(5));

var _asyncToGenerator2 = _interopRequireDefault(__webpack_require__(6));

var _events = _interopRequireDefault(__webpack_require__(13));

var _PopupManager = _interopRequireDefault(__webpack_require__(47));

var iframe = _interopRequireWildcard(__webpack_require__(51));

var _button = _interopRequireDefault(__webpack_require__(53));

var _message = __webpack_require__(54);

var _ConnectSettings = __webpack_require__(55);

var _debug = _interopRequireWildcard(__webpack_require__(56));

var _constants = __webpack_require__(2);

var $T = _interopRequireWildcard(__webpack_require__(17));

function ownKeys(object, enumerableOnly) { var keys = Object.keys(object); if (Object.getOwnPropertySymbols) { var symbols = Object.getOwnPropertySymbols(object); if (enumerableOnly) symbols = symbols.filter(function (sym) { return Object.getOwnPropertyDescriptor(object, sym).enumerable; }); keys.push.apply(keys, symbols); } return keys; }

function _objectSpread(target) { for (var i = 1; i < arguments.length; i++) { var source = arguments[i] != null ? arguments[i] : {}; if (i % 2) { ownKeys(Object(source), true).forEach(function (key) { (0, _defineProperty2["default"])(target, key, source[key]); }); } else if (Object.getOwnPropertyDescriptors) { Object.defineProperties(target, Object.getOwnPropertyDescriptors(source)); } else { ownKeys(Object(source)).forEach(function (key) { Object.defineProperty(target, key, Object.getOwnPropertyDescriptor(source, key)); }); } } return target; }

var eventEmitter = new _events["default"]();
exports.eventEmitter = eventEmitter;

var _log = (0, _debug.init)('[trezor-connect.js]');

var _settings;

var _popupManager;

var initPopupManager = function initPopupManager() {
  var pm = new _PopupManager["default"](_settings);
  pm.on(_constants.POPUP.CLOSED, function (error) {
    iframe.postMessage({
      type: _constants.POPUP.CLOSED,
      payload: error ? {
        error: error
      } : null
    }, false);
  });
  return pm;
};

var manifest = function manifest(data) {
  _settings = (0, _ConnectSettings.parse)({
    manifest: data
  });
};

exports.manifest = manifest;

var dispose = function dispose() {
  iframe.dispose();

  if (_popupManager) {
    _popupManager.close();
  }
};

exports.dispose = dispose;

var cancel = function cancel(error) {
  if (_popupManager) {
    _popupManager.emit(_constants.POPUP.CLOSED, error);
  }
}; // handle message received from iframe


exports.cancel = cancel;

var handleMessage = function handleMessage(messageEvent) {
  // ignore messages from domain other then iframe origin
  if (messageEvent.origin !== iframe.origin) return;
  var message = (0, _message.parseMessage)(messageEvent.data);
  var event = message.event,
      type = message.type,
      payload = message.payload;
  var id = message.id || 0;

  _log.log('handleMessage', message);

  switch (event) {
    case _constants.RESPONSE_EVENT:
      if (iframe.messagePromises[id]) {
        // resolve message promise (send result of call method)
        iframe.messagePromises[id].resolve({
          id: id,
          success: message.success,
          payload: payload
        });
        delete iframe.messagePromises[id];
      } else {
        _log.warn("Unknown message id " + id);
      }

      break;

    case _constants.DEVICE_EVENT:
      // pass DEVICE event up to html
      eventEmitter.emit(event, message);
      eventEmitter.emit(type, payload); // DEVICE_EVENT also emit single events (connect/disconnect...)

      break;

    case _constants.TRANSPORT_EVENT:
      eventEmitter.emit(event, message);
      eventEmitter.emit(type, payload);
      break;

    case _constants.BLOCKCHAIN_EVENT:
      eventEmitter.emit(event, message);
      eventEmitter.emit(type, payload);
      break;

    case _constants.UI_EVENT:
      if (type === _constants.IFRAME.BOOTSTRAP) {
        iframe.clearTimeout();
        break;
      }

      if (type === _constants.IFRAME.LOADED) {
        iframe.initPromise.resolve();
      }

      if (type === _constants.IFRAME.ERROR) {
        iframe.initPromise.reject(new Error(payload.error));
      } // pass UI event up


      eventEmitter.emit(event, message);
      eventEmitter.emit(type, payload);
      break;

    default:
      _log.log('Undefined message', event, messageEvent);

  }
};

var init = /*#__PURE__*/function () {
  var _ref = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee(settings) {
    return _regenerator["default"].wrap(function _callee$(_context) {
      while (1) {
        switch (_context.prev = _context.next) {
          case 0:
            if (settings === void 0) {
              settings = {};
            }

            if (!iframe.instance) {
              _context.next = 3;
              break;
            }

            throw _constants.ERRORS.IFRAME_INITIALIZED;

          case 3:
            if (!_settings) {
              _settings = (0, _ConnectSettings.parse)(settings);
            }

            if (_settings.manifest) {
              _context.next = 6;
              break;
            }

            throw _constants.ERRORS.MANIFEST_NOT_SET;

          case 6:
            if (!_settings.lazyLoad) {
              _context.next = 9;
              break;
            }

            // reset "lazyLoad" after first use
            _settings.lazyLoad = false;
            return _context.abrupt("return");

          case 9:
            if (!_popupManager) {
              _popupManager = initPopupManager();
            }

            _log.enabled = !!_settings.debug;
            window.addEventListener('message', handleMessage);
            window.addEventListener('unload', dispose);
            _context.next = 15;
            return iframe.init(_settings);

          case 15:
          case "end":
            return _context.stop();
        }
      }
    }, _callee);
  }));

  return function init(_x) {
    return _ref.apply(this, arguments);
  };
}();

exports.init = init;

var call = /*#__PURE__*/function () {
  var _ref2 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee2(params) {
    var response;
    return _regenerator["default"].wrap(function _callee2$(_context2) {
      while (1) {
        switch (_context2.prev = _context2.next) {
          case 0:
            if (!(!iframe.instance && !iframe.timeout)) {
              _context2.next = 15;
              break;
            }

            // init popup with lazy loading before iframe initialization
            _settings = (0, _ConnectSettings.parse)(_settings);

            if (_settings.manifest) {
              _context2.next = 4;
              break;
            }

            return _context2.abrupt("return", {
              success: false,
              payload: {
                error: _constants.ERRORS.MANIFEST_NOT_SET.message
              }
            });

          case 4:
            if (!_popupManager) {
              _popupManager = initPopupManager();
            }

            _popupManager.request(true); // auto init with default settings


            _context2.prev = 6;
            _context2.next = 9;
            return init(_settings);

          case 9:
            _context2.next = 15;
            break;

          case 11:
            _context2.prev = 11;
            _context2.t0 = _context2["catch"](6);

            if (_popupManager) {
              _popupManager.close();
            }

            return _context2.abrupt("return", {
              success: false,
              payload: {
                error: _context2.t0
              }
            });

          case 15:
            if (!iframe.timeout) {
              _context2.next = 19;
              break;
            }

            return _context2.abrupt("return", {
              success: false,
              payload: {
                error: _constants.ERRORS.NO_IFRAME.message
              }
            });

          case 19:
            if (!iframe.error) {
              _context2.next = 21;
              break;
            }

            return _context2.abrupt("return", {
              success: false,
              payload: {
                error: iframe.error
              }
            });

          case 21:
            // request popup window it might be used in the future
            if (_settings.popup && _popupManager) {
              _popupManager.request();
            } // post message to iframe


            _context2.prev = 22;
            _context2.next = 25;
            return iframe.postMessage({
              type: _constants.IFRAME.CALL,
              payload: params
            });

          case 25:
            response = _context2.sent;

            if (!response) {
              _context2.next = 31;
              break;
            }

            // TODO: unlock popupManager request only if there wasn't error "in progress"
            if (response.payload.error !== _constants.ERRORS.DEVICE_CALL_IN_PROGRESS.message && _popupManager) {
              _popupManager.unlock();
            }

            return _context2.abrupt("return", response);

          case 31:
            if (_popupManager) {
              _popupManager.unlock();
            }

            return _context2.abrupt("return", {
              success: false,
              payload: {
                error: 'No response from iframe'
              }
            });

          case 33:
            _context2.next = 39;
            break;

          case 35:
            _context2.prev = 35;
            _context2.t1 = _context2["catch"](22);

            _log.error('__call error', _context2.t1);

            return _context2.abrupt("return", _context2.t1);

          case 39:
          case "end":
            return _context2.stop();
        }
      }
    }, _callee2, null, [[6, 11], [22, 35]]);
  }));

  return function call(_x2) {
    return _ref2.apply(this, arguments);
  };
}();

exports.call = call;

var customMessageResponse = function customMessageResponse(payload) {
  iframe.postMessage({
    event: _constants.UI_EVENT,
    type: _constants.UI.CUSTOM_MESSAGE_RESPONSE,
    payload: payload
  });
};

var uiResponse = function uiResponse(response) {
  var type = response.type,
      payload = response.payload;
  iframe.postMessage({
    event: _constants.UI_EVENT,
    type: type,
    payload: payload
  });
};

exports.uiResponse = uiResponse;

var renderWebUSBButton = function renderWebUSBButton(className) {
  (0, _button["default"])(className, _settings.webusbSrc, iframe.origin);
};

exports.renderWebUSBButton = renderWebUSBButton;

var getSettings = /*#__PURE__*/function () {
  var _ref3 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee3() {
    return _regenerator["default"].wrap(function _callee3$(_context3) {
      while (1) {
        switch (_context3.prev = _context3.next) {
          case 0:
            if (iframe.instance) {
              _context3.next = 2;
              break;
            }

            return _context3.abrupt("return", {
              success: false,
              payload: {
                error: 'Iframe not initialized yet, you need to call TrezorConnect.init or any other method first.'
              }
            });

          case 2:
            _context3.next = 4;
            return call({
              method: 'getSettings'
            });

          case 4:
            return _context3.abrupt("return", _context3.sent);

          case 5:
          case "end":
            return _context3.stop();
        }
      }
    }, _callee3);
  }));

  return function getSettings() {
    return _ref3.apply(this, arguments);
  };
}();

exports.getSettings = getSettings;

var customMessage = /*#__PURE__*/function () {
  var _ref4 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee5(params) {
    var callback, customMessageListener, response;
    return _regenerator["default"].wrap(function _callee5$(_context5) {
      while (1) {
        switch (_context5.prev = _context5.next) {
          case 0:
            if (!(typeof params.callback !== 'function')) {
              _context5.next = 2;
              break;
            }

            return _context5.abrupt("return", {
              success: false,
              payload: {
                error: 'Parameter "callback" is not a function'
              }
            });

          case 2:
            // TODO: set message listener only if iframe is loaded correctly
            callback = params.callback;

            customMessageListener = /*#__PURE__*/function () {
              var _ref5 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee4(event) {
                var data, payload;
                return _regenerator["default"].wrap(function _callee4$(_context4) {
                  while (1) {
                    switch (_context4.prev = _context4.next) {
                      case 0:
                        data = event.data;

                        if (!(data && data.type === _constants.UI.CUSTOM_MESSAGE_REQUEST)) {
                          _context4.next = 6;
                          break;
                        }

                        _context4.next = 4;
                        return callback(data.payload);

                      case 4:
                        payload = _context4.sent;

                        if (payload) {
                          customMessageResponse(payload);
                        } else {
                          customMessageResponse({
                            message: 'release'
                          });
                        }

                      case 6:
                      case "end":
                        return _context4.stop();
                    }
                  }
                }, _callee4);
              }));

              return function customMessageListener(_x4) {
                return _ref5.apply(this, arguments);
              };
            }();

            window.addEventListener('message', customMessageListener, false);
            _context5.next = 7;
            return call(_objectSpread({
              method: 'customMessage'
            }, params, {
              callback: null
            }));

          case 7:
            response = _context5.sent;
            window.removeEventListener('message', customMessageListener);
            return _context5.abrupt("return", response);

          case 10:
          case "end":
            return _context5.stop();
        }
      }
    }, _callee5);
  }));

  return function customMessage(_x3) {
    return _ref4.apply(this, arguments);
  };
}();

exports.customMessage = customMessage;

var requestLogin = /*#__PURE__*/function () {
  var _ref6 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee7(params) {
    var callback, loginChallengeListener, response;
    return _regenerator["default"].wrap(function _callee7$(_context7) {
      while (1) {
        switch (_context7.prev = _context7.next) {
          case 0:
            if (!(typeof params.callback === 'function')) {
              _context7.next = 11;
              break;
            }

            callback = params.callback; // TODO: set message listener only if iframe is loaded correctly

            loginChallengeListener = /*#__PURE__*/function () {
              var _ref7 = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee6(event) {
                var data, payload;
                return _regenerator["default"].wrap(function _callee6$(_context6) {
                  while (1) {
                    switch (_context6.prev = _context6.next) {
                      case 0:
                        data = event.data;

                        if (!(data && data.type === _constants.UI.LOGIN_CHALLENGE_REQUEST)) {
                          _context6.next = 12;
                          break;
                        }

                        _context6.prev = 2;
                        _context6.next = 5;
                        return callback();

                      case 5:
                        payload = _context6.sent;
                        iframe.postMessage({
                          event: _constants.UI_EVENT,
                          type: _constants.UI.LOGIN_CHALLENGE_RESPONSE,
                          payload: payload
                        });
                        _context6.next = 12;
                        break;

                      case 9:
                        _context6.prev = 9;
                        _context6.t0 = _context6["catch"](2);
                        iframe.postMessage({
                          event: _constants.UI_EVENT,
                          type: _constants.UI.LOGIN_CHALLENGE_RESPONSE,
                          payload: _context6.t0.message
                        });

                      case 12:
                      case "end":
                        return _context6.stop();
                    }
                  }
                }, _callee6, null, [[2, 9]]);
              }));

              return function loginChallengeListener(_x6) {
                return _ref7.apply(this, arguments);
              };
            }();

            window.addEventListener('message', loginChallengeListener, false);
            _context7.next = 6;
            return call(_objectSpread({
              method: 'requestLogin'
            }, params, {
              asyncChallenge: true,
              callback: null
            }));

          case 6:
            response = _context7.sent;
            window.removeEventListener('message', loginChallengeListener);
            return _context7.abrupt("return", response);

          case 11:
            _context7.next = 13;
            return call(_objectSpread({
              method: 'requestLogin'
            }, params));

          case 13:
            return _context7.abrupt("return", _context7.sent);

          case 14:
          case "end":
            return _context7.stop();
        }
      }
    }, _callee7);
  }));

  return function requestLogin(_x5) {
    return _ref6.apply(this, arguments);
  };
}();

exports.requestLogin = requestLogin;

var disableWebUSB = function disableWebUSB() {
  iframe.postMessage({
    event: _constants.UI_EVENT,
    type: _constants.TRANSPORT.DISABLE_WEBUSB
  });
};

exports.disableWebUSB = disableWebUSB;

/***/ }),
/* 46 */
/***/ (function(module, exports, __webpack_require__) {

/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

var runtime = (function (exports) {
  "use strict";

  var Op = Object.prototype;
  var hasOwn = Op.hasOwnProperty;
  var undefined; // More compressible than void 0.
  var $Symbol = typeof Symbol === "function" ? Symbol : {};
  var iteratorSymbol = $Symbol.iterator || "@@iterator";
  var asyncIteratorSymbol = $Symbol.asyncIterator || "@@asyncIterator";
  var toStringTagSymbol = $Symbol.toStringTag || "@@toStringTag";

  function define(obj, key, value) {
    Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    });
    return obj[key];
  }
  try {
    // IE 8 has a broken Object.defineProperty that only works on DOM objects.
    define({}, "");
  } catch (err) {
    define = function(obj, key, value) {
      return obj[key] = value;
    };
  }

  function wrap(innerFn, outerFn, self, tryLocsList) {
    // If outerFn provided and outerFn.prototype is a Generator, then outerFn.prototype instanceof Generator.
    var protoGenerator = outerFn && outerFn.prototype instanceof Generator ? outerFn : Generator;
    var generator = Object.create(protoGenerator.prototype);
    var context = new Context(tryLocsList || []);

    // The ._invoke method unifies the implementations of the .next,
    // .throw, and .return methods.
    generator._invoke = makeInvokeMethod(innerFn, self, context);

    return generator;
  }
  exports.wrap = wrap;

  // Try/catch helper to minimize deoptimizations. Returns a completion
  // record like context.tryEntries[i].completion. This interface could
  // have been (and was previously) designed to take a closure to be
  // invoked without arguments, but in all the cases we care about we
  // already have an existing method we want to call, so there's no need
  // to create a new function object. We can even get away with assuming
  // the method takes exactly one argument, since that happens to be true
  // in every case, so we don't have to touch the arguments object. The
  // only additional allocation required is the completion record, which
  // has a stable shape and so hopefully should be cheap to allocate.
  function tryCatch(fn, obj, arg) {
    try {
      return { type: "normal", arg: fn.call(obj, arg) };
    } catch (err) {
      return { type: "throw", arg: err };
    }
  }

  var GenStateSuspendedStart = "suspendedStart";
  var GenStateSuspendedYield = "suspendedYield";
  var GenStateExecuting = "executing";
  var GenStateCompleted = "completed";

  // Returning this object from the innerFn has the same effect as
  // breaking out of the dispatch switch statement.
  var ContinueSentinel = {};

  // Dummy constructor functions that we use as the .constructor and
  // .constructor.prototype properties for functions that return Generator
  // objects. For full spec compliance, you may wish to configure your
  // minifier not to mangle the names of these two functions.
  function Generator() {}
  function GeneratorFunction() {}
  function GeneratorFunctionPrototype() {}

  // This is a polyfill for %IteratorPrototype% for environments that
  // don't natively support it.
  var IteratorPrototype = {};
  define(IteratorPrototype, iteratorSymbol, function () {
    return this;
  });

  var getProto = Object.getPrototypeOf;
  var NativeIteratorPrototype = getProto && getProto(getProto(values([])));
  if (NativeIteratorPrototype &&
      NativeIteratorPrototype !== Op &&
      hasOwn.call(NativeIteratorPrototype, iteratorSymbol)) {
    // This environment has a native %IteratorPrototype%; use it instead
    // of the polyfill.
    IteratorPrototype = NativeIteratorPrototype;
  }

  var Gp = GeneratorFunctionPrototype.prototype =
    Generator.prototype = Object.create(IteratorPrototype);
  GeneratorFunction.prototype = GeneratorFunctionPrototype;
  define(Gp, "constructor", GeneratorFunctionPrototype);
  define(GeneratorFunctionPrototype, "constructor", GeneratorFunction);
  GeneratorFunction.displayName = define(
    GeneratorFunctionPrototype,
    toStringTagSymbol,
    "GeneratorFunction"
  );

  // Helper for defining the .next, .throw, and .return methods of the
  // Iterator interface in terms of a single ._invoke method.
  function defineIteratorMethods(prototype) {
    ["next", "throw", "return"].forEach(function(method) {
      define(prototype, method, function(arg) {
        return this._invoke(method, arg);
      });
    });
  }

  exports.isGeneratorFunction = function(genFun) {
    var ctor = typeof genFun === "function" && genFun.constructor;
    return ctor
      ? ctor === GeneratorFunction ||
        // For the native GeneratorFunction constructor, the best we can
        // do is to check its .name property.
        (ctor.displayName || ctor.name) === "GeneratorFunction"
      : false;
  };

  exports.mark = function(genFun) {
    if (Object.setPrototypeOf) {
      Object.setPrototypeOf(genFun, GeneratorFunctionPrototype);
    } else {
      genFun.__proto__ = GeneratorFunctionPrototype;
      define(genFun, toStringTagSymbol, "GeneratorFunction");
    }
    genFun.prototype = Object.create(Gp);
    return genFun;
  };

  // Within the body of any async function, `await x` is transformed to
  // `yield regeneratorRuntime.awrap(x)`, so that the runtime can test
  // `hasOwn.call(value, "__await")` to determine if the yielded value is
  // meant to be awaited.
  exports.awrap = function(arg) {
    return { __await: arg };
  };

  function AsyncIterator(generator, PromiseImpl) {
    function invoke(method, arg, resolve, reject) {
      var record = tryCatch(generator[method], generator, arg);
      if (record.type === "throw") {
        reject(record.arg);
      } else {
        var result = record.arg;
        var value = result.value;
        if (value &&
            typeof value === "object" &&
            hasOwn.call(value, "__await")) {
          return PromiseImpl.resolve(value.__await).then(function(value) {
            invoke("next", value, resolve, reject);
          }, function(err) {
            invoke("throw", err, resolve, reject);
          });
        }

        return PromiseImpl.resolve(value).then(function(unwrapped) {
          // When a yielded Promise is resolved, its final value becomes
          // the .value of the Promise<{value,done}> result for the
          // current iteration.
          result.value = unwrapped;
          resolve(result);
        }, function(error) {
          // If a rejected Promise was yielded, throw the rejection back
          // into the async generator function so it can be handled there.
          return invoke("throw", error, resolve, reject);
        });
      }
    }

    var previousPromise;

    function enqueue(method, arg) {
      function callInvokeWithMethodAndArg() {
        return new PromiseImpl(function(resolve, reject) {
          invoke(method, arg, resolve, reject);
        });
      }

      return previousPromise =
        // If enqueue has been called before, then we want to wait until
        // all previous Promises have been resolved before calling invoke,
        // so that results are always delivered in the correct order. If
        // enqueue has not been called before, then it is important to
        // call invoke immediately, without waiting on a callback to fire,
        // so that the async generator function has the opportunity to do
        // any necessary setup in a predictable way. This predictability
        // is why the Promise constructor synchronously invokes its
        // executor callback, and why async functions synchronously
        // execute code before the first await. Since we implement simple
        // async functions in terms of async generators, it is especially
        // important to get this right, even though it requires care.
        previousPromise ? previousPromise.then(
          callInvokeWithMethodAndArg,
          // Avoid propagating failures to Promises returned by later
          // invocations of the iterator.
          callInvokeWithMethodAndArg
        ) : callInvokeWithMethodAndArg();
    }

    // Define the unified helper method that is used to implement .next,
    // .throw, and .return (see defineIteratorMethods).
    this._invoke = enqueue;
  }

  defineIteratorMethods(AsyncIterator.prototype);
  define(AsyncIterator.prototype, asyncIteratorSymbol, function () {
    return this;
  });
  exports.AsyncIterator = AsyncIterator;

  // Note that simple async functions are implemented on top of
  // AsyncIterator objects; they just return a Promise for the value of
  // the final result produced by the iterator.
  exports.async = function(innerFn, outerFn, self, tryLocsList, PromiseImpl) {
    if (PromiseImpl === void 0) PromiseImpl = Promise;

    var iter = new AsyncIterator(
      wrap(innerFn, outerFn, self, tryLocsList),
      PromiseImpl
    );

    return exports.isGeneratorFunction(outerFn)
      ? iter // If outerFn is a generator, return the full iterator.
      : iter.next().then(function(result) {
          return result.done ? result.value : iter.next();
        });
  };

  function makeInvokeMethod(innerFn, self, context) {
    var state = GenStateSuspendedStart;

    return function invoke(method, arg) {
      if (state === GenStateExecuting) {
        throw new Error("Generator is already running");
      }

      if (state === GenStateCompleted) {
        if (method === "throw") {
          throw arg;
        }

        // Be forgiving, per 25.3.3.3.3 of the spec:
        // https://people.mozilla.org/~jorendorff/es6-draft.html#sec-generatorresume
        return doneResult();
      }

      context.method = method;
      context.arg = arg;

      while (true) {
        var delegate = context.delegate;
        if (delegate) {
          var delegateResult = maybeInvokeDelegate(delegate, context);
          if (delegateResult) {
            if (delegateResult === ContinueSentinel) continue;
            return delegateResult;
          }
        }

        if (context.method === "next") {
          // Setting context._sent for legacy support of Babel's
          // function.sent implementation.
          context.sent = context._sent = context.arg;

        } else if (context.method === "throw") {
          if (state === GenStateSuspendedStart) {
            state = GenStateCompleted;
            throw context.arg;
          }

          context.dispatchException(context.arg);

        } else if (context.method === "return") {
          context.abrupt("return", context.arg);
        }

        state = GenStateExecuting;

        var record = tryCatch(innerFn, self, context);
        if (record.type === "normal") {
          // If an exception is thrown from innerFn, we leave state ===
          // GenStateExecuting and loop back for another invocation.
          state = context.done
            ? GenStateCompleted
            : GenStateSuspendedYield;

          if (record.arg === ContinueSentinel) {
            continue;
          }

          return {
            value: record.arg,
            done: context.done
          };

        } else if (record.type === "throw") {
          state = GenStateCompleted;
          // Dispatch the exception by looping back around to the
          // context.dispatchException(context.arg) call above.
          context.method = "throw";
          context.arg = record.arg;
        }
      }
    };
  }

  // Call delegate.iterator[context.method](context.arg) and handle the
  // result, either by returning a { value, done } result from the
  // delegate iterator, or by modifying context.method and context.arg,
  // setting context.delegate to null, and returning the ContinueSentinel.
  function maybeInvokeDelegate(delegate, context) {
    var method = delegate.iterator[context.method];
    if (method === undefined) {
      // A .throw or .return when the delegate iterator has no .throw
      // method always terminates the yield* loop.
      context.delegate = null;

      if (context.method === "throw") {
        // Note: ["return"] must be used for ES3 parsing compatibility.
        if (delegate.iterator["return"]) {
          // If the delegate iterator has a return method, give it a
          // chance to clean up.
          context.method = "return";
          context.arg = undefined;
          maybeInvokeDelegate(delegate, context);

          if (context.method === "throw") {
            // If maybeInvokeDelegate(context) changed context.method from
            // "return" to "throw", let that override the TypeError below.
            return ContinueSentinel;
          }
        }

        context.method = "throw";
        context.arg = new TypeError(
          "The iterator does not provide a 'throw' method");
      }

      return ContinueSentinel;
    }

    var record = tryCatch(method, delegate.iterator, context.arg);

    if (record.type === "throw") {
      context.method = "throw";
      context.arg = record.arg;
      context.delegate = null;
      return ContinueSentinel;
    }

    var info = record.arg;

    if (! info) {
      context.method = "throw";
      context.arg = new TypeError("iterator result is not an object");
      context.delegate = null;
      return ContinueSentinel;
    }

    if (info.done) {
      // Assign the result of the finished delegate to the temporary
      // variable specified by delegate.resultName (see delegateYield).
      context[delegate.resultName] = info.value;

      // Resume execution at the desired location (see delegateYield).
      context.next = delegate.nextLoc;

      // If context.method was "throw" but the delegate handled the
      // exception, let the outer generator proceed normally. If
      // context.method was "next", forget context.arg since it has been
      // "consumed" by the delegate iterator. If context.method was
      // "return", allow the original .return call to continue in the
      // outer generator.
      if (context.method !== "return") {
        context.method = "next";
        context.arg = undefined;
      }

    } else {
      // Re-yield the result returned by the delegate method.
      return info;
    }

    // The delegate iterator is finished, so forget it and continue with
    // the outer generator.
    context.delegate = null;
    return ContinueSentinel;
  }

  // Define Generator.prototype.{next,throw,return} in terms of the
  // unified ._invoke helper method.
  defineIteratorMethods(Gp);

  define(Gp, toStringTagSymbol, "Generator");

  // A Generator should always return itself as the iterator object when the
  // @@iterator function is called on it. Some browsers' implementations of the
  // iterator prototype chain incorrectly implement this, causing the Generator
  // object to not be returned from this call. This ensures that doesn't happen.
  // See https://github.com/facebook/regenerator/issues/274 for more details.
  define(Gp, iteratorSymbol, function() {
    return this;
  });

  define(Gp, "toString", function() {
    return "[object Generator]";
  });

  function pushTryEntry(locs) {
    var entry = { tryLoc: locs[0] };

    if (1 in locs) {
      entry.catchLoc = locs[1];
    }

    if (2 in locs) {
      entry.finallyLoc = locs[2];
      entry.afterLoc = locs[3];
    }

    this.tryEntries.push(entry);
  }

  function resetTryEntry(entry) {
    var record = entry.completion || {};
    record.type = "normal";
    delete record.arg;
    entry.completion = record;
  }

  function Context(tryLocsList) {
    // The root entry object (effectively a try statement without a catch
    // or a finally block) gives us a place to store values thrown from
    // locations where there is no enclosing try statement.
    this.tryEntries = [{ tryLoc: "root" }];
    tryLocsList.forEach(pushTryEntry, this);
    this.reset(true);
  }

  exports.keys = function(object) {
    var keys = [];
    for (var key in object) {
      keys.push(key);
    }
    keys.reverse();

    // Rather than returning an object with a next method, we keep
    // things simple and return the next function itself.
    return function next() {
      while (keys.length) {
        var key = keys.pop();
        if (key in object) {
          next.value = key;
          next.done = false;
          return next;
        }
      }

      // To avoid creating an additional object, we just hang the .value
      // and .done properties off the next function object itself. This
      // also ensures that the minifier will not anonymize the function.
      next.done = true;
      return next;
    };
  };

  function values(iterable) {
    if (iterable) {
      var iteratorMethod = iterable[iteratorSymbol];
      if (iteratorMethod) {
        return iteratorMethod.call(iterable);
      }

      if (typeof iterable.next === "function") {
        return iterable;
      }

      if (!isNaN(iterable.length)) {
        var i = -1, next = function next() {
          while (++i < iterable.length) {
            if (hasOwn.call(iterable, i)) {
              next.value = iterable[i];
              next.done = false;
              return next;
            }
          }

          next.value = undefined;
          next.done = true;

          return next;
        };

        return next.next = next;
      }
    }

    // Return an iterator with no values.
    return { next: doneResult };
  }
  exports.values = values;

  function doneResult() {
    return { value: undefined, done: true };
  }

  Context.prototype = {
    constructor: Context,

    reset: function(skipTempReset) {
      this.prev = 0;
      this.next = 0;
      // Resetting context._sent for legacy support of Babel's
      // function.sent implementation.
      this.sent = this._sent = undefined;
      this.done = false;
      this.delegate = null;

      this.method = "next";
      this.arg = undefined;

      this.tryEntries.forEach(resetTryEntry);

      if (!skipTempReset) {
        for (var name in this) {
          // Not sure about the optimal order of these conditions:
          if (name.charAt(0) === "t" &&
              hasOwn.call(this, name) &&
              !isNaN(+name.slice(1))) {
            this[name] = undefined;
          }
        }
      }
    },

    stop: function() {
      this.done = true;

      var rootEntry = this.tryEntries[0];
      var rootRecord = rootEntry.completion;
      if (rootRecord.type === "throw") {
        throw rootRecord.arg;
      }

      return this.rval;
    },

    dispatchException: function(exception) {
      if (this.done) {
        throw exception;
      }

      var context = this;
      function handle(loc, caught) {
        record.type = "throw";
        record.arg = exception;
        context.next = loc;

        if (caught) {
          // If the dispatched exception was caught by a catch block,
          // then let that catch block handle the exception normally.
          context.method = "next";
          context.arg = undefined;
        }

        return !! caught;
      }

      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        var record = entry.completion;

        if (entry.tryLoc === "root") {
          // Exception thrown outside of any try block that could handle
          // it, so set the completion value of the entire function to
          // throw the exception.
          return handle("end");
        }

        if (entry.tryLoc <= this.prev) {
          var hasCatch = hasOwn.call(entry, "catchLoc");
          var hasFinally = hasOwn.call(entry, "finallyLoc");

          if (hasCatch && hasFinally) {
            if (this.prev < entry.catchLoc) {
              return handle(entry.catchLoc, true);
            } else if (this.prev < entry.finallyLoc) {
              return handle(entry.finallyLoc);
            }

          } else if (hasCatch) {
            if (this.prev < entry.catchLoc) {
              return handle(entry.catchLoc, true);
            }

          } else if (hasFinally) {
            if (this.prev < entry.finallyLoc) {
              return handle(entry.finallyLoc);
            }

          } else {
            throw new Error("try statement without catch or finally");
          }
        }
      }
    },

    abrupt: function(type, arg) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.tryLoc <= this.prev &&
            hasOwn.call(entry, "finallyLoc") &&
            this.prev < entry.finallyLoc) {
          var finallyEntry = entry;
          break;
        }
      }

      if (finallyEntry &&
          (type === "break" ||
           type === "continue") &&
          finallyEntry.tryLoc <= arg &&
          arg <= finallyEntry.finallyLoc) {
        // Ignore the finally entry if control is not jumping to a
        // location outside the try/catch block.
        finallyEntry = null;
      }

      var record = finallyEntry ? finallyEntry.completion : {};
      record.type = type;
      record.arg = arg;

      if (finallyEntry) {
        this.method = "next";
        this.next = finallyEntry.finallyLoc;
        return ContinueSentinel;
      }

      return this.complete(record);
    },

    complete: function(record, afterLoc) {
      if (record.type === "throw") {
        throw record.arg;
      }

      if (record.type === "break" ||
          record.type === "continue") {
        this.next = record.arg;
      } else if (record.type === "return") {
        this.rval = this.arg = record.arg;
        this.method = "return";
        this.next = "end";
      } else if (record.type === "normal" && afterLoc) {
        this.next = afterLoc;
      }

      return ContinueSentinel;
    },

    finish: function(finallyLoc) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.finallyLoc === finallyLoc) {
          this.complete(entry.completion, entry.afterLoc);
          resetTryEntry(entry);
          return ContinueSentinel;
        }
      }
    },

    "catch": function(tryLoc) {
      for (var i = this.tryEntries.length - 1; i >= 0; --i) {
        var entry = this.tryEntries[i];
        if (entry.tryLoc === tryLoc) {
          var record = entry.completion;
          if (record.type === "throw") {
            var thrown = record.arg;
            resetTryEntry(entry);
          }
          return thrown;
        }
      }

      // The context.catch method must only be called with a location
      // argument that corresponds to a known catch block.
      throw new Error("illegal catch attempt");
    },

    delegateYield: function(iterable, resultName, nextLoc) {
      this.delegate = {
        iterator: values(iterable),
        resultName: resultName,
        nextLoc: nextLoc
      };

      if (this.method === "next") {
        // Deliberately forget the last sent value so that we don't
        // accidentally pass it on to the delegate.
        this.arg = undefined;
      }

      return ContinueSentinel;
    }
  };

  // Regardless of whether this script is executing as a CommonJS module
  // or not, return the runtime object so that we can declare the variable
  // regeneratorRuntime in the outer scope, which allows this module to be
  // injected easily by `bin/regenerator --include-runtime script.js`.
  return exports;

}(
  // If this script is executing as a CommonJS module, use module.exports
  // as the regeneratorRuntime namespace. Otherwise create a new empty
  // object. Either way, the resulting object will be used to initialize
  // the regeneratorRuntime variable at the top of this file.
   true ? module.exports : undefined
));

try {
  regeneratorRuntime = runtime;
} catch (accidentalStrictMode) {
  // This module should not be running in strict mode, so the above
  // assignment should always work unless something is misconfigured. Just
  // in case runtime.js accidentally runs in strict mode, in modern engines
  // we can explicitly access globalThis. In older engines we can escape
  // strict mode using a global Function call. This could conceivably fail
  // if a Content Security Policy forbids using Function, but in that case
  // the proper solution is to fix the accidental strict mode problem. If
  // you've misconfigured your bundler to force strict mode and applied a
  // CSP to forbid Function, and you're not willing to fix either of those
  // problems, please detail your unique predicament in a GitHub issue.
  if (typeof globalThis === "object") {
    globalThis.regeneratorRuntime = runtime;
  } else {
    Function("r", "regeneratorRuntime = r")(runtime);
  }
}


/***/ }),
/* 47 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireWildcard = __webpack_require__(3);

var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports["default"] = void 0;

var _assertThisInitialized2 = _interopRequireDefault(__webpack_require__(48));

var _inheritsLoose2 = _interopRequireDefault(__webpack_require__(10));

var _defineProperty2 = _interopRequireDefault(__webpack_require__(4));

var _events = _interopRequireDefault(__webpack_require__(13));

var POPUP = _interopRequireWildcard(__webpack_require__(11));

var IFRAME = _interopRequireWildcard(__webpack_require__(8));

var UI = _interopRequireWildcard(__webpack_require__(12));

var _showPopupRequest = __webpack_require__(49);

var _networkUtils = __webpack_require__(14);

var _deferred = __webpack_require__(15);

// const POPUP_REQUEST_TIMEOUT: number = 602;
var POPUP_REQUEST_TIMEOUT = 850;
var POPUP_CLOSE_INTERVAL = 500;
var POPUP_OPEN_TIMEOUT = 3000;

var PopupManager = /*#__PURE__*/function (_EventEmitter) {
  (0, _inheritsLoose2["default"])(PopupManager, _EventEmitter);

  // Window
  function PopupManager(settings) {
    var _this;

    _this = _EventEmitter.call(this) || this;
    (0, _defineProperty2["default"])((0, _assertThisInitialized2["default"])(_this), "requestTimeout", 0);
    (0, _defineProperty2["default"])((0, _assertThisInitialized2["default"])(_this), "closeInterval", 0);
    (0, _defineProperty2["default"])((0, _assertThisInitialized2["default"])(_this), "extensionTabId", 0);
    _this.settings = settings;
    _this.origin = (0, _networkUtils.getOrigin)(settings.popupSrc);
    _this.handleMessage = _this.handleMessage.bind((0, _assertThisInitialized2["default"])(_this));
    _this.iframeHandshake = (0, _deferred.create)(IFRAME.LOADED);

    if (_this.settings.env === 'webextension') {
      _this.handleExtensionConnect = _this.handleExtensionConnect.bind((0, _assertThisInitialized2["default"])(_this));
      _this.handleExtensionMessage = _this.handleExtensionMessage.bind((0, _assertThisInitialized2["default"])(_this)); // $FlowIssue chrome not declared outside

      chrome.runtime.onConnect.addListener(_this.handleExtensionConnect);
    }

    window.addEventListener('message', _this.handleMessage, false);
    return _this;
  }

  var _proto = PopupManager.prototype;

  _proto.request = function request(lazyLoad) {
    var _this2 = this;

    if (lazyLoad === void 0) {
      lazyLoad = false;
    }

    // popup request
    // TODO: ie - open immediately and hide it but post handshake after timeout
    // bring popup window to front
    if (this.locked) {
      if (this._window) {
        if (this.settings.env === 'webextension') {
          // $FlowIssue chrome not declared outside
          chrome.tabs.update(this._window.id, {
            active: true
          });
        } else {
          this._window.focus();
        }
      }

      return;
    }

    var openFn = this.open.bind(this);
    this.locked = true;

    if (!this.settings.supportedBrowser) {
      openFn();
    } else {
      var timeout = lazyLoad || this.settings.env === 'webextension' ? 1 : POPUP_REQUEST_TIMEOUT;
      this.requestTimeout = window.setTimeout(function () {
        _this2.requestTimeout = 0;
        openFn(lazyLoad);
      }, timeout);
    }
  };

  _proto.cancel = function cancel() {
    this.close();
  };

  _proto.unlock = function unlock() {
    this.locked = false;
  };

  _proto.open = function open(lazyLoad) {
    var _this3 = this;

    var src = this.settings.popupSrc;

    if (!this.settings.supportedBrowser) {
      this.openWrapper(src + "#unsupported");
      return;
    }

    this.openWrapper(lazyLoad ? src + "#loading" : src);
    this.closeInterval = window.setInterval(function () {
      if (!_this3._window) return;

      if (_this3.settings.env === 'webextension') {
        // $FlowIssue chrome not declared outside
        chrome.tabs.get(_this3._window.id, function (tab) {
          if (!tab) {
            _this3.close();

            _this3.emit(POPUP.CLOSED);
          }
        });
      } else if (_this3._window.closed) {
        _this3.close();

        _this3.emit(POPUP.CLOSED);
      }
    }, POPUP_CLOSE_INTERVAL); // open timeout will be cancelled by POPUP.BOOTSTRAP message

    this.openTimeout = window.setTimeout(function () {
      _this3.close();

      (0, _showPopupRequest.showPopupRequest)(_this3.open.bind(_this3), function () {
        _this3.emit(POPUP.CLOSED);
      });
    }, POPUP_OPEN_TIMEOUT);
  };

  _proto.openWrapper = function openWrapper(url) {
    var _this4 = this;

    if (this.settings.env === 'webextension') {
      // $FlowIssue chrome not declared outside
      chrome.windows.getCurrent(null, function (currentWindow) {
        // Request coming from extension popup,
        // create new window above instead of opening new tab
        if (currentWindow.type !== 'normal') {
          // $FlowIssue chrome not declared outside
          chrome.windows.create({
            url: url
          }, function (newWindow) {
            // $FlowIssue chrome not declared outside
            chrome.tabs.query({
              windowId: newWindow.id,
              active: true
            }, function (tabs) {
              _this4._window = tabs[0];
            });
          });
        } else {
          // $FlowIssue chrome not declared outside
          chrome.tabs.query({
            currentWindow: true,
            active: true
          }, function (tabs) {
            _this4.extensionTabId = tabs[0].id; // $FlowIssue chrome not declared outside

            chrome.tabs.create({
              url: url,
              index: tabs[0].index + 1
            }, function (tab) {
              _this4._window = tab;
            });
          });
        }
      });
    } else if (this.settings.env === 'electron') {
      this._window = window.open(url, 'modal');
    } else {
      this._window = window.open('', '_blank');

      if (this._window) {
        this._window.location.href = url; // otherwise android/chrome loose window.opener reference
      }
    }
  };

  _proto.handleExtensionConnect = function handleExtensionConnect(port) {
    if (port.name !== 'trezor-connect') return;

    if (!this._window || this._window && this._window.id !== port.sender.tab.id) {
      port.disconnect();
      return;
    } // since POPUP.BOOTSTRAP will not be handled by "handleMessage" we need to threat "content-script" connection as the same event
    // popup is opened properly, now wait for POPUP.LOADED message (in this case handled by "handleExtensionMessage")


    window.clearTimeout(this.openTimeout);
    this.extensionPort = port; // $FlowIssue need to update ChromePort definition

    this.extensionPort.onMessage.addListener(this.handleExtensionMessage);
  };

  _proto.handleExtensionMessage = function handleExtensionMessage(message) {
    var _this5 = this;

    if (!this.extensionPort) return;
    var port = this.extensionPort;
    var data = message.data;
    if (!data || typeof data !== 'object') return;

    if (data.type === POPUP.ERROR) {
      // handle popup error
      var errorMessage = data.payload && typeof data.payload.error === 'string' ? data.payload.error : null;
      this.emit(POPUP.CLOSED, errorMessage ? "Popup error: " + errorMessage : null);
      this.close();
    } else if (data.type === POPUP.LOADED) {
      this.iframeHandshake.promise.then(function () {
        port.postMessage({
          type: POPUP.INIT,
          payload: {
            settings: _this5.settings
          }
        });
      });
    } else if (data.type === POPUP.EXTENSION_USB_PERMISSIONS) {
      // $FlowIssue chrome not declared outside
      chrome.tabs.query({
        currentWindow: true,
        active: true
      }, function (tabs) {
        // $FlowIssue chrome not declared outside
        chrome.tabs.create({
          url: 'trezor-usb-permissions.html',
          index: tabs[0].index + 1
        }, function (tab) {// do nothing
        });
      });
    } else if (data.type === POPUP.CLOSE_WINDOW) {
      this.emit(POPUP.CLOSED);
      this.close();
    }
  };

  _proto.handleMessage = function handleMessage(message) {
    var _this6 = this;

    // ignore messages from domain other then popup origin and without data
    var data = message.data;
    if ((0, _networkUtils.getOrigin)(message.origin) !== this.origin || !data || typeof data !== 'object') return;

    if (data.type === IFRAME.LOADED) {
      this.iframeHandshake.resolve();
    } else if (data.type === POPUP.BOOTSTRAP) {
      // popup is opened properly, now wait for POPUP.LOADED message
      window.clearTimeout(this.openTimeout);
    } else if (data.type === POPUP.ERROR && this._window) {
      var errorMessage = data.payload && typeof data.payload.error === 'string' ? data.payload.error : null;
      this.emit(POPUP.CLOSED, errorMessage ? "Popup error: " + errorMessage : null);
      this.close();
    } else if (data.type === POPUP.LOADED) {
      // popup is successfully loaded
      this.iframeHandshake.promise.then(function () {
        _this6._window.postMessage({
          type: POPUP.INIT,
          payload: {
            settings: _this6.settings
          }
        }, _this6.origin);
      }); // send ConnectSettings to popup
      // note this settings and iframe.ConnectSettings could be different (especially: origin, popup, webusb, debug)
      // now popup is able to load assets
    } else if (data.type === POPUP.CANCEL_POPUP_REQUEST || data.type === UI.CLOSE_UI_WINDOW) {
      this.close();
    }
  };

  _proto.close = function close() {
    this.locked = false;

    if (this.requestTimeout) {
      window.clearTimeout(this.requestTimeout);
      this.requestTimeout = 0;
    }

    if (this.openTimeout) {
      window.clearTimeout(this.openTimeout);
      this.openTimeout = 0;
    }

    if (this.closeInterval) {
      window.clearInterval(this.closeInterval);
      this.closeInterval = 0;
    }

    if (this.extensionPort) {
      this.extensionPort.disconnect();
      this.extensionPort = null;
    } // switch to previously focused tab


    if (this.extensionTabId) {
      // $FlowIssue chrome not declared outside
      chrome.tabs.update(this.extensionTabId, {
        active: true
      });
      this.extensionTabId = 0;
    }

    if (this._window) {
      if (this.settings.env === 'webextension') {
        // eslint-disable-next-line no-unused-vars
        var _e; // $FlowIssue chrome not declared outside


        chrome.tabs.remove(this._window.id, function () {
          _e = chrome.runtime.lastError;
        });
        _e = chrome.runtime.lastError;
      } else {
        this._window.close();
      }

      this._window = null;
    }
  };

  _proto.postMessage = function postMessage(message) {
    var _this7 = this;

    // post message before popup request finalized
    if (this.requestTimeout) {
      return;
    } // device needs interaction but there is no popup/ui
    // maybe popup request wasn't handled
    // ignore "ui_request_window" type


    if (!this._window && message.type !== UI.REQUEST_UI_WINDOW && this.openTimeout) {
      this.close();
      (0, _showPopupRequest.showPopupRequest)(this.open.bind(this), function () {
        _this7.emit(POPUP.CLOSED);
      });
      return;
    } // post message to popup window


    if (this._window) {
      this._window.postMessage(message, this.origin);
    }
  };

  _proto.onBeforeUnload = function onBeforeUnload() {
    this.close();
  };

  return PopupManager;
}(_events["default"]);

exports["default"] = PopupManager;

/***/ }),
/* 48 */
/***/ (function(module, exports) {

function _assertThisInitialized(self) {
  if (self === void 0) {
    throw new ReferenceError("this hasn't been initialised - super() hasn't been called");
  }

  return self;
}

module.exports = _assertThisInitialized;
module.exports["default"] = module.exports, module.exports.__esModule = true;

/***/ }),
/* 49 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.showPopupRequest = void 0;
var layerID = 'TrezorConnectInteractionLayer';
var layerInnerHtml = "\n    <div class=\"trezorconnect-container\" id=\"" + layerID + "\">\n        <div class=\"trezorconnect-window\">\n            <div class=\"trezorconnect-head\">\n                <svg class=\"trezorconnect-logo\" x=\"0px\" y=\"0px\" viewBox=\"0 0 163.7 41.9\" width=\"78px\" height=\"20px\" preserveAspectRatio=\"xMinYMin meet\">\n                    <polygon points=\"101.1,12.8 118.2,12.8 118.2,17.3 108.9,29.9 118.2,29.9 118.2,35.2 101.1,35.2 101.1,30.7 110.4,18.1 101.1,18.1\"/>\n                    <path d=\"M158.8,26.9c2.1-0.8,4.3-2.9,4.3-6.6c0-4.5-3.1-7.4-7.7-7.4h-10.5v22.3h5.8v-7.5h2.2l4.1,7.5h6.7L158.8,26.9z M154.7,22.5 h-4V18h4c1.5,0,2.5,0.9,2.5,2.2C157.2,21.6,156.2,22.5,154.7,22.5z\"/>\n                    <path d=\"M130.8,12.5c-6.8,0-11.6,4.9-11.6,11.5s4.9,11.5,11.6,11.5s11.7-4.9,11.7-11.5S137.6,12.5,130.8,12.5z M130.8,30.3 c-3.4,0-5.7-2.6-5.7-6.3c0-3.8,2.3-6.3,5.7-6.3c3.4,0,5.8,2.6,5.8,6.3C136.6,27.7,134.2,30.3,130.8,30.3z\"/>\n                    <polygon points=\"82.1,12.8 98.3,12.8 98.3,18 87.9,18 87.9,21.3 98,21.3 98,26.4 87.9,26.4 87.9,30 98.3,30 98.3,35.2 82.1,35.2 \"/>\n                    <path d=\"M24.6,9.7C24.6,4.4,20,0,14.4,0S4.2,4.4,4.2,9.7v3.1H0v22.3h0l14.4,6.7l14.4-6.7h0V12.9h-4.2V9.7z M9.4,9.7 c0-2.5,2.2-4.5,5-4.5s5,2,5,4.5v3.1H9.4V9.7z M23,31.5l-8.6,4l-8.6-4V18.1H23V31.5z\"/>\n                    <path d=\"M79.4,20.3c0-4.5-3.1-7.4-7.7-7.4H61.2v22.3H67v-7.5h2.2l4.1,7.5H80l-4.9-8.3C77.2,26.1,79.4,24,79.4,20.3z M71,22.5h-4V18 h4c1.5,0,2.5,0.9,2.5,2.2C73.5,21.6,72.5,22.5,71,22.5z\"/>\n                    <polygon points=\"40.5,12.8 58.6,12.8 58.6,18.1 52.4,18.1 52.4,35.2 46.6,35.2 46.6,18.1 40.5,18.1 \"/>\n                </svg>\n                <div class=\"trezorconnect-close\">\n                    <svg x=\"0px\" y=\"0px\" viewBox=\"24 24 60 60\" width=\"24px\" height=\"24px\" preserveAspectRatio=\"xMinYMin meet\">\n                        <polygon class=\"st0\" points=\"40,67.9 42.1,70 55,57.1 67.9,70 70,67.9 57.1,55 70,42.1 67.9,40 55,52.9 42.1,40 40,42.1 52.9,55 \"/>\n                    </svg>\n                </div>\n            </div>\n            <div class=\"trezorconnect-body\">\n                <h3>Popup was blocked</h3>\n                <p>Please click to \u201CContinue\u201D to open popup manually</p>\n                <button class=\"trezorconnect-open\">Continue</button>\n            </div>\n        </div>\n    </div>\n";

var showPopupRequest = function showPopupRequest(open, cancel) {
  if (document.getElementById(layerID)) {
    return;
  }

  var div = document.createElement('div');
  div.id = layerID;
  div.className = 'trezorconnect-container';
  div.innerHTML = layerInnerHtml;

  if (document.body) {
    document.body.appendChild(div);
  }

  var button = div.getElementsByClassName('trezorconnect-open')[0];

  button.onclick = function () {
    open();

    if (document.body) {
      document.body.removeChild(div);
    }
  };

  var close = div.getElementsByClassName('trezorconnect-close')[0];

  close.onclick = function () {
    cancel();

    if (document.body) {
      document.body.removeChild(div);
    }
  };
};

exports.showPopupRequest = showPopupRequest;

/***/ }),
/* 50 */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Headers", function() { return Headers; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Request", function() { return Request; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Response", function() { return Response; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "DOMException", function() { return DOMException; });
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "fetch", function() { return fetch; });
var global =
  (typeof globalThis !== 'undefined' && globalThis) ||
  (typeof self !== 'undefined' && self) ||
  (typeof global !== 'undefined' && global)

var support = {
  searchParams: 'URLSearchParams' in global,
  iterable: 'Symbol' in global && 'iterator' in Symbol,
  blob:
    'FileReader' in global &&
    'Blob' in global &&
    (function() {
      try {
        new Blob()
        return true
      } catch (e) {
        return false
      }
    })(),
  formData: 'FormData' in global,
  arrayBuffer: 'ArrayBuffer' in global
}

function isDataView(obj) {
  return obj && DataView.prototype.isPrototypeOf(obj)
}

if (support.arrayBuffer) {
  var viewClasses = [
    '[object Int8Array]',
    '[object Uint8Array]',
    '[object Uint8ClampedArray]',
    '[object Int16Array]',
    '[object Uint16Array]',
    '[object Int32Array]',
    '[object Uint32Array]',
    '[object Float32Array]',
    '[object Float64Array]'
  ]

  var isArrayBufferView =
    ArrayBuffer.isView ||
    function(obj) {
      return obj && viewClasses.indexOf(Object.prototype.toString.call(obj)) > -1
    }
}

function normalizeName(name) {
  if (typeof name !== 'string') {
    name = String(name)
  }
  if (/[^a-z0-9\-#$%&'*+.^_`|~!]/i.test(name) || name === '') {
    throw new TypeError('Invalid character in header field name: "' + name + '"')
  }
  return name.toLowerCase()
}

function normalizeValue(value) {
  if (typeof value !== 'string') {
    value = String(value)
  }
  return value
}

// Build a destructive iterator for the value list
function iteratorFor(items) {
  var iterator = {
    next: function() {
      var value = items.shift()
      return {done: value === undefined, value: value}
    }
  }

  if (support.iterable) {
    iterator[Symbol.iterator] = function() {
      return iterator
    }
  }

  return iterator
}

function Headers(headers) {
  this.map = {}

  if (headers instanceof Headers) {
    headers.forEach(function(value, name) {
      this.append(name, value)
    }, this)
  } else if (Array.isArray(headers)) {
    headers.forEach(function(header) {
      this.append(header[0], header[1])
    }, this)
  } else if (headers) {
    Object.getOwnPropertyNames(headers).forEach(function(name) {
      this.append(name, headers[name])
    }, this)
  }
}

Headers.prototype.append = function(name, value) {
  name = normalizeName(name)
  value = normalizeValue(value)
  var oldValue = this.map[name]
  this.map[name] = oldValue ? oldValue + ', ' + value : value
}

Headers.prototype['delete'] = function(name) {
  delete this.map[normalizeName(name)]
}

Headers.prototype.get = function(name) {
  name = normalizeName(name)
  return this.has(name) ? this.map[name] : null
}

Headers.prototype.has = function(name) {
  return this.map.hasOwnProperty(normalizeName(name))
}

Headers.prototype.set = function(name, value) {
  this.map[normalizeName(name)] = normalizeValue(value)
}

Headers.prototype.forEach = function(callback, thisArg) {
  for (var name in this.map) {
    if (this.map.hasOwnProperty(name)) {
      callback.call(thisArg, this.map[name], name, this)
    }
  }
}

Headers.prototype.keys = function() {
  var items = []
  this.forEach(function(value, name) {
    items.push(name)
  })
  return iteratorFor(items)
}

Headers.prototype.values = function() {
  var items = []
  this.forEach(function(value) {
    items.push(value)
  })
  return iteratorFor(items)
}

Headers.prototype.entries = function() {
  var items = []
  this.forEach(function(value, name) {
    items.push([name, value])
  })
  return iteratorFor(items)
}

if (support.iterable) {
  Headers.prototype[Symbol.iterator] = Headers.prototype.entries
}

function consumed(body) {
  if (body.bodyUsed) {
    return Promise.reject(new TypeError('Already read'))
  }
  body.bodyUsed = true
}

function fileReaderReady(reader) {
  return new Promise(function(resolve, reject) {
    reader.onload = function() {
      resolve(reader.result)
    }
    reader.onerror = function() {
      reject(reader.error)
    }
  })
}

function readBlobAsArrayBuffer(blob) {
  var reader = new FileReader()
  var promise = fileReaderReady(reader)
  reader.readAsArrayBuffer(blob)
  return promise
}

function readBlobAsText(blob) {
  var reader = new FileReader()
  var promise = fileReaderReady(reader)
  reader.readAsText(blob)
  return promise
}

function readArrayBufferAsText(buf) {
  var view = new Uint8Array(buf)
  var chars = new Array(view.length)

  for (var i = 0; i < view.length; i++) {
    chars[i] = String.fromCharCode(view[i])
  }
  return chars.join('')
}

function bufferClone(buf) {
  if (buf.slice) {
    return buf.slice(0)
  } else {
    var view = new Uint8Array(buf.byteLength)
    view.set(new Uint8Array(buf))
    return view.buffer
  }
}

function Body() {
  this.bodyUsed = false

  this._initBody = function(body) {
    /*
      fetch-mock wraps the Response object in an ES6 Proxy to
      provide useful test harness features such as flush. However, on
      ES5 browsers without fetch or Proxy support pollyfills must be used;
      the proxy-pollyfill is unable to proxy an attribute unless it exists
      on the object before the Proxy is created. This change ensures
      Response.bodyUsed exists on the instance, while maintaining the
      semantic of setting Request.bodyUsed in the constructor before
      _initBody is called.
    */
    this.bodyUsed = this.bodyUsed
    this._bodyInit = body
    if (!body) {
      this._bodyText = ''
    } else if (typeof body === 'string') {
      this._bodyText = body
    } else if (support.blob && Blob.prototype.isPrototypeOf(body)) {
      this._bodyBlob = body
    } else if (support.formData && FormData.prototype.isPrototypeOf(body)) {
      this._bodyFormData = body
    } else if (support.searchParams && URLSearchParams.prototype.isPrototypeOf(body)) {
      this._bodyText = body.toString()
    } else if (support.arrayBuffer && support.blob && isDataView(body)) {
      this._bodyArrayBuffer = bufferClone(body.buffer)
      // IE 10-11 can't handle a DataView body.
      this._bodyInit = new Blob([this._bodyArrayBuffer])
    } else if (support.arrayBuffer && (ArrayBuffer.prototype.isPrototypeOf(body) || isArrayBufferView(body))) {
      this._bodyArrayBuffer = bufferClone(body)
    } else {
      this._bodyText = body = Object.prototype.toString.call(body)
    }

    if (!this.headers.get('content-type')) {
      if (typeof body === 'string') {
        this.headers.set('content-type', 'text/plain;charset=UTF-8')
      } else if (this._bodyBlob && this._bodyBlob.type) {
        this.headers.set('content-type', this._bodyBlob.type)
      } else if (support.searchParams && URLSearchParams.prototype.isPrototypeOf(body)) {
        this.headers.set('content-type', 'application/x-www-form-urlencoded;charset=UTF-8')
      }
    }
  }

  if (support.blob) {
    this.blob = function() {
      var rejected = consumed(this)
      if (rejected) {
        return rejected
      }

      if (this._bodyBlob) {
        return Promise.resolve(this._bodyBlob)
      } else if (this._bodyArrayBuffer) {
        return Promise.resolve(new Blob([this._bodyArrayBuffer]))
      } else if (this._bodyFormData) {
        throw new Error('could not read FormData body as blob')
      } else {
        return Promise.resolve(new Blob([this._bodyText]))
      }
    }

    this.arrayBuffer = function() {
      if (this._bodyArrayBuffer) {
        var isConsumed = consumed(this)
        if (isConsumed) {
          return isConsumed
        }
        if (ArrayBuffer.isView(this._bodyArrayBuffer)) {
          return Promise.resolve(
            this._bodyArrayBuffer.buffer.slice(
              this._bodyArrayBuffer.byteOffset,
              this._bodyArrayBuffer.byteOffset + this._bodyArrayBuffer.byteLength
            )
          )
        } else {
          return Promise.resolve(this._bodyArrayBuffer)
        }
      } else {
        return this.blob().then(readBlobAsArrayBuffer)
      }
    }
  }

  this.text = function() {
    var rejected = consumed(this)
    if (rejected) {
      return rejected
    }

    if (this._bodyBlob) {
      return readBlobAsText(this._bodyBlob)
    } else if (this._bodyArrayBuffer) {
      return Promise.resolve(readArrayBufferAsText(this._bodyArrayBuffer))
    } else if (this._bodyFormData) {
      throw new Error('could not read FormData body as text')
    } else {
      return Promise.resolve(this._bodyText)
    }
  }

  if (support.formData) {
    this.formData = function() {
      return this.text().then(decode)
    }
  }

  this.json = function() {
    return this.text().then(JSON.parse)
  }

  return this
}

// HTTP methods whose capitalization should be normalized
var methods = ['DELETE', 'GET', 'HEAD', 'OPTIONS', 'POST', 'PUT']

function normalizeMethod(method) {
  var upcased = method.toUpperCase()
  return methods.indexOf(upcased) > -1 ? upcased : method
}

function Request(input, options) {
  if (!(this instanceof Request)) {
    throw new TypeError('Please use the "new" operator, this DOM object constructor cannot be called as a function.')
  }

  options = options || {}
  var body = options.body

  if (input instanceof Request) {
    if (input.bodyUsed) {
      throw new TypeError('Already read')
    }
    this.url = input.url
    this.credentials = input.credentials
    if (!options.headers) {
      this.headers = new Headers(input.headers)
    }
    this.method = input.method
    this.mode = input.mode
    this.signal = input.signal
    if (!body && input._bodyInit != null) {
      body = input._bodyInit
      input.bodyUsed = true
    }
  } else {
    this.url = String(input)
  }

  this.credentials = options.credentials || this.credentials || 'same-origin'
  if (options.headers || !this.headers) {
    this.headers = new Headers(options.headers)
  }
  this.method = normalizeMethod(options.method || this.method || 'GET')
  this.mode = options.mode || this.mode || null
  this.signal = options.signal || this.signal
  this.referrer = null

  if ((this.method === 'GET' || this.method === 'HEAD') && body) {
    throw new TypeError('Body not allowed for GET or HEAD requests')
  }
  this._initBody(body)

  if (this.method === 'GET' || this.method === 'HEAD') {
    if (options.cache === 'no-store' || options.cache === 'no-cache') {
      // Search for a '_' parameter in the query string
      var reParamSearch = /([?&])_=[^&]*/
      if (reParamSearch.test(this.url)) {
        // If it already exists then set the value with the current time
        this.url = this.url.replace(reParamSearch, '$1_=' + new Date().getTime())
      } else {
        // Otherwise add a new '_' parameter to the end with the current time
        var reQueryString = /\?/
        this.url += (reQueryString.test(this.url) ? '&' : '?') + '_=' + new Date().getTime()
      }
    }
  }
}

Request.prototype.clone = function() {
  return new Request(this, {body: this._bodyInit})
}

function decode(body) {
  var form = new FormData()
  body
    .trim()
    .split('&')
    .forEach(function(bytes) {
      if (bytes) {
        var split = bytes.split('=')
        var name = split.shift().replace(/\+/g, ' ')
        var value = split.join('=').replace(/\+/g, ' ')
        form.append(decodeURIComponent(name), decodeURIComponent(value))
      }
    })
  return form
}

function parseHeaders(rawHeaders) {
  var headers = new Headers()
  // Replace instances of \r\n and \n followed by at least one space or horizontal tab with a space
  // https://tools.ietf.org/html/rfc7230#section-3.2
  var preProcessedHeaders = rawHeaders.replace(/\r?\n[\t ]+/g, ' ')
  // Avoiding split via regex to work around a common IE11 bug with the core-js 3.6.0 regex polyfill
  // https://github.com/github/fetch/issues/748
  // https://github.com/zloirock/core-js/issues/751
  preProcessedHeaders
    .split('\r')
    .map(function(header) {
      return header.indexOf('\n') === 0 ? header.substr(1, header.length) : header
    })
    .forEach(function(line) {
      var parts = line.split(':')
      var key = parts.shift().trim()
      if (key) {
        var value = parts.join(':').trim()
        headers.append(key, value)
      }
    })
  return headers
}

Body.call(Request.prototype)

function Response(bodyInit, options) {
  if (!(this instanceof Response)) {
    throw new TypeError('Please use the "new" operator, this DOM object constructor cannot be called as a function.')
  }
  if (!options) {
    options = {}
  }

  this.type = 'default'
  this.status = options.status === undefined ? 200 : options.status
  this.ok = this.status >= 200 && this.status < 300
  this.statusText = options.statusText === undefined ? '' : '' + options.statusText
  this.headers = new Headers(options.headers)
  this.url = options.url || ''
  this._initBody(bodyInit)
}

Body.call(Response.prototype)

Response.prototype.clone = function() {
  return new Response(this._bodyInit, {
    status: this.status,
    statusText: this.statusText,
    headers: new Headers(this.headers),
    url: this.url
  })
}

Response.error = function() {
  var response = new Response(null, {status: 0, statusText: ''})
  response.type = 'error'
  return response
}

var redirectStatuses = [301, 302, 303, 307, 308]

Response.redirect = function(url, status) {
  if (redirectStatuses.indexOf(status) === -1) {
    throw new RangeError('Invalid status code')
  }

  return new Response(null, {status: status, headers: {location: url}})
}

var DOMException = global.DOMException
try {
  new DOMException()
} catch (err) {
  DOMException = function(message, name) {
    this.message = message
    this.name = name
    var error = Error(message)
    this.stack = error.stack
  }
  DOMException.prototype = Object.create(Error.prototype)
  DOMException.prototype.constructor = DOMException
}

function fetch(input, init) {
  return new Promise(function(resolve, reject) {
    var request = new Request(input, init)

    if (request.signal && request.signal.aborted) {
      return reject(new DOMException('Aborted', 'AbortError'))
    }

    var xhr = new XMLHttpRequest()

    function abortXhr() {
      xhr.abort()
    }

    xhr.onload = function() {
      var options = {
        status: xhr.status,
        statusText: xhr.statusText,
        headers: parseHeaders(xhr.getAllResponseHeaders() || '')
      }
      options.url = 'responseURL' in xhr ? xhr.responseURL : options.headers.get('X-Request-URL')
      var body = 'response' in xhr ? xhr.response : xhr.responseText
      setTimeout(function() {
        resolve(new Response(body, options))
      }, 0)
    }

    xhr.onerror = function() {
      setTimeout(function() {
        reject(new TypeError('Network request failed'))
      }, 0)
    }

    xhr.ontimeout = function() {
      setTimeout(function() {
        reject(new TypeError('Network request failed'))
      }, 0)
    }

    xhr.onabort = function() {
      setTimeout(function() {
        reject(new DOMException('Aborted', 'AbortError'))
      }, 0)
    }

    function fixUrl(url) {
      try {
        return url === '' && global.location.href ? global.location.href : url
      } catch (e) {
        return url
      }
    }

    xhr.open(request.method, fixUrl(request.url), true)

    if (request.credentials === 'include') {
      xhr.withCredentials = true
    } else if (request.credentials === 'omit') {
      xhr.withCredentials = false
    }

    if ('responseType' in xhr) {
      if (support.blob) {
        xhr.responseType = 'blob'
      } else if (
        support.arrayBuffer &&
        request.headers.get('Content-Type') &&
        request.headers.get('Content-Type').indexOf('application/octet-stream') !== -1
      ) {
        xhr.responseType = 'arraybuffer'
      }
    }

    if (init && typeof init.headers === 'object' && !(init.headers instanceof Headers)) {
      Object.getOwnPropertyNames(init.headers).forEach(function(name) {
        xhr.setRequestHeader(name, normalizeValue(init.headers[name]))
      })
    } else {
      request.headers.forEach(function(value, name) {
        xhr.setRequestHeader(name, value)
      })
    }

    if (request.signal) {
      request.signal.addEventListener('abort', abortXhr)

      xhr.onreadystatechange = function() {
        // DONE (success or failure)
        if (xhr.readyState === 4) {
          request.signal.removeEventListener('abort', abortXhr)
        }
      }
    }

    xhr.send(typeof request._bodyInit === 'undefined' ? null : request._bodyInit)
  })
}

fetch.polyfill = true

if (!global.fetch) {
  global.fetch = fetch
  global.Headers = Headers
  global.Request = Request
  global.Response = Response
}


/***/ }),
/* 51 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireWildcard = __webpack_require__(3);

var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.clearTimeout = exports.dispose = exports.postMessage = exports.init = exports.messagePromises = exports.error = exports.timeout = exports.initPromise = exports.origin = exports.instance = void 0;

var _regenerator = _interopRequireDefault(__webpack_require__(5));

var _asyncToGenerator2 = _interopRequireDefault(__webpack_require__(6));

var _deferred = __webpack_require__(15);

var IFRAME = _interopRequireWildcard(__webpack_require__(8));

var _errors = __webpack_require__(9);

var _networkUtils = __webpack_require__(14);

var _inlineStyles = _interopRequireDefault(__webpack_require__(52));

var instance;
exports.instance = instance;
var origin;
exports.origin = origin;
var initPromise = (0, _deferred.create)();
exports.initPromise = initPromise;
var timeout = 0;
exports.timeout = timeout;
var error;
exports.error = error;
var _messageID = 0; // every postMessage to iframe has its own promise to resolve

var messagePromises = {};
exports.messagePromises = messagePromises;

var init = /*#__PURE__*/function () {
  var _ref = (0, _asyncToGenerator2["default"])( /*#__PURE__*/_regenerator["default"].mark(function _callee(settings) {
    var existedFrame, src, manifestString, manifest, onLoad;
    return _regenerator["default"].wrap(function _callee$(_context) {
      while (1) {
        switch (_context.prev = _context.next) {
          case 0:
            exports.initPromise = initPromise = (0, _deferred.create)();
            existedFrame = document.getElementById('trezorconnect');

            if (existedFrame) {
              exports.instance = instance = existedFrame;
            } else {
              exports.instance = instance = document.createElement('iframe');
              instance.frameBorder = '0';
              instance.width = '0px';
              instance.height = '0px';
              instance.style.position = 'absolute';
              instance.style.display = 'none';
              instance.style.border = '0px';
              instance.style.width = '0px';
              instance.style.height = '0px';
              instance.id = 'trezorconnect';
              instance.setAttribute('sandbox', 'allow-same-origin allow-scripts');
            }

            if (settings.env === 'web') {
              manifestString = settings.manifest ? JSON.stringify(settings.manifest) : 'undefined'; // note: btoa(undefined) === btoa('undefined') === "dW5kZWZpbmVk"

              manifest = "&version=" + settings.version + "&manifest=" + encodeURIComponent(btoa(JSON.stringify(manifestString)));
              src = settings.iframeSrc + "?" + Date.now() + manifest;
            } else {
              src = settings.iframeSrc;
            }

            instance.setAttribute('src', src);

            if (settings.webusb) {
              instance.setAttribute('allow', 'usb');
            }

            exports.origin = origin = (0, _networkUtils.getOrigin)(instance.src);
            exports.timeout = timeout = window.setTimeout(function () {
              initPromise.reject(_errors.IFRAME_TIMEOUT);
            }, 10000);

            onLoad = function onLoad() {
              if (!instance) {
                initPromise.reject(_errors.IFRAME_BLOCKED);
                return;
              }

              try {
                // if hosting page is able to access cross-origin location it means that the iframe is not loaded
                var iframeOrigin = instance.contentWindow.location.origin;

                if (!iframeOrigin || iframeOrigin === 'null') {
                  // eslint-disable-next-line no-use-before-define
                  handleIframeBlocked();
                  return;
                }
              } catch (e) {// empty
                console.log("5007:", e)
              }

              var extension; // $FlowIssue chrome is not declared outside
              
              if (typeof chrome !== 'undefined' && chrome.runtime && typeof chrome.runtime.onConnect !== 'undefined') {
                chrome.runtime.onConnect.addListener(function () {});
                extension = chrome.runtime.id;
              }
              instance.contentWindow.postMessage({
                type: IFRAME.INIT,
                payload: {
                  settings: settings,
                  extension: extension
                }
              }, origin);
              instance.onload = undefined;
            }; // IE hack


            if (instance.attachEvent) {
              instance.attachEvent('onload', onLoad);
            } else {
              instance.onload = onLoad;
            } // inject iframe into host document body

            
            if (document.body) {
              document.body.appendChild(instance); // eslint-disable-next-line no-use-before-define

              injectStyleSheet();
            }

            _context.prev = 11;
            _context.next = 14;
            return initPromise.promise;

          case 14:
            _context.next = 20;
            break;

          case 16:
            _context.prev = 16;
            _context.t0 = _context["catch"](11);

            // reset state to allow initialization again
            if (instance) {
              if (instance.parentNode) {
                instance.parentNode.removeChild(instance);
              } // eslint-disable-next-line require-atomic-updates


              exports.instance = instance = null;
            }

            throw _context.t0.message || _context.t0;

          case 20:
            _context.prev = 20;
            window.clearTimeout(timeout);
            exports.timeout = timeout = 0;
            return _context.finish(20);

          case 24:
          case "end":
            return _context.stop();
        }
      }
    }, _callee, null, [[11, 16, 20, 24]]);
  }));

  return function init(_x) {
    return _ref.apply(this, arguments);
  };
}();

exports.init = init;

var injectStyleSheet = function injectStyleSheet() {
  if (!instance) {
    throw _errors.IFRAME_BLOCKED;
  }

  var doc = instance.ownerDocument;
  var head = doc.head || doc.getElementsByTagName('head')[0];
  var style = document.createElement('style');
  style.setAttribute('type', 'text/css');
  style.setAttribute('id', 'TrezorConnectStylesheet'); // $FlowIssue

  if (style.styleSheet) {
    // IE
    // $FlowIssue
    style.styleSheet.cssText = _inlineStyles["default"];
    head.appendChild(style);
  } else {
    style.appendChild(document.createTextNode(_inlineStyles["default"]));
    head.append(style);
  }
};

var handleIframeBlocked = function handleIframeBlocked() {
  window.clearTimeout(timeout);
  exports.error = error = _errors.IFRAME_BLOCKED.message; // eslint-disable-next-line no-use-before-define

  dispose();
  initPromise.reject(_errors.IFRAME_BLOCKED);
}; // post messages to iframe


var postMessage = function postMessage(message, usePromise) {
  if (usePromise === void 0) {
    usePromise = true;
  }

  if (!instance) {
    throw _errors.IFRAME_BLOCKED;
  }

  if (usePromise) {
    _messageID++;
    message.id = _messageID;
    messagePromises[_messageID] = (0, _deferred.create)();
    var promise = messagePromises[_messageID].promise;
    instance.contentWindow.postMessage(message, origin);
    return promise;
  }

  instance.contentWindow.postMessage(message, origin);
  return null;
};

exports.postMessage = postMessage;

var dispose = function dispose() {
  if (instance && instance.parentNode) {
    try {
      instance.parentNode.removeChild(instance);
    } catch (error) {// do nothing
    }
  }

  exports.instance = instance = null;
  exports.timeout = timeout = 0;
};

exports.dispose = dispose;

var clearTimeout = function clearTimeout() {
  window.clearTimeout(timeout);
};

exports.clearTimeout = clearTimeout;

/***/ }),
/* 52 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports["default"] = void 0;
var css = '.trezorconnect-container{position:fixed!important;display:-webkit-box!important;display:-webkit-flex!important;display:-ms-flexbox!important;display:flex!important;-webkit-box-orient:vertical!important;-webkit-box-direction:normal!important;-webkit-flex-direction:column!important;-ms-flex-direction:column!important;flex-direction:column!important;-webkit-box-align:center!important;-webkit-align-items:center!important;-ms-flex-align:center!important;align-items:center!important;z-index:10000!important;width:100%!important;height:100%!important;top:0!important;left:0!important;background:rgba(0,0,0,.35)!important;overflow:auto!important;padding:20px!important;margin:0!important}.trezorconnect-container .trezorconnect-window{position:relative!important;display:block!important;width:370px!important;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif!important;margin:auto!important;border-radius:3px!important;background-color:#fff!important;text-align:center!important;overflow:hidden!important}.trezorconnect-container .trezorconnect-window .trezorconnect-head{text-align:left;padding:12px 24px!important;display:-webkit-box!important;display:-webkit-flex!important;display:-ms-flexbox!important;display:flex!important;-webkit-box-align:center!important;-webkit-align-items:center!important;-ms-flex-align:center!important;align-items:center!important}.trezorconnect-container .trezorconnect-window .trezorconnect-head .trezorconnect-logo{-webkit-box-flex:1;-webkit-flex:1;-ms-flex:1;flex:1}.trezorconnect-container .trezorconnect-window .trezorconnect-head .trezorconnect-close{cursor:pointer!important;height:24px!important}.trezorconnect-container .trezorconnect-window .trezorconnect-head .trezorconnect-close svg{fill:#757575;-webkit-transition:fill .3s ease-in-out!important;transition:fill .3s ease-in-out!important}.trezorconnect-container .trezorconnect-window .trezorconnect-head .trezorconnect-close:hover svg{fill:#494949}.trezorconnect-container .trezorconnect-window .trezorconnect-body{padding:24px 24px 32px!important;background:#FBFBFB!important;border-top:1px solid #EBEBEB}.trezorconnect-container .trezorconnect-window .trezorconnect-body h3{color:#505050!important;font-size:16px!important;font-weight:500!important}.trezorconnect-container .trezorconnect-window .trezorconnect-body p{margin:8px 0 24px!important;font-weight:400!important;color:#A9A9A9!important;font-size:12px!important}.trezorconnect-container .trezorconnect-window .trezorconnect-body button{width:100%!important;padding:12px 24px!important;margin:0!important;border-radius:3px!important;font-size:14px!important;font-weight:300!important;cursor:pointer!important;background:#01B757!important;color:#fff!important;border:0!important;-webkit-transition:background-color .3s ease-in-out!important;transition:background-color .3s ease-in-out!important}.trezorconnect-container .trezorconnect-window .trezorconnect-body button:hover{background-color:#00AB51!important}.trezorconnect-container .trezorconnect-window .trezorconnect-body button:active{background-color:#009546!important}/*# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImlucHV0IiwiJHN0ZGluIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQWNBLHlCQUNJLFNBQUEsZ0JBQ0EsUUFBQSxzQkFDQSxRQUFBLHVCQUNBLFFBQUEsc0JBRUEsUUFBQSxlQUNBLG1CQUFBLG1CQUNBLHNCQUFBLGlCQUNBLHVCQUFBLGlCQUNBLG1CQUFBLGlCQUNBLGVBQUEsaUJBRUEsa0JBQUEsaUJBQ0Esb0JBQUEsaUJBQ0EsZUFBQSxpQkNmTSxZQUFhLGlCREFyQixRQUFTLGdCQWtCSCxNQUFBLGVBQ0EsT0FBQSxlQUNBLElBQUEsWUFDQSxLQUFBLFlBQ0EsV0FBQSwwQkFDQSxTQUFBLGVBQ0EsUUFBQSxlQUNBLE9BQUEsWUNkUiwrQ0RYRSxTQUFVLG1CQTZCQSxRQUFBLGdCQUNBLE1BQUEsZ0JBQ0EsWUFBQSxjQUFBLG1CQUFBLFdBQUEsT0FBQSxpQkFBQSxNQUFBLHFCQUNBLE9BQUEsZUNmVixjQUFlLGNEakJmLGlCQWlCRSxlQWtCWSxXQUFBLGlCQ2ZkLFNBQVUsaUJEbUJJLG1FQUNBLFdBQUEsS0NoQmQsUUFBUyxLQUFLLGVEeEJkLFFBQVMsc0JBMENTLFFBQUEsdUJBQ0EsUUFBQSxzQkNmbEIsUUFBUyxlRGlCSyxrQkE1QlosaUJBOEJvQixvQkFBQSxpQkNoQmxCLGVBQWdCLGlCRC9CWixZQWlCTixpQkFzQ1EsdUZBQ0EsaUJBQUEsRUNwQlYsYUFBYyxFRHBDVixTQUFVLEVBMkRBLEtBQUEsRUFFQSx3RkNwQmQsT0FBUSxrQkR6Q1IsT0FBUSxlQWlFTSw0RkFDQSxLQUFBLFFBQ0EsbUJBQUEsS0FBQSxJQUFBLHNCQ3BCZCxXQUFZLEtBQUssSUFBSyxzQkR3QlIsa0dBQ0EsS0FBQSxRQUVBLG1FQUNBLFFBQUEsS0FBQSxLQUFBLGVBQ0EsV0FBQSxrQkFDQSxXQUFBLElBQUEsTUFBQSxRQUVBLHNFQUNBLE1BQUEsa0JBQ0EsVUFBQSxlQ3JCZCxZQUFhLGNEd0JLLHFFQ3JCbEIsT0FBUSxJQUFJLEVBQUksZUR3QkYsWUFBQSxjQUNJLE1BQUEsa0JDdEJsQixVQUFXLGVBRWIsMEVBQ0UsTUFBTyxlQUNQLFFBQVMsS0FBSyxlQUNkLE9BQVEsWUFDUixjQUFlLGNBQ2YsVUFBVyxlQUNYLFlBQWEsY0FDYixPQUFRLGtCQUNSLFdBQVksa0JBQ1osTUFBTyxlQUNQLE9BQVEsWUFDUixtQkFBb0IsaUJBQWlCLElBQUssc0JBQzFDLFdBQVksaUJBQWlCLElBQUssc0JBRXBDLGdGQUNFLGlCQUFrQixrQkFFcEIsaUZBQ0UsaUJBQWtCIn0= */';
var _default = css;
exports["default"] = _default;

/***/ }),
/* 53 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports["default"] = void 0;

var render = function render(className, url, origin) {
  var query = className || '.trezor-webusb-button';
  var buttons = document.querySelectorAll(query);
  var src = url + "?" + Date.now();
  buttons.forEach(function (b) {
    if (b.getElementsByTagName('iframe').length < 1) {
      var bounds = b.getBoundingClientRect();
      var btnIframe = document.createElement('iframe');
      btnIframe.frameBorder = '0';
      btnIframe.width = Math.round(bounds.width) + 'px';
      btnIframe.height = Math.round(bounds.height) + 'px';
      btnIframe.style.position = 'absolute';
      btnIframe.style.top = '0px';
      btnIframe.style.left = '0px';
      btnIframe.style.zIndex = '1'; // btnIframe.style.opacity = '0'; // this makes click impossible on cross-origin

      btnIframe.setAttribute('allow', 'usb');
      btnIframe.setAttribute('scrolling', 'no');

      btnIframe.onload = function () {
        btnIframe.contentWindow.postMessage({// style: JSON.stringify( window.getComputedStyle(b) ),
          // outer: b.outerHTML,
          // inner: b.innerHTML
        }, origin);
      };

      btnIframe.src = src; // inject iframe into button

      b.append(btnIframe);
    }
  });
};

var _default = render;
exports["default"] = _default;

/***/ }),
/* 54 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


exports.__esModule = true;
exports.parseMessage = void 0;

// parse MessageEvent .data into CoreMessage
var parseMessage = function parseMessage(messageData) {
  var message = {
    event: messageData.event,
    type: messageData.type,
    payload: messageData.payload
  };

  if (typeof messageData.id === 'number') {
    message.id = messageData.id;
  }

  if (typeof messageData.success === 'boolean') {
    message.success = messageData.success;
  }

  return message;
};

exports.parseMessage = parseMessage;

/***/ }),
/* 55 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
/* WEBPACK VAR INJECTION */(function(global) {

var _interopRequireDefault = __webpack_require__(1);

exports.__esModule = true;
exports.parse = exports.corsValidator = exports.getEnv = exports.DEFAULT_PRIORITY = void 0;

var _defineProperty2 = _interopRequireDefault(__webpack_require__(4));

function ownKeys(object, enumerableOnly) { var keys = Object.keys(object); if (Object.getOwnPropertySymbols) { var symbols = Object.getOwnPropertySymbols(object); if (enumerableOnly) symbols = symbols.filter(function (sym) { return Object.getOwnPropertyDescriptor(object, sym).enumerable; }); keys.push.apply(keys, symbols); } return keys; }

function _objectSpread(target) { for (var i = 1; i < arguments.length; i++) { var source = arguments[i] != null ? arguments[i] : {}; if (i % 2) { ownKeys(Object(source), true).forEach(function (key) { (0, _defineProperty2["default"])(target, key, source[key]); }); } else if (Object.getOwnPropertyDescriptors) { Object.defineProperties(target, Object.getOwnPropertyDescriptors(source)); } else { ownKeys(Object(source)).forEach(function (key) { Object.defineProperty(target, key, Object.getOwnPropertyDescriptor(source, key)); }); } } return target; }

/*
 * Initial settings for connect.
 * It could be changed by passing values into TrezorConnect.init(...) method
 */
var VERSION = '8.1.5';
var versionN = VERSION.split('.').map(function (s) {
  return parseInt(s);
}); // const DIRECTORY = `${ versionN[0] }${ (versionN[1] > 0 ? `.${versionN[1]}` : '') }/`;

var DIRECTORY = versionN[0] + "/";
var DEFAULT_DOMAIN = "https://connect.trezor.io/" + DIRECTORY;
var DEFAULT_PRIORITY = 2;
exports.DEFAULT_PRIORITY = DEFAULT_PRIORITY;
var initialSettings = {
  configSrc: './data/config.json',
  // constant
  version: VERSION,
  // constant
  debug: false,
  priority: DEFAULT_PRIORITY,
  trustedHost: false,
  connectSrc: DEFAULT_DOMAIN,
  iframeSrc: DEFAULT_DOMAIN + "iframe.html",
  popup: true,
  popupSrc: DEFAULT_DOMAIN + "popup.html",
  webusbSrc: DEFAULT_DOMAIN + "webusb.html",
  transportReconnect: false,
  webusb: true,
  pendingTransportEvent: true,
  supportedBrowser: typeof navigator !== 'undefined' ? !/Trident|MSIE|Edge/.test(navigator.userAgent) : true,
  manifest: null,
  env: 'web',
  lazyLoad: false,
  timestamp: new Date().getTime()
};
var currentSettings = initialSettings;

var parseManifest = function parseManifest(manifest) {
  if (!manifest) return;
  if (typeof manifest.email !== 'string') return;
  if (typeof manifest.appUrl !== 'string') return;
  return {
    email: manifest.email,
    appUrl: manifest.appUrl
  };
};

var getEnv = function getEnv() {
  // $FlowIssue: chrome is not declared outside the project
  if (typeof chrome !== 'undefined' && chrome.runtime && typeof chrome.runtime.onConnect !== 'undefined') {
    return 'webextension';
  }

  if (typeof navigator !== 'undefined') {
    if (typeof navigator.product === 'string' && navigator.product.toLowerCase() === 'reactnative') {
      return 'react-native';
    }

    var userAgent = navigator.userAgent.toLowerCase();

    if (userAgent.indexOf(' electron/') > -1) {
      return 'electron';
    }
  } // if (typeof navigator !== 'undefined' && typeof navigator.product === 'string' && navigator.product.toLowerCase() === 'reactnative') {
  //     return 'react-native';
  // }
  // if (typeof process !== 'undefined' && process.versions.hasOwnProperty('electron')) {
  //     return 'electron';
  // }


  return 'web';
}; // Cors validation copied from Trezor Bridge
// see: https://github.com/trezor/trezord-go/blob/05991cea5900d18bcc6ece5ae5e319d138fc5551/server/api/api.go#L229
// Its pointless to allow `trezor-connect` endpoints { connectSrc } for domains other than listed below
// `trezord` will block communication anyway


exports.getEnv = getEnv;

var corsValidator = function corsValidator(url) {
  if (typeof url !== 'string') return;
  if (url.match(/^https:\/\/([A-Za-z0-9\-_]+\.)*trezor\.io\//)) return url;
  if (url.match(/^https?:\/\/localhost:[58][0-9]{3}\//)) return url;
  if (url.match(/^https:\/\/([A-Za-z0-9\-_]+\.)*sldev\.cz\//)) return url;
};

exports.corsValidator = corsValidator;

var parse = function parse(input) {
  if (input === void 0) {
    input = {};
  }

  var settings = _objectSpread({}, currentSettings);

  if (Object.prototype.hasOwnProperty.call(input, 'debug')) {
    if (Array.isArray(input)) {// enable log with prefix
    }

    if (typeof input.debug === 'boolean') {
      settings.debug = input.debug;
    } else if (typeof input.debug === 'string') {
      settings.debug = input.debug === 'true';
    }
  }

  if (typeof input.connectSrc === 'string') {
    settings.connectSrc = input.connectSrc;
  } // For debugging purposes `connectSrc` could be defined in `global.__TREZOR_CONNECT_SRC` variable


  if (typeof global !== 'undefined' && typeof global.__TREZOR_CONNECT_SRC === 'string') {
    settings.connectSrc = corsValidator(global.__TREZOR_CONNECT_SRC);
  } // For debugging purposes `connectSrc` could be defined in url query of hosting page. Usage:
  // https://3rdparty-page.com/?trezor-connect-src=https://localhost:8088/


  if (typeof window !== 'undefined' && window.location && typeof window.location.search === 'string') {
    var vars = window.location.search.split('&');
    var customUrl = vars.find(function (v) {
      return v.indexOf('trezor-connect-src') >= 0;
    });

    if (customUrl) {
      var _customUrl$split = customUrl.split('='),
          connectSrc = _customUrl$split[1];

      settings.connectSrc = corsValidator(decodeURIComponent(connectSrc));
    }
  }

  var src = settings.connectSrc || DEFAULT_DOMAIN;
  settings.iframeSrc = src + "iframe.html";
  settings.popupSrc = src + "popup.html";
  settings.webusbSrc = src + "webusb.html";

  if (typeof input.transportReconnect === 'boolean') {
    settings.transportReconnect = input.transportReconnect;
  }

  if (typeof input.webusb === 'boolean') {
    settings.webusb = input.webusb;
  }

  if (typeof input.popup === 'boolean') {
    settings.popup = input.popup;
  }

  if (typeof input.lazyLoad === 'boolean') {
    settings.lazyLoad = input.lazyLoad;
  }

  if (typeof input.pendingTransportEvent === 'boolean') {
    settings.pendingTransportEvent = input.pendingTransportEvent;
  } // local files


  if (typeof window !== 'undefined' && window.location && window.location.protocol === 'file:') {
    settings.origin = "file://" + window.location.pathname;
    settings.webusb = false;
  }

  if (typeof input.extension === 'string') {
    settings.extension = input.extension;
  }

  if (typeof input.env === 'string') {
    settings.env = input.env;
  } else {
    settings.env = getEnv();
  }

  if (typeof input.timestamp === 'number') {
    settings.timestamp = input.timestamp;
  }

  if (typeof input.manifest === 'object') {
    settings.manifest = parseManifest(input.manifest);
  }

  currentSettings = settings;
  return currentSettings;
};

exports.parse = parse;
/* WEBPACK VAR INJECTION */}.call(this, __webpack_require__(16)))

/***/ }),
/* 56 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";
/* WEBPACK VAR INJECTION */(function(global) {

exports.__esModule = true;
exports.popupConsole = exports.enableByPrefix = exports.getLog = exports.enable = exports.init = exports["default"] = void 0;

var _this = void 0;

// https://stackoverflow.com/questions/7505623/colors-in-javascript-console
// https://github.com/pimterry/loglevel/blob/master/lib/loglevel.js
// http://www.color-hex.com/color-palette/5016
var colors = {
  // green
  'DescriptorStream': 'color: #77ab59',
  'DeviceList': 'color: #36802d',
  'Device': 'color: #bada55',
  'Core': 'color: #c9df8a',
  'IFrame': 'color: #FFFFFF; background: #f4a742;',
  'Popup': 'color: #f48a00'
};

var Log = /*#__PURE__*/function () {
  function Log(prefix, enabled) {
    if (enabled === void 0) {
      enabled = false;
    }

    this.prefix = prefix;
    this.enabled = enabled;
    this.messages = [];
    this.css = colors[prefix] || 'color: #000000; background: #FFFFFF;';
  }

  var _proto = Log.prototype;

  _proto.addMessage = function addMessage(level, prefix) {
    for (var _len = arguments.length, args = new Array(_len > 2 ? _len - 2 : 0), _key = 2; _key < _len; _key++) {
      args[_key - 2] = arguments[_key];
    }

    this.messages.push({
      level: level,
      prefix: prefix,
      message: args,
      timestamp: new Date().getTime()
    });
  };

  _proto.log = function log() {
    for (var _len2 = arguments.length, args = new Array(_len2), _key2 = 0; _key2 < _len2; _key2++) {
      args[_key2] = arguments[_key2];
    }

    this.addMessage.apply(this, ['log', this.prefix].concat(args)); // eslint-disable-next-line no-console

    if (this.enabled) {
      var _console;

      (_console = console).log.apply(_console, [this.prefix].concat(args));
    }
  };

  _proto.error = function error() {
    for (var _len3 = arguments.length, args = new Array(_len3), _key3 = 0; _key3 < _len3; _key3++) {
      args[_key3] = arguments[_key3];
    }

    this.addMessage.apply(this, ['error', this.prefix].concat(args)); // eslint-disable-next-line no-console

    if (this.enabled) {
      var _console2;

      (_console2 = console).error.apply(_console2, [this.prefix].concat(args));
    }
  };

  _proto.warn = function warn() {
    for (var _len4 = arguments.length, args = new Array(_len4), _key4 = 0; _key4 < _len4; _key4++) {
      args[_key4] = arguments[_key4];
    }

    this.addMessage.apply(this, ['warn', this.prefix].concat(args)); // eslint-disable-next-line no-console

    if (this.enabled) {
      var _console3;

      (_console3 = console).warn.apply(_console3, [this.prefix].concat(args));
    }
  };

  _proto.debug = function debug() {
    for (var _len5 = arguments.length, args = new Array(_len5), _key5 = 0; _key5 < _len5; _key5++) {
      args[_key5] = arguments[_key5];
    }

    this.addMessage.apply(this, ['debug', this.prefix].concat(args)); // eslint-disable-next-line no-console

    if (this.enabled) {
      var _console4;

      (_console4 = console).log.apply(_console4, ['%c' + this.prefix, this.css].concat(args));
    }
  };

  return Log;
}();

exports["default"] = Log;
var _logs = {};

var init = function init(prefix, enabled) {
  var enab = typeof enabled === 'boolean' ? enabled : false;
  var instance = new Log(prefix, enab);
  _logs[prefix] = instance;
  return instance;
};

exports.init = init;

var enable = function enable(enabled) {
  for (var _i = 0, _Object$keys = Object.keys(_logs); _i < _Object$keys.length; _i++) {
    var l = _Object$keys[_i];
    _logs[l].enabled = enabled;
  }
};

exports.enable = enable;

var getLog = function getLog(args) {
  // if
  var logs = [];

  for (var _i2 = 0, _Object$keys2 = Object.keys(_logs); _i2 < _Object$keys2.length; _i2++) {
    var l = _Object$keys2[_i2];
    logs = logs.concat(_logs[l].messages);
  }

  logs.sort(function (a, b) {
    return a.timestamp - b.timestamp;
  });
  return logs;
};

exports.getLog = getLog;

var enableByPrefix = function enableByPrefix(prefix, enabled) {
  if (_logs[prefix]) {
    _logs[prefix].enabled = enabled;
  }
}; // TODO: enable/disable log at runtime


exports.enableByPrefix = enableByPrefix;

var popupConsole = function popupConsole(tag, postMessage) {
  var c = global.console;
  var orig = {
    error: c.error,
    // warn: c.warn,
    info: c.info,
    debug: c.debug,
    log: c.log
  };
  var log = [];

  var inject = function inject(method, level) {
    return function () {
      // args.unshift('[popup.js]');
      var time = new Date().toUTCString();

      for (var _len6 = arguments.length, args = new Array(_len6), _key6 = 0; _key6 < _len6; _key6++) {
        args[_key6] = arguments[_key6];
      }

      log.push([level, time].concat(args));
      postMessage.apply(_this, [{
        type: tag,
        level: level,
        time: time,
        args: JSON.stringify(args)
      } // { type: 'LOG', level: level, time: time, args: JSON.stringify(deepClone(args)) }
      ]);
      return method.apply(c, args);
    };
  };

  for (var level in orig) {
    c[level] = inject(orig[level], level);
  }
};

exports.popupConsole = popupConsole;
/* WEBPACK VAR INJECTION */}.call(this, __webpack_require__(16)))

/***/ }),
/* 57 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


var _interopRequireWildcard = __webpack_require__(3);

var CONSTANTS = _interopRequireWildcard(__webpack_require__(2));

var P = _interopRequireWildcard(__webpack_require__(18));

var Device = _interopRequireWildcard(__webpack_require__(19));

var Mgmnt = _interopRequireWildcard(__webpack_require__(20));

var Protobuf = _interopRequireWildcard(__webpack_require__(58));

var Account = _interopRequireWildcard(__webpack_require__(21));

var Bitcoin = _interopRequireWildcard(__webpack_require__(22));

var Binance = _interopRequireWildcard(__webpack_require__(23));

var Cardano = _interopRequireWildcard(__webpack_require__(24));

var EOS = _interopRequireWildcard(__webpack_require__(25));

var Ethereum = _interopRequireWildcard(__webpack_require__(26));

var Lisk = _interopRequireWildcard(__webpack_require__(27));

var NEM = _interopRequireWildcard(__webpack_require__(28));

var Ripple = _interopRequireWildcard(__webpack_require__(29));

var Stellar = _interopRequireWildcard(__webpack_require__(30));

var Tezos = _interopRequireWildcard(__webpack_require__(31));

var Misc = _interopRequireWildcard(__webpack_require__(32));

var Events = _interopRequireWildcard(__webpack_require__(33));

var Blockchain = _interopRequireWildcard(__webpack_require__(34));

/***/ }),
/* 58 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 59 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ }),
/* 60 */
/***/ (function(module, exports, __webpack_require__) {

"use strict";


/***/ })
/******/ ]);
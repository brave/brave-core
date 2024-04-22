/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

  
/* Auto generated from @solana/web3.js 1.91.7 index.iife.min.js */
var solanaWeb3 = $(function (exports) {
  "use strict";

  var commonjsGlobal = typeof globalThis !== "undefined" ? globalThis : typeof window !== "undefined" ? window : typeof __webpack_require__.g !== "undefined" ? __webpack_require__.g : typeof self !== "undefined" ? self : $Object.create(null, undefined);
  function getDefaultExportFromCjs(x) {
    return x && x.__esModule && $Object.prototype.hasOwnProperty.call(x, "default") ? x["default"] : x;
  }
  $(getDefaultExportFromCjs);
  function getAugmentedNamespace(n) {
    if (n.__esModule) return n;
    var f = n.default;
    if (typeof f == "function") {
      var a = function a() {
        if (this instanceof a) {
          return Reflect.construct(f, arguments, this.constructor);
        }
        return f.apply(this, arguments);
      };
      $(a);
      a.prototype = f.prototype;
    } else a = $Object.create(null, undefined);
    $Object.defineProperty(a, "__esModule", $(function () {
      let result = $Object.create(null, undefined);
      result.value = true;
      return result;
    })());
    $Object.keys(n).forEach($(function (k) {
      var d = $Object.getOwnPropertyDescriptor(n, k);
      $Object.defineProperty(a, k, d.get ? d : $(function () {
        let result = $Object.create(null, undefined);
        result.enumerable = true;
        result.get = $(function () {
          return n[k];
        });
        return result;
      })());
    }));
    return a;
  }
  $(getAugmentedNamespace);
  var buffer = $Object.create(null, undefined);
  var base64Js = $Object.create(null, undefined);
  base64Js.byteLength = byteLength;
  base64Js.toByteArray = toByteArray;
  base64Js.fromByteArray = fromByteArray;
  var lookup = $Array.of();
  var revLookup = $Array.of();
  var Arr = typeof Uint8Array !== "undefined" ? Uint8Array : Array;
  var code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (var i$1 = 0, len = code.length; i$1 < len; ++i$1) {
    lookup[i$1] = code[i$1];
    revLookup[code.charCodeAt(i$1)] = i$1;
  }
  revLookup["-".charCodeAt(0)] = 62;
  revLookup["_".charCodeAt(0)] = 63;
  function getLens(b64) {
    var len = b64.length;
    if (len % 4 > 0) {
      throw new Error("Invalid string. Length must be a multiple of 4");
    }
    var validLen = b64.indexOf("=");
    if (validLen === -1) validLen = len;
    var placeHoldersLen = validLen === len ? 0 : 4 - validLen % 4;
    return $Array.of(validLen, placeHoldersLen);
  }
  $(getLens);
  function byteLength(b64) {
    var lens = getLens(b64);
    var validLen = lens[0];
    var placeHoldersLen = lens[1];
    return (validLen + placeHoldersLen) * 3 / 4 - placeHoldersLen;
  }
  $(byteLength);
  function _byteLength(b64, validLen, placeHoldersLen) {
    return (validLen + placeHoldersLen) * 3 / 4 - placeHoldersLen;
  }
  $(_byteLength);
  function toByteArray(b64) {
    var tmp;
    var lens = getLens(b64);
    var validLen = lens[0];
    var placeHoldersLen = lens[1];
    var arr = new Arr(_byteLength(b64, validLen, placeHoldersLen));
    var curByte = 0;
    var len = placeHoldersLen > 0 ? validLen - 4 : validLen;
    var i;
    for (i = 0; i < len; i += 4) {
      tmp = revLookup[b64.charCodeAt(i)] << 18 | revLookup[b64.charCodeAt(i + 1)] << 12 | revLookup[b64.charCodeAt(i + 2)] << 6 | revLookup[b64.charCodeAt(i + 3)];
      arr[curByte++] = tmp >> 16 & 255;
      arr[curByte++] = tmp >> 8 & 255;
      arr[curByte++] = tmp & 255;
    }
    if (placeHoldersLen === 2) {
      tmp = revLookup[b64.charCodeAt(i)] << 2 | revLookup[b64.charCodeAt(i + 1)] >> 4;
      arr[curByte++] = tmp & 255;
    }
    if (placeHoldersLen === 1) {
      tmp = revLookup[b64.charCodeAt(i)] << 10 | revLookup[b64.charCodeAt(i + 1)] << 4 | revLookup[b64.charCodeAt(i + 2)] >> 2;
      arr[curByte++] = tmp >> 8 & 255;
      arr[curByte++] = tmp & 255;
    }
    return arr;
  }
  $(toByteArray);
  function tripletToBase64(num) {
    return lookup[num >> 18 & 63] + lookup[num >> 12 & 63] + lookup[num >> 6 & 63] + lookup[num & 63];
  }
  $(tripletToBase64);
  function encodeChunk(uint8, start, end) {
    var tmp;
    var output = $Array.of();
    for (var i = start; i < end; i += 3) {
      tmp = (uint8[i] << 16 & 16711680) + (uint8[i + 1] << 8 & 65280) + (uint8[i + 2] & 255);
      output.push(tripletToBase64(tmp));
    }
    return output.join("");
  }
  $(encodeChunk);
  function fromByteArray(uint8) {
    var tmp;
    var len = uint8.length;
    var extraBytes = len % 3;
    var parts = $Array.of();
    var maxChunkLength = 16383;
    for (var i = 0, len2 = len - extraBytes; i < len2; i += maxChunkLength) {
      parts.push(encodeChunk(uint8, i, i + maxChunkLength > len2 ? len2 : i + maxChunkLength));
    }
    if (extraBytes === 1) {
      tmp = uint8[len - 1];
      parts.push(lookup[tmp >> 2] + lookup[tmp << 4 & 63] + "==");
    } else if (extraBytes === 2) {
      tmp = (uint8[len - 2] << 8) + uint8[len - 1];
      parts.push(lookup[tmp >> 10] + lookup[tmp >> 4 & 63] + lookup[tmp << 2 & 63] + "=");
    }
    return parts.join("");
  }
  $(fromByteArray);
  var ieee754 = $Object.create(null, undefined);
  /*! ieee754. BSD-3-Clause License. Feross Aboukhadijeh <https://feross.org/opensource> */
  ieee754.read = $(function (buffer, offset, isLE, mLen, nBytes) {
    var e, m;
    var eLen = nBytes * 8 - mLen - 1;
    var eMax = (1 << eLen) - 1;
    var eBias = eMax >> 1;
    var nBits = -7;
    var i = isLE ? nBytes - 1 : 0;
    var d = isLE ? -1 : 1;
    var s = buffer[offset + i];
    i += d;
    e = s & (1 << -nBits) - 1;
    s >>= -nBits;
    nBits += eLen;
    for (; nBits > 0; e = e * 256 + buffer[offset + i], i += d, nBits -= 8) {}
    m = e & (1 << -nBits) - 1;
    e >>= -nBits;
    nBits += mLen;
    for (; nBits > 0; m = m * 256 + buffer[offset + i], i += d, nBits -= 8) {}
    if (e === 0) {
      e = 1 - eBias;
    } else if (e === eMax) {
      return m ? NaN : (s ? -1 : 1) * Infinity;
    } else {
      m = m + Math.pow(2, mLen);
      e = e - eBias;
    }
    return (s ? -1 : 1) * m * Math.pow(2, e - mLen);
  });
  ieee754.write = $(function (buffer, value, offset, isLE, mLen, nBytes) {
    var e, m, c;
    var eLen = nBytes * 8 - mLen - 1;
    var eMax = (1 << eLen) - 1;
    var eBias = eMax >> 1;
    var rt = mLen === 23 ? Math.pow(2, -24) - Math.pow(2, -77) : 0;
    var i = isLE ? 0 : nBytes - 1;
    var d = isLE ? 1 : -1;
    var s = value < 0 || value === 0 && 1 / value < 0 ? 1 : 0;
    value = Math.abs(value);
    if (isNaN(value) || value === Infinity) {
      m = isNaN(value) ? 1 : 0;
      e = eMax;
    } else {
      e = Math.floor(Math.log(value) / Math.LN2);
      if (value * (c = Math.pow(2, -e)) < 1) {
        e--;
        c *= 2;
      }
      if (e + eBias >= 1) {
        value += rt / c;
      } else {
        value += rt * Math.pow(2, 1 - eBias);
      }
      if (value * c >= 2) {
        e++;
        c /= 2;
      }
      if (e + eBias >= eMax) {
        m = 0;
        e = eMax;
      } else if (e + eBias >= 1) {
        m = (value * c - 1) * Math.pow(2, mLen);
        e = e + eBias;
      } else {
        m = value * Math.pow(2, eBias - 1) * Math.pow(2, mLen);
        e = 0;
      }
    }
    for (; mLen >= 8; buffer[offset + i] = m & 255, i += d, m /= 256, mLen -= 8) {}
    e = e << mLen | m;
    eLen += mLen;
    for (; eLen > 0; buffer[offset + i] = e & 255, i += d, e /= 256, eLen -= 8) {}
    buffer[offset + i - d] |= s * 128;
  });
  /*!
  	 * The buffer module from node.js, for the browser.
  	 *
  	 * @author   Feross Aboukhadijeh <https://feross.org>
  	 * @license  MIT
  	 */
  $(function (exports) {
    const base64 = base64Js;
    const ieee754$1 = ieee754;
    const customInspectSymbol = typeof Symbol === "function" && typeof Symbol["for"] === "function" ? Symbol["for"]("nodejs.util.inspect.custom") : null;
    exports.Buffer = Buffer;
    exports.SlowBuffer = SlowBuffer;
    exports.INSPECT_MAX_BYTES = 50;
    const K_MAX_LENGTH = 2147483647;
    exports.kMaxLength = K_MAX_LENGTH;
    Buffer.TYPED_ARRAY_SUPPORT = typedArraySupport();
    if (!Buffer.TYPED_ARRAY_SUPPORT && typeof console !== "undefined" && typeof console.error === "function") {
      console.error("This browser lacks typed array (Uint8Array) support which is required by " + "`buffer` v5.x. Use `buffer` v4.x if you require old browser support.");
    }
    function typedArraySupport() {
      try {
        const arr = new Uint8Array(1);
        const proto = $(function () {
          let result = $Object.create(null, undefined);
          result.foo = $(function () {
            return 42;
          });
          return result;
        })();
        $Object.setPrototypeOf(proto, Uint8Array.prototype);
        $Object.setPrototypeOf(arr, proto);
        return arr.foo() === 42;
      } catch (e) {
        return false;
      }
    }
    $(typedArraySupport);
    $Object.defineProperty(Buffer.prototype, "parent", $(function () {
      let result = $Object.create(null, undefined);
      result.enumerable = true;
      result.get = $(function () {
        if (!Buffer.isBuffer(this)) return undefined;
        return this.buffer;
      });
      return result;
    })());
    $Object.defineProperty(Buffer.prototype, "offset", $(function () {
      let result = $Object.create(null, undefined);
      result.enumerable = true;
      result.get = $(function () {
        if (!Buffer.isBuffer(this)) return undefined;
        return this.byteOffset;
      });
      return result;
    })());
    function createBuffer(length) {
      if (length > K_MAX_LENGTH) {
        throw new RangeError('The value "' + length + '" is invalid for option "size"');
      }
      const buf = new Uint8Array(length);
      $Object.setPrototypeOf(buf, Buffer.prototype);
      return buf;
    }
    $(createBuffer);
    function Buffer(arg, encodingOrOffset, length) {
      if (typeof arg === "number") {
        if (typeof encodingOrOffset === "string") {
          throw new TypeError('The "string" argument must be of type string. Received type number');
        }
        return allocUnsafe(arg);
      }
      return from(arg, encodingOrOffset, length);
    }
    $(Buffer);
    Buffer.poolSize = 8192;
    function from(value, encodingOrOffset, length) {
      if (typeof value === "string") {
        return fromString(value, encodingOrOffset);
      }
      if (ArrayBuffer.isView(value)) {
        return fromArrayView(value);
      }
      if (value == null) {
        throw new TypeError("The first argument must be one of type string, Buffer, ArrayBuffer, Array, " + "or Array-like Object. Received type " + typeof value);
      }
      if (isInstance(value, ArrayBuffer) || value && isInstance(value.buffer, ArrayBuffer)) {
        return fromArrayBuffer(value, encodingOrOffset, length);
      }
      if (typeof SharedArrayBuffer !== "undefined" && (isInstance(value, SharedArrayBuffer) || value && isInstance(value.buffer, SharedArrayBuffer))) {
        return fromArrayBuffer(value, encodingOrOffset, length);
      }
      if (typeof value === "number") {
        throw new TypeError('The "value" argument must not be of type number. Received type number');
      }
      const valueOf = value.valueOf && value.valueOf();
      if (valueOf != null && valueOf !== value) {
        return Buffer.from(valueOf, encodingOrOffset, length);
      }
      const b = fromObject(value);
      if (b) return b;
      if (typeof Symbol !== "undefined" && Symbol.toPrimitive != null && typeof value[Symbol.toPrimitive] === "function") {
        return Buffer.from(value[Symbol.toPrimitive]("string"), encodingOrOffset, length);
      }
      throw new TypeError("The first argument must be one of type string, Buffer, ArrayBuffer, Array, " + "or Array-like Object. Received type " + typeof value);
    }
    $(from);
    Buffer.from = $(function (value, encodingOrOffset, length) {
      return from(value, encodingOrOffset, length);
    });
    $Object.setPrototypeOf(Buffer.prototype, Uint8Array.prototype);
    $Object.setPrototypeOf(Buffer, Uint8Array);
    function assertSize(size) {
      if (typeof size !== "number") {
        throw new TypeError('"size" argument must be of type number');
      } else if (size < 0) {
        throw new RangeError('The value "' + size + '" is invalid for option "size"');
      }
    }
    $(assertSize);
    function alloc(size, fill, encoding) {
      assertSize(size);
      if (size <= 0) {
        return createBuffer(size);
      }
      if (fill !== undefined) {
        return typeof encoding === "string" ? createBuffer(size).fill(fill, encoding) : createBuffer(size).fill(fill);
      }
      return createBuffer(size);
    }
    $(alloc);
    Buffer.alloc = $(function (size, fill, encoding) {
      return alloc(size, fill, encoding);
    });
    function allocUnsafe(size) {
      assertSize(size);
      return createBuffer(size < 0 ? 0 : checked(size) | 0);
    }
    $(allocUnsafe);
    Buffer.allocUnsafe = $(function (size) {
      return allocUnsafe(size);
    });
    Buffer.allocUnsafeSlow = $(function (size) {
      return allocUnsafe(size);
    });
    function fromString(string, encoding) {
      if (typeof encoding !== "string" || encoding === "") {
        encoding = "utf8";
      }
      if (!Buffer.isEncoding(encoding)) {
        throw new TypeError("Unknown encoding: " + encoding);
      }
      const length = byteLength(string, encoding) | 0;
      let buf = createBuffer(length);
      const actual = buf.write(string, encoding);
      if (actual !== length) {
        buf = buf.slice(0, actual);
      }
      return buf;
    }
    $(fromString);
    function fromArrayLike(array) {
      const length = array.length < 0 ? 0 : checked(array.length) | 0;
      const buf = createBuffer(length);
      for (let i = 0; i < length; i += 1) {
        buf[i] = array[i] & 255;
      }
      return buf;
    }
    $(fromArrayLike);
    function fromArrayView(arrayView) {
      if (isInstance(arrayView, Uint8Array)) {
        const copy = new Uint8Array(arrayView);
        return fromArrayBuffer(copy.buffer, copy.byteOffset, copy.byteLength);
      }
      return fromArrayLike(arrayView);
    }
    $(fromArrayView);
    function fromArrayBuffer(array, byteOffset, length) {
      if (byteOffset < 0 || array.byteLength < byteOffset) {
        throw new RangeError('"offset" is outside of buffer bounds');
      }
      if (array.byteLength < byteOffset + (length || 0)) {
        throw new RangeError('"length" is outside of buffer bounds');
      }
      let buf;
      if (byteOffset === undefined && length === undefined) {
        buf = new Uint8Array(array);
      } else if (length === undefined) {
        buf = new Uint8Array(array, byteOffset);
      } else {
        buf = new Uint8Array(array, byteOffset, length);
      }
      $Object.setPrototypeOf(buf, Buffer.prototype);
      return buf;
    }
    $(fromArrayBuffer);
    function fromObject(obj) {
      if (Buffer.isBuffer(obj)) {
        const len = checked(obj.length) | 0;
        const buf = createBuffer(len);
        if (buf.length === 0) {
          return buf;
        }
        obj.copy(buf, 0, 0, len);
        return buf;
      }
      if (obj.length !== undefined) {
        if (typeof obj.length !== "number" || numberIsNaN(obj.length)) {
          return createBuffer(0);
        }
        return fromArrayLike(obj);
      }
      if (obj.type === "Buffer" && $Array.isArray(obj.data)) {
        return fromArrayLike(obj.data);
      }
    }
    $(fromObject);
    function checked(length) {
      if (length >= K_MAX_LENGTH) {
        throw new RangeError("Attempt to allocate Buffer larger than maximum " + "size: 0x" + K_MAX_LENGTH.toString(16) + " bytes");
      }
      return length | 0;
    }
    $(checked);
    function SlowBuffer(length) {
      if (+length != length) {
        length = 0;
      }
      return Buffer.alloc(+length);
    }
    $(SlowBuffer);
    Buffer.isBuffer = $(function isBuffer(b) {
      return b != null && b._isBuffer === true && b !== Buffer.prototype;
    });
    Buffer.compare = $(function compare(a, b) {
      if (isInstance(a, Uint8Array)) a = Buffer.from(a, a.offset, a.byteLength);
      if (isInstance(b, Uint8Array)) b = Buffer.from(b, b.offset, b.byteLength);
      if (!Buffer.isBuffer(a) || !Buffer.isBuffer(b)) {
        throw new TypeError('The "buf1", "buf2" arguments must be one of type Buffer or Uint8Array');
      }
      if (a === b) return 0;
      let x = a.length;
      let y = b.length;
      for (let i = 0, len = Math.min(x, y); i < len; ++i) {
        if (a[i] !== b[i]) {
          x = a[i];
          y = b[i];
          break;
        }
      }
      if (x < y) return -1;
      if (y < x) return 1;
      return 0;
    });
    Buffer.isEncoding = $(function isEncoding(encoding) {
      switch (String(encoding).toLowerCase()) {
        case "hex":
        case "utf8":
        case "utf-8":
        case "ascii":
        case "latin1":
        case "binary":
        case "base64":
        case "ucs2":
        case "ucs-2":
        case "utf16le":
        case "utf-16le":
          return true;
        default:
          return false;
      }
    });
    Buffer.concat = $(function concat(list, length) {
      if (!$Array.isArray(list)) {
        throw new TypeError('"list" argument must be an Array of Buffers');
      }
      if (list.length === 0) {
        return Buffer.alloc(0);
      }
      let i;
      if (length === undefined) {
        length = 0;
        for (i = 0; i < list.length; ++i) {
          length += list[i].length;
        }
      }
      const buffer = Buffer.allocUnsafe(length);
      let pos = 0;
      for (i = 0; i < list.length; ++i) {
        let buf = list[i];
        if (isInstance(buf, Uint8Array)) {
          if (pos + buf.length > buffer.length) {
            if (!Buffer.isBuffer(buf)) buf = Buffer.from(buf);
            buf.copy(buffer, pos);
          } else {
            Uint8Array.prototype.set.call(buffer, buf, pos);
          }
        } else if (!Buffer.isBuffer(buf)) {
          throw new TypeError('"list" argument must be an Array of Buffers');
        } else {
          buf.copy(buffer, pos);
        }
        pos += buf.length;
      }
      return buffer;
    });
    function byteLength(string, encoding) {
      if (Buffer.isBuffer(string)) {
        return string.length;
      }
      if (ArrayBuffer.isView(string) || isInstance(string, ArrayBuffer)) {
        return string.byteLength;
      }
      if (typeof string !== "string") {
        throw new TypeError('The "string" argument must be one of type string, Buffer, or ArrayBuffer. ' + "Received type " + typeof string);
      }
      const len = string.length;
      const mustMatch = arguments.length > 2 && arguments[2] === true;
      if (!mustMatch && len === 0) return 0;
      let loweredCase = false;
      for (;;) {
        switch (encoding) {
          case "ascii":
          case "latin1":
          case "binary":
            return len;
          case "utf8":
          case "utf-8":
            return utf8ToBytes(string).length;
          case "ucs2":
          case "ucs-2":
          case "utf16le":
          case "utf-16le":
            return len * 2;
          case "hex":
            return len >>> 1;
          case "base64":
            return base64ToBytes(string).length;
          default:
            if (loweredCase) {
              return mustMatch ? -1 : utf8ToBytes(string).length;
            }
            encoding = ("" + encoding).toLowerCase();
            loweredCase = true;
        }
      }
    }
    $(byteLength);
    Buffer.byteLength = byteLength;
    function slowToString(encoding, start, end) {
      let loweredCase = false;
      if (start === undefined || start < 0) {
        start = 0;
      }
      if (start > this.length) {
        return "";
      }
      if (end === undefined || end > this.length) {
        end = this.length;
      }
      if (end <= 0) {
        return "";
      }
      end >>>= 0;
      start >>>= 0;
      if (end <= start) {
        return "";
      }
      if (!encoding) encoding = "utf8";
      while (true) {
        switch (encoding) {
          case "hex":
            return hexSlice(this, start, end);
          case "utf8":
          case "utf-8":
            return utf8Slice(this, start, end);
          case "ascii":
            return asciiSlice(this, start, end);
          case "latin1":
          case "binary":
            return latin1Slice(this, start, end);
          case "base64":
            return base64Slice(this, start, end);
          case "ucs2":
          case "ucs-2":
          case "utf16le":
          case "utf-16le":
            return utf16leSlice(this, start, end);
          default:
            if (loweredCase) throw new TypeError("Unknown encoding: " + encoding);
            encoding = (encoding + "").toLowerCase();
            loweredCase = true;
        }
      }
    }
    $(slowToString);
    Buffer.prototype._isBuffer = true;
    function swap(b, n, m) {
      const i = b[n];
      b[n] = b[m];
      b[m] = i;
    }
    $(swap);
    Buffer.prototype.swap16 = $(function swap16() {
      const len = this.length;
      if (len % 2 !== 0) {
        throw new RangeError("Buffer size must be a multiple of 16-bits");
      }
      for (let i = 0; i < len; i += 2) {
        swap(this, i, i + 1);
      }
      return this;
    });
    Buffer.prototype.swap32 = $(function swap32() {
      const len = this.length;
      if (len % 4 !== 0) {
        throw new RangeError("Buffer size must be a multiple of 32-bits");
      }
      for (let i = 0; i < len; i += 4) {
        swap(this, i, i + 3);
        swap(this, i + 1, i + 2);
      }
      return this;
    });
    Buffer.prototype.swap64 = $(function swap64() {
      const len = this.length;
      if (len % 8 !== 0) {
        throw new RangeError("Buffer size must be a multiple of 64-bits");
      }
      for (let i = 0; i < len; i += 8) {
        swap(this, i, i + 7);
        swap(this, i + 1, i + 6);
        swap(this, i + 2, i + 5);
        swap(this, i + 3, i + 4);
      }
      return this;
    });
    Buffer.prototype.toString = $(function toString() {
      const length = this.length;
      if (length === 0) return "";
      if (arguments.length === 0) return utf8Slice(this, 0, length);
      return slowToString.apply(this, arguments);
    });
    Buffer.prototype.toLocaleString = Buffer.prototype.toString;
    Buffer.prototype.equals = $(function equals(b) {
      if (!Buffer.isBuffer(b)) throw new TypeError("Argument must be a Buffer");
      if (this === b) return true;
      return Buffer.compare(this, b) === 0;
    });
    Buffer.prototype.inspect = $(function inspect() {
      let str = "";
      const max = exports.INSPECT_MAX_BYTES;
      str = this.toString("hex", 0, max).replace(/(.{2})/g, "$1 ").trim();
      if (this.length > max) str += " ... ";
      return "<Buffer " + str + ">";
    });
    if (customInspectSymbol) {
      Buffer.prototype[customInspectSymbol] = Buffer.prototype.inspect;
    }
    Buffer.prototype.compare = $(function compare(target, start, end, thisStart, thisEnd) {
      if (isInstance(target, Uint8Array)) {
        target = Buffer.from(target, target.offset, target.byteLength);
      }
      if (!Buffer.isBuffer(target)) {
        throw new TypeError('The "target" argument must be one of type Buffer or Uint8Array. ' + "Received type " + typeof target);
      }
      if (start === undefined) {
        start = 0;
      }
      if (end === undefined) {
        end = target ? target.length : 0;
      }
      if (thisStart === undefined) {
        thisStart = 0;
      }
      if (thisEnd === undefined) {
        thisEnd = this.length;
      }
      if (start < 0 || end > target.length || thisStart < 0 || thisEnd > this.length) {
        throw new RangeError("out of range index");
      }
      if (thisStart >= thisEnd && start >= end) {
        return 0;
      }
      if (thisStart >= thisEnd) {
        return -1;
      }
      if (start >= end) {
        return 1;
      }
      start >>>= 0;
      end >>>= 0;
      thisStart >>>= 0;
      thisEnd >>>= 0;
      if (this === target) return 0;
      let x = thisEnd - thisStart;
      let y = end - start;
      const len = Math.min(x, y);
      const thisCopy = this.slice(thisStart, thisEnd);
      const targetCopy = target.slice(start, end);
      for (let i = 0; i < len; ++i) {
        if (thisCopy[i] !== targetCopy[i]) {
          x = thisCopy[i];
          y = targetCopy[i];
          break;
        }
      }
      if (x < y) return -1;
      if (y < x) return 1;
      return 0;
    });
    function bidirectionalIndexOf(buffer, val, byteOffset, encoding, dir) {
      if (buffer.length === 0) return -1;
      if (typeof byteOffset === "string") {
        encoding = byteOffset;
        byteOffset = 0;
      } else if (byteOffset > 2147483647) {
        byteOffset = 2147483647;
      } else if (byteOffset < -2147483648) {
        byteOffset = -2147483648;
      }
      byteOffset = +byteOffset;
      if (numberIsNaN(byteOffset)) {
        byteOffset = dir ? 0 : buffer.length - 1;
      }
      if (byteOffset < 0) byteOffset = buffer.length + byteOffset;
      if (byteOffset >= buffer.length) {
        if (dir) return -1;else byteOffset = buffer.length - 1;
      } else if (byteOffset < 0) {
        if (dir) byteOffset = 0;else return -1;
      }
      if (typeof val === "string") {
        val = Buffer.from(val, encoding);
      }
      if (Buffer.isBuffer(val)) {
        if (val.length === 0) {
          return -1;
        }
        return arrayIndexOf(buffer, val, byteOffset, encoding, dir);
      } else if (typeof val === "number") {
        val = val & 255;
        if (typeof Uint8Array.prototype.indexOf === "function") {
          if (dir) {
            return Uint8Array.prototype.indexOf.call(buffer, val, byteOffset);
          } else {
            return Uint8Array.prototype.lastIndexOf.call(buffer, val, byteOffset);
          }
        }
        return arrayIndexOf(buffer, $Array.of(val), byteOffset, encoding, dir);
      }
      throw new TypeError("val must be string, number or Buffer");
    }
    $(bidirectionalIndexOf);
    function arrayIndexOf(arr, val, byteOffset, encoding, dir) {
      let indexSize = 1;
      let arrLength = arr.length;
      let valLength = val.length;
      if (encoding !== undefined) {
        encoding = String(encoding).toLowerCase();
        if (encoding === "ucs2" || encoding === "ucs-2" || encoding === "utf16le" || encoding === "utf-16le") {
          if (arr.length < 2 || val.length < 2) {
            return -1;
          }
          indexSize = 2;
          arrLength /= 2;
          valLength /= 2;
          byteOffset /= 2;
        }
      }
      function read(buf, i) {
        if (indexSize === 1) {
          return buf[i];
        } else {
          return buf.readUInt16BE(i * indexSize);
        }
      }
      $(read);
      let i;
      if (dir) {
        let foundIndex = -1;
        for (i = byteOffset; i < arrLength; i++) {
          if (read(arr, i) === read(val, foundIndex === -1 ? 0 : i - foundIndex)) {
            if (foundIndex === -1) foundIndex = i;
            if (i - foundIndex + 1 === valLength) return foundIndex * indexSize;
          } else {
            if (foundIndex !== -1) i -= i - foundIndex;
            foundIndex = -1;
          }
        }
      } else {
        if (byteOffset + valLength > arrLength) byteOffset = arrLength - valLength;
        for (i = byteOffset; i >= 0; i--) {
          let found = true;
          for (let j = 0; j < valLength; j++) {
            if (read(arr, i + j) !== read(val, j)) {
              found = false;
              break;
            }
          }
          if (found) return i;
        }
      }
      return -1;
    }
    $(arrayIndexOf);
    Buffer.prototype.includes = $(function includes(val, byteOffset, encoding) {
      return this.indexOf(val, byteOffset, encoding) !== -1;
    });
    Buffer.prototype.indexOf = $(function indexOf(val, byteOffset, encoding) {
      return bidirectionalIndexOf(this, val, byteOffset, encoding, true);
    });
    Buffer.prototype.lastIndexOf = $(function lastIndexOf(val, byteOffset, encoding) {
      return bidirectionalIndexOf(this, val, byteOffset, encoding, false);
    });
    function hexWrite(buf, string, offset, length) {
      offset = Number(offset) || 0;
      const remaining = buf.length - offset;
      if (!length) {
        length = remaining;
      } else {
        length = Number(length);
        if (length > remaining) {
          length = remaining;
        }
      }
      const strLen = string.length;
      if (length > strLen / 2) {
        length = strLen / 2;
      }
      let i;
      for (i = 0; i < length; ++i) {
        const parsed = parseInt(string.substr(i * 2, 2), 16);
        if (numberIsNaN(parsed)) return i;
        buf[offset + i] = parsed;
      }
      return i;
    }
    $(hexWrite);
    function utf8Write(buf, string, offset, length) {
      return blitBuffer(utf8ToBytes(string, buf.length - offset), buf, offset, length);
    }
    $(utf8Write);
    function asciiWrite(buf, string, offset, length) {
      return blitBuffer(asciiToBytes(string), buf, offset, length);
    }
    $(asciiWrite);
    function base64Write(buf, string, offset, length) {
      return blitBuffer(base64ToBytes(string), buf, offset, length);
    }
    $(base64Write);
    function ucs2Write(buf, string, offset, length) {
      return blitBuffer(utf16leToBytes(string, buf.length - offset), buf, offset, length);
    }
    $(ucs2Write);
    Buffer.prototype.write = $(function write(string, offset, length, encoding) {
      if (offset === undefined) {
        encoding = "utf8";
        length = this.length;
        offset = 0;
      } else if (length === undefined && typeof offset === "string") {
        encoding = offset;
        length = this.length;
        offset = 0;
      } else if (isFinite(offset)) {
        offset = offset >>> 0;
        if (isFinite(length)) {
          length = length >>> 0;
          if (encoding === undefined) encoding = "utf8";
        } else {
          encoding = length;
          length = undefined;
        }
      } else {
        throw new Error("Buffer.write(string, encoding, offset[, length]) is no longer supported");
      }
      const remaining = this.length - offset;
      if (length === undefined || length > remaining) length = remaining;
      if (string.length > 0 && (length < 0 || offset < 0) || offset > this.length) {
        throw new RangeError("Attempt to write outside buffer bounds");
      }
      if (!encoding) encoding = "utf8";
      let loweredCase = false;
      for (;;) {
        switch (encoding) {
          case "hex":
            return hexWrite(this, string, offset, length);
          case "utf8":
          case "utf-8":
            return utf8Write(this, string, offset, length);
          case "ascii":
          case "latin1":
          case "binary":
            return asciiWrite(this, string, offset, length);
          case "base64":
            return base64Write(this, string, offset, length);
          case "ucs2":
          case "ucs-2":
          case "utf16le":
          case "utf-16le":
            return ucs2Write(this, string, offset, length);
          default:
            if (loweredCase) throw new TypeError("Unknown encoding: " + encoding);
            encoding = ("" + encoding).toLowerCase();
            loweredCase = true;
        }
      }
    });
    Buffer.prototype.toJSON = $(function toJSON() {
      return $(function () {
        let result = $Object.create(null, undefined);
        result.type = "Buffer";
        result.data = $Array.prototype.slice.call(this._arr || this, 0);
        return result;
      })();
    });
    function base64Slice(buf, start, end) {
      if (start === 0 && end === buf.length) {
        return base64.fromByteArray(buf);
      } else {
        return base64.fromByteArray(buf.slice(start, end));
      }
    }
    $(base64Slice);
    function utf8Slice(buf, start, end) {
      end = Math.min(buf.length, end);
      const res = $Array.of();
      let i = start;
      while (i < end) {
        const firstByte = buf[i];
        let codePoint = null;
        let bytesPerSequence = firstByte > 239 ? 4 : firstByte > 223 ? 3 : firstByte > 191 ? 2 : 1;
        if (i + bytesPerSequence <= end) {
          let secondByte, thirdByte, fourthByte, tempCodePoint;
          switch (bytesPerSequence) {
            case 1:
              if (firstByte < 128) {
                codePoint = firstByte;
              }
              break;
            case 2:
              secondByte = buf[i + 1];
              if ((secondByte & 192) === 128) {
                tempCodePoint = (firstByte & 31) << 6 | secondByte & 63;
                if (tempCodePoint > 127) {
                  codePoint = tempCodePoint;
                }
              }
              break;
            case 3:
              secondByte = buf[i + 1];
              thirdByte = buf[i + 2];
              if ((secondByte & 192) === 128 && (thirdByte & 192) === 128) {
                tempCodePoint = (firstByte & 15) << 12 | (secondByte & 63) << 6 | thirdByte & 63;
                if (tempCodePoint > 2047 && (tempCodePoint < 55296 || tempCodePoint > 57343)) {
                  codePoint = tempCodePoint;
                }
              }
              break;
            case 4:
              secondByte = buf[i + 1];
              thirdByte = buf[i + 2];
              fourthByte = buf[i + 3];
              if ((secondByte & 192) === 128 && (thirdByte & 192) === 128 && (fourthByte & 192) === 128) {
                tempCodePoint = (firstByte & 15) << 18 | (secondByte & 63) << 12 | (thirdByte & 63) << 6 | fourthByte & 63;
                if (tempCodePoint > 65535 && tempCodePoint < 1114112) {
                  codePoint = tempCodePoint;
                }
              }
          }
        }
        if (codePoint === null) {
          codePoint = 65533;
          bytesPerSequence = 1;
        } else if (codePoint > 65535) {
          codePoint -= 65536;
          res.push(codePoint >>> 10 & 1023 | 55296);
          codePoint = 56320 | codePoint & 1023;
        }
        res.push(codePoint);
        i += bytesPerSequence;
      }
      return decodeCodePointsArray(res);
    }
    $(utf8Slice);
    const MAX_ARGUMENTS_LENGTH = 4096;
    function decodeCodePointsArray(codePoints) {
      const len = codePoints.length;
      if (len <= MAX_ARGUMENTS_LENGTH) {
        return String.fromCharCode.apply(String, codePoints);
      }
      let res = "";
      let i = 0;
      while (i < len) {
        res += String.fromCharCode.apply(String, codePoints.slice(i, i += MAX_ARGUMENTS_LENGTH));
      }
      return res;
    }
    $(decodeCodePointsArray);
    function asciiSlice(buf, start, end) {
      let ret = "";
      end = Math.min(buf.length, end);
      for (let i = start; i < end; ++i) {
        ret += String.fromCharCode(buf[i] & 127);
      }
      return ret;
    }
    $(asciiSlice);
    function latin1Slice(buf, start, end) {
      let ret = "";
      end = Math.min(buf.length, end);
      for (let i = start; i < end; ++i) {
        ret += String.fromCharCode(buf[i]);
      }
      return ret;
    }
    $(latin1Slice);
    function hexSlice(buf, start, end) {
      const len = buf.length;
      if (!start || start < 0) start = 0;
      if (!end || end < 0 || end > len) end = len;
      let out = "";
      for (let i = start; i < end; ++i) {
        out += hexSliceLookupTable[buf[i]];
      }
      return out;
    }
    $(hexSlice);
    function utf16leSlice(buf, start, end) {
      const bytes = buf.slice(start, end);
      let res = "";
      for (let i = 0; i < bytes.length - 1; i += 2) {
        res += String.fromCharCode(bytes[i] + bytes[i + 1] * 256);
      }
      return res;
    }
    $(utf16leSlice);
    Buffer.prototype.slice = $(function slice(start, end) {
      const len = this.length;
      start = ~~start;
      end = end === undefined ? len : ~~end;
      if (start < 0) {
        start += len;
        if (start < 0) start = 0;
      } else if (start > len) {
        start = len;
      }
      if (end < 0) {
        end += len;
        if (end < 0) end = 0;
      } else if (end > len) {
        end = len;
      }
      if (end < start) end = start;
      const newBuf = this.subarray(start, end);
      $Object.setPrototypeOf(newBuf, Buffer.prototype);
      return newBuf;
    });
    function checkOffset(offset, ext, length) {
      if (offset % 1 !== 0 || offset < 0) throw new RangeError("offset is not uint");
      if (offset + ext > length) throw new RangeError("Trying to access beyond buffer length");
    }
    $(checkOffset);
    Buffer.prototype.readUintLE = Buffer.prototype.readUIntLE = $(function readUIntLE(offset, byteLength, noAssert) {
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) checkOffset(offset, byteLength, this.length);
      let val = this[offset];
      let mul = 1;
      let i = 0;
      while (++i < byteLength && (mul *= 256)) {
        val += this[offset + i] * mul;
      }
      return val;
    });
    Buffer.prototype.readUintBE = Buffer.prototype.readUIntBE = $(function readUIntBE(offset, byteLength, noAssert) {
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) {
        checkOffset(offset, byteLength, this.length);
      }
      let val = this[offset + --byteLength];
      let mul = 1;
      while (byteLength > 0 && (mul *= 256)) {
        val += this[offset + --byteLength] * mul;
      }
      return val;
    });
    Buffer.prototype.readUint8 = Buffer.prototype.readUInt8 = $(function readUInt8(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 1, this.length);
      return this[offset];
    });
    Buffer.prototype.readUint16LE = Buffer.prototype.readUInt16LE = $(function readUInt16LE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 2, this.length);
      return this[offset] | this[offset + 1] << 8;
    });
    Buffer.prototype.readUint16BE = Buffer.prototype.readUInt16BE = $(function readUInt16BE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 2, this.length);
      return this[offset] << 8 | this[offset + 1];
    });
    Buffer.prototype.readUint32LE = Buffer.prototype.readUInt32LE = $(function readUInt32LE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return (this[offset] | this[offset + 1] << 8 | this[offset + 2] << 16) + this[offset + 3] * 16777216;
    });
    Buffer.prototype.readUint32BE = Buffer.prototype.readUInt32BE = $(function readUInt32BE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return this[offset] * 16777216 + (this[offset + 1] << 16 | this[offset + 2] << 8 | this[offset + 3]);
    });
    Buffer.prototype.readBigUInt64LE = defineBigIntMethod($(function readBigUInt64LE(offset) {
      offset = offset >>> 0;
      validateNumber(offset, "offset");
      const first = this[offset];
      const last = this[offset + 7];
      if (first === undefined || last === undefined) {
        boundsError(offset, this.length - 8);
      }
      const lo = first + this[++offset] * 2 ** 8 + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 24;
      const hi = this[++offset] + this[++offset] * 2 ** 8 + this[++offset] * 2 ** 16 + last * 2 ** 24;
      return BigInt(lo) + (BigInt(hi) << BigInt(32));
    }));
    Buffer.prototype.readBigUInt64BE = defineBigIntMethod($(function readBigUInt64BE(offset) {
      offset = offset >>> 0;
      validateNumber(offset, "offset");
      const first = this[offset];
      const last = this[offset + 7];
      if (first === undefined || last === undefined) {
        boundsError(offset, this.length - 8);
      }
      const hi = first * 2 ** 24 + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 8 + this[++offset];
      const lo = this[++offset] * 2 ** 24 + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 8 + last;
      return (BigInt(hi) << BigInt(32)) + BigInt(lo);
    }));
    Buffer.prototype.readIntLE = $(function readIntLE(offset, byteLength, noAssert) {
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) checkOffset(offset, byteLength, this.length);
      let val = this[offset];
      let mul = 1;
      let i = 0;
      while (++i < byteLength && (mul *= 256)) {
        val += this[offset + i] * mul;
      }
      mul *= 128;
      if (val >= mul) val -= Math.pow(2, 8 * byteLength);
      return val;
    });
    Buffer.prototype.readIntBE = $(function readIntBE(offset, byteLength, noAssert) {
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) checkOffset(offset, byteLength, this.length);
      let i = byteLength;
      let mul = 1;
      let val = this[offset + --i];
      while (i > 0 && (mul *= 256)) {
        val += this[offset + --i] * mul;
      }
      mul *= 128;
      if (val >= mul) val -= Math.pow(2, 8 * byteLength);
      return val;
    });
    Buffer.prototype.readInt8 = $(function readInt8(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 1, this.length);
      if (!(this[offset] & 128)) return this[offset];
      return (255 - this[offset] + 1) * -1;
    });
    Buffer.prototype.readInt16LE = $(function readInt16LE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 2, this.length);
      const val = this[offset] | this[offset + 1] << 8;
      return val & 32768 ? val | 4294901760 : val;
    });
    Buffer.prototype.readInt16BE = $(function readInt16BE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 2, this.length);
      const val = this[offset + 1] | this[offset] << 8;
      return val & 32768 ? val | 4294901760 : val;
    });
    Buffer.prototype.readInt32LE = $(function readInt32LE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return this[offset] | this[offset + 1] << 8 | this[offset + 2] << 16 | this[offset + 3] << 24;
    });
    Buffer.prototype.readInt32BE = $(function readInt32BE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return this[offset] << 24 | this[offset + 1] << 16 | this[offset + 2] << 8 | this[offset + 3];
    });
    Buffer.prototype.readBigInt64LE = defineBigIntMethod($(function readBigInt64LE(offset) {
      offset = offset >>> 0;
      validateNumber(offset, "offset");
      const first = this[offset];
      const last = this[offset + 7];
      if (first === undefined || last === undefined) {
        boundsError(offset, this.length - 8);
      }
      const val = this[offset + 4] + this[offset + 5] * 2 ** 8 + this[offset + 6] * 2 ** 16 + (last << 24);
      return (BigInt(val) << BigInt(32)) + BigInt(first + this[++offset] * 2 ** 8 + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 24);
    }));
    Buffer.prototype.readBigInt64BE = defineBigIntMethod($(function readBigInt64BE(offset) {
      offset = offset >>> 0;
      validateNumber(offset, "offset");
      const first = this[offset];
      const last = this[offset + 7];
      if (first === undefined || last === undefined) {
        boundsError(offset, this.length - 8);
      }
      const val = (first << 24) + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 8 + this[++offset];
      return (BigInt(val) << BigInt(32)) + BigInt(this[++offset] * 2 ** 24 + this[++offset] * 2 ** 16 + this[++offset] * 2 ** 8 + last);
    }));
    Buffer.prototype.readFloatLE = $(function readFloatLE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return ieee754$1.read(this, offset, true, 23, 4);
    });
    Buffer.prototype.readFloatBE = $(function readFloatBE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 4, this.length);
      return ieee754$1.read(this, offset, false, 23, 4);
    });
    Buffer.prototype.readDoubleLE = $(function readDoubleLE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 8, this.length);
      return ieee754$1.read(this, offset, true, 52, 8);
    });
    Buffer.prototype.readDoubleBE = $(function readDoubleBE(offset, noAssert) {
      offset = offset >>> 0;
      if (!noAssert) checkOffset(offset, 8, this.length);
      return ieee754$1.read(this, offset, false, 52, 8);
    });
    function checkInt(buf, value, offset, ext, max, min) {
      if (!Buffer.isBuffer(buf)) throw new TypeError('"buffer" argument must be a Buffer instance');
      if (value > max || value < min) throw new RangeError('"value" argument is out of bounds');
      if (offset + ext > buf.length) throw new RangeError("Index out of range");
    }
    $(checkInt);
    Buffer.prototype.writeUintLE = Buffer.prototype.writeUIntLE = $(function writeUIntLE(value, offset, byteLength, noAssert) {
      value = +value;
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) {
        const maxBytes = Math.pow(2, 8 * byteLength) - 1;
        checkInt(this, value, offset, byteLength, maxBytes, 0);
      }
      let mul = 1;
      let i = 0;
      this[offset] = value & 255;
      while (++i < byteLength && (mul *= 256)) {
        this[offset + i] = value / mul & 255;
      }
      return offset + byteLength;
    });
    Buffer.prototype.writeUintBE = Buffer.prototype.writeUIntBE = $(function writeUIntBE(value, offset, byteLength, noAssert) {
      value = +value;
      offset = offset >>> 0;
      byteLength = byteLength >>> 0;
      if (!noAssert) {
        const maxBytes = Math.pow(2, 8 * byteLength) - 1;
        checkInt(this, value, offset, byteLength, maxBytes, 0);
      }
      let i = byteLength - 1;
      let mul = 1;
      this[offset + i] = value & 255;
      while (--i >= 0 && (mul *= 256)) {
        this[offset + i] = value / mul & 255;
      }
      return offset + byteLength;
    });
    Buffer.prototype.writeUint8 = Buffer.prototype.writeUInt8 = $(function writeUInt8(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 1, 255, 0);
      this[offset] = value & 255;
      return offset + 1;
    });
    Buffer.prototype.writeUint16LE = Buffer.prototype.writeUInt16LE = $(function writeUInt16LE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 2, 65535, 0);
      this[offset] = value & 255;
      this[offset + 1] = value >>> 8;
      return offset + 2;
    });
    Buffer.prototype.writeUint16BE = Buffer.prototype.writeUInt16BE = $(function writeUInt16BE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 2, 65535, 0);
      this[offset] = value >>> 8;
      this[offset + 1] = value & 255;
      return offset + 2;
    });
    Buffer.prototype.writeUint32LE = Buffer.prototype.writeUInt32LE = $(function writeUInt32LE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 4, 4294967295, 0);
      this[offset + 3] = value >>> 24;
      this[offset + 2] = value >>> 16;
      this[offset + 1] = value >>> 8;
      this[offset] = value & 255;
      return offset + 4;
    });
    Buffer.prototype.writeUint32BE = Buffer.prototype.writeUInt32BE = $(function writeUInt32BE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 4, 4294967295, 0);
      this[offset] = value >>> 24;
      this[offset + 1] = value >>> 16;
      this[offset + 2] = value >>> 8;
      this[offset + 3] = value & 255;
      return offset + 4;
    });
    function wrtBigUInt64LE(buf, value, offset, min, max) {
      checkIntBI(value, min, max, buf, offset, 7);
      let lo = Number(value & BigInt(4294967295));
      buf[offset++] = lo;
      lo = lo >> 8;
      buf[offset++] = lo;
      lo = lo >> 8;
      buf[offset++] = lo;
      lo = lo >> 8;
      buf[offset++] = lo;
      let hi = Number(value >> BigInt(32) & BigInt(4294967295));
      buf[offset++] = hi;
      hi = hi >> 8;
      buf[offset++] = hi;
      hi = hi >> 8;
      buf[offset++] = hi;
      hi = hi >> 8;
      buf[offset++] = hi;
      return offset;
    }
    $(wrtBigUInt64LE);
    function wrtBigUInt64BE(buf, value, offset, min, max) {
      checkIntBI(value, min, max, buf, offset, 7);
      let lo = Number(value & BigInt(4294967295));
      buf[offset + 7] = lo;
      lo = lo >> 8;
      buf[offset + 6] = lo;
      lo = lo >> 8;
      buf[offset + 5] = lo;
      lo = lo >> 8;
      buf[offset + 4] = lo;
      let hi = Number(value >> BigInt(32) & BigInt(4294967295));
      buf[offset + 3] = hi;
      hi = hi >> 8;
      buf[offset + 2] = hi;
      hi = hi >> 8;
      buf[offset + 1] = hi;
      hi = hi >> 8;
      buf[offset] = hi;
      return offset + 8;
    }
    $(wrtBigUInt64BE);
    Buffer.prototype.writeBigUInt64LE = defineBigIntMethod($(function writeBigUInt64LE(value, offset = 0) {
      return wrtBigUInt64LE(this, value, offset, BigInt(0), BigInt("0xffffffffffffffff"));
    }));
    Buffer.prototype.writeBigUInt64BE = defineBigIntMethod($(function writeBigUInt64BE(value, offset = 0) {
      return wrtBigUInt64BE(this, value, offset, BigInt(0), BigInt("0xffffffffffffffff"));
    }));
    Buffer.prototype.writeIntLE = $(function writeIntLE(value, offset, byteLength, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) {
        const limit = Math.pow(2, 8 * byteLength - 1);
        checkInt(this, value, offset, byteLength, limit - 1, -limit);
      }
      let i = 0;
      let mul = 1;
      let sub = 0;
      this[offset] = value & 255;
      while (++i < byteLength && (mul *= 256)) {
        if (value < 0 && sub === 0 && this[offset + i - 1] !== 0) {
          sub = 1;
        }
        this[offset + i] = (value / mul >> 0) - sub & 255;
      }
      return offset + byteLength;
    });
    Buffer.prototype.writeIntBE = $(function writeIntBE(value, offset, byteLength, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) {
        const limit = Math.pow(2, 8 * byteLength - 1);
        checkInt(this, value, offset, byteLength, limit - 1, -limit);
      }
      let i = byteLength - 1;
      let mul = 1;
      let sub = 0;
      this[offset + i] = value & 255;
      while (--i >= 0 && (mul *= 256)) {
        if (value < 0 && sub === 0 && this[offset + i + 1] !== 0) {
          sub = 1;
        }
        this[offset + i] = (value / mul >> 0) - sub & 255;
      }
      return offset + byteLength;
    });
    Buffer.prototype.writeInt8 = $(function writeInt8(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 1, 127, -128);
      if (value < 0) value = 255 + value + 1;
      this[offset] = value & 255;
      return offset + 1;
    });
    Buffer.prototype.writeInt16LE = $(function writeInt16LE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 2, 32767, -32768);
      this[offset] = value & 255;
      this[offset + 1] = value >>> 8;
      return offset + 2;
    });
    Buffer.prototype.writeInt16BE = $(function writeInt16BE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 2, 32767, -32768);
      this[offset] = value >>> 8;
      this[offset + 1] = value & 255;
      return offset + 2;
    });
    Buffer.prototype.writeInt32LE = $(function writeInt32LE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 4, 2147483647, -2147483648);
      this[offset] = value & 255;
      this[offset + 1] = value >>> 8;
      this[offset + 2] = value >>> 16;
      this[offset + 3] = value >>> 24;
      return offset + 4;
    });
    Buffer.prototype.writeInt32BE = $(function writeInt32BE(value, offset, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) checkInt(this, value, offset, 4, 2147483647, -2147483648);
      if (value < 0) value = 4294967295 + value + 1;
      this[offset] = value >>> 24;
      this[offset + 1] = value >>> 16;
      this[offset + 2] = value >>> 8;
      this[offset + 3] = value & 255;
      return offset + 4;
    });
    Buffer.prototype.writeBigInt64LE = defineBigIntMethod($(function writeBigInt64LE(value, offset = 0) {
      return wrtBigUInt64LE(this, value, offset, -BigInt("0x8000000000000000"), BigInt("0x7fffffffffffffff"));
    }));
    Buffer.prototype.writeBigInt64BE = defineBigIntMethod($(function writeBigInt64BE(value, offset = 0) {
      return wrtBigUInt64BE(this, value, offset, -BigInt("0x8000000000000000"), BigInt("0x7fffffffffffffff"));
    }));
    function checkIEEE754(buf, value, offset, ext, max, min) {
      if (offset + ext > buf.length) throw new RangeError("Index out of range");
      if (offset < 0) throw new RangeError("Index out of range");
    }
    $(checkIEEE754);
    function writeFloat(buf, value, offset, littleEndian, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) {
        checkIEEE754(buf, value, offset, 4);
      }
      ieee754$1.write(buf, value, offset, littleEndian, 23, 4);
      return offset + 4;
    }
    $(writeFloat);
    Buffer.prototype.writeFloatLE = $(function writeFloatLE(value, offset, noAssert) {
      return writeFloat(this, value, offset, true, noAssert);
    });
    Buffer.prototype.writeFloatBE = $(function writeFloatBE(value, offset, noAssert) {
      return writeFloat(this, value, offset, false, noAssert);
    });
    function writeDouble(buf, value, offset, littleEndian, noAssert) {
      value = +value;
      offset = offset >>> 0;
      if (!noAssert) {
        checkIEEE754(buf, value, offset, 8);
      }
      ieee754$1.write(buf, value, offset, littleEndian, 52, 8);
      return offset + 8;
    }
    $(writeDouble);
    Buffer.prototype.writeDoubleLE = $(function writeDoubleLE(value, offset, noAssert) {
      return writeDouble(this, value, offset, true, noAssert);
    });
    Buffer.prototype.writeDoubleBE = $(function writeDoubleBE(value, offset, noAssert) {
      return writeDouble(this, value, offset, false, noAssert);
    });
    Buffer.prototype.copy = $(function copy(target, targetStart, start, end) {
      if (!Buffer.isBuffer(target)) throw new TypeError("argument should be a Buffer");
      if (!start) start = 0;
      if (!end && end !== 0) end = this.length;
      if (targetStart >= target.length) targetStart = target.length;
      if (!targetStart) targetStart = 0;
      if (end > 0 && end < start) end = start;
      if (end === start) return 0;
      if (target.length === 0 || this.length === 0) return 0;
      if (targetStart < 0) {
        throw new RangeError("targetStart out of bounds");
      }
      if (start < 0 || start >= this.length) throw new RangeError("Index out of range");
      if (end < 0) throw new RangeError("sourceEnd out of bounds");
      if (end > this.length) end = this.length;
      if (target.length - targetStart < end - start) {
        end = target.length - targetStart + start;
      }
      const len = end - start;
      if (this === target && typeof Uint8Array.prototype.copyWithin === "function") {
        this.copyWithin(targetStart, start, end);
      } else {
        Uint8Array.prototype.set.call(target, this.subarray(start, end), targetStart);
      }
      return len;
    });
    Buffer.prototype.fill = $(function fill(val, start, end, encoding) {
      if (typeof val === "string") {
        if (typeof start === "string") {
          encoding = start;
          start = 0;
          end = this.length;
        } else if (typeof end === "string") {
          encoding = end;
          end = this.length;
        }
        if (encoding !== undefined && typeof encoding !== "string") {
          throw new TypeError("encoding must be a string");
        }
        if (typeof encoding === "string" && !Buffer.isEncoding(encoding)) {
          throw new TypeError("Unknown encoding: " + encoding);
        }
        if (val.length === 1) {
          const code = val.charCodeAt(0);
          if (encoding === "utf8" && code < 128 || encoding === "latin1") {
            val = code;
          }
        }
      } else if (typeof val === "number") {
        val = val & 255;
      } else if (typeof val === "boolean") {
        val = Number(val);
      }
      if (start < 0 || this.length < start || this.length < end) {
        throw new RangeError("Out of range index");
      }
      if (end <= start) {
        return this;
      }
      start = start >>> 0;
      end = end === undefined ? this.length : end >>> 0;
      if (!val) val = 0;
      let i;
      if (typeof val === "number") {
        for (i = start; i < end; ++i) {
          this[i] = val;
        }
      } else {
        const bytes = Buffer.isBuffer(val) ? val : Buffer.from(val, encoding);
        const len = bytes.length;
        if (len === 0) {
          throw new TypeError('The value "' + val + '" is invalid for argument "value"');
        }
        for (i = 0; i < end - start; ++i) {
          this[i + start] = bytes[i % len];
        }
      }
      return this;
    });
    const errors = $Object.create(null, undefined);
    function E(sym, getMessage, Base) {
      errors[sym] = class NodeError extends Base {
        constructor() {
          super();
          $Object.defineProperty(this, "message", $(function () {
            let result = $Object.create(null, undefined);
            result.value = getMessage.apply(this, arguments);
            result.writable = true;
            result.configurable = true;
            return result;
          }).bind(this)());
          this.name = `${this.name} [${sym}]`;
          this.stack;
          delete this.name;
        }
        get code() {
          return sym;
        }
        set code(value) {
          $Object.defineProperty(this, "code", $(function () {
            let result = $Object.create(null, undefined);
            result.configurable = true;
            result.enumerable = true;
            result.value = value;
            result.writable = true;
            return result;
          })());
        }
        toString() {
          return `${this.name} [${sym}]: ${this.message}`;
        }
      };
    }
    $(E);
    E("ERR_BUFFER_OUT_OF_BOUNDS", $(function (name) {
      if (name) {
        return `${name} is outside of buffer bounds`;
      }
      return "Attempt to access memory outside buffer bounds";
    }), RangeError);
    E("ERR_INVALID_ARG_TYPE", $(function (name, actual) {
      return `The "${name}" argument must be of type number. Received type ${typeof actual}`;
    }), TypeError);
    E("ERR_OUT_OF_RANGE", $(function (str, range, input) {
      let msg = `The value of "${str}" is out of range.`;
      let received = input;
      if (Number.isInteger(input) && Math.abs(input) > 2 ** 32) {
        received = addNumericalSeparator(String(input));
      } else if (typeof input === "bigint") {
        received = String(input);
        if (input > BigInt(2) ** BigInt(32) || input < -(BigInt(2) ** BigInt(32))) {
          received = addNumericalSeparator(received);
        }
        received += "n";
      }
      msg += ` It must be ${range}. Received ${received}`;
      return msg;
    }), RangeError);
    function addNumericalSeparator(val) {
      let res = "";
      let i = val.length;
      const start = val[0] === "-" ? 1 : 0;
      for (; i >= start + 4; i -= 3) {
        res = `_${val.slice(i - 3, i)}${res}`;
      }
      return `${val.slice(0, i)}${res}`;
    }
    $(addNumericalSeparator);
    function checkBounds(buf, offset, byteLength) {
      validateNumber(offset, "offset");
      if (buf[offset] === undefined || buf[offset + byteLength] === undefined) {
        boundsError(offset, buf.length - (byteLength + 1));
      }
    }
    $(checkBounds);
    function checkIntBI(value, min, max, buf, offset, byteLength) {
      if (value > max || value < min) {
        const n = typeof min === "bigint" ? "n" : "";
        let range;
        if (byteLength > 3) {
          if (min === 0 || min === BigInt(0)) {
            range = `>= 0${n} and < 2${n} ** ${(byteLength + 1) * 8}${n}`;
          } else {
            range = `>= -(2${n} ** ${(byteLength + 1) * 8 - 1}${n}) and < 2 ** ` + `${(byteLength + 1) * 8 - 1}${n}`;
          }
        } else {
          range = `>= ${min}${n} and <= ${max}${n}`;
        }
        throw new errors.ERR_OUT_OF_RANGE("value", range, value);
      }
      checkBounds(buf, offset, byteLength);
    }
    $(checkIntBI);
    function validateNumber(value, name) {
      if (typeof value !== "number") {
        throw new errors.ERR_INVALID_ARG_TYPE(name, "number", value);
      }
    }
    $(validateNumber);
    function boundsError(value, length, type) {
      if (Math.floor(value) !== value) {
        validateNumber(value, type);
        throw new errors.ERR_OUT_OF_RANGE(type || "offset", "an integer", value);
      }
      if (length < 0) {
        throw new errors.ERR_BUFFER_OUT_OF_BOUNDS();
      }
      throw new errors.ERR_OUT_OF_RANGE(type || "offset", `>= ${type ? 1 : 0} and <= ${length}`, value);
    }
    $(boundsError);
    const INVALID_BASE64_RE = /[^+/0-9A-Za-z-_]/g;
    function base64clean(str) {
      str = str.split("=")[0];
      str = str.trim().replace(INVALID_BASE64_RE, "");
      if (str.length < 2) return "";
      while (str.length % 4 !== 0) {
        str = str + "=";
      }
      return str;
    }
    $(base64clean);
    function utf8ToBytes(string, units) {
      units = units || Infinity;
      let codePoint;
      const length = string.length;
      let leadSurrogate = null;
      const bytes = $Array.of();
      for (let i = 0; i < length; ++i) {
        codePoint = string.charCodeAt(i);
        if (codePoint > 55295 && codePoint < 57344) {
          if (!leadSurrogate) {
            if (codePoint > 56319) {
              if ((units -= 3) > -1) bytes.push(239, 191, 189);
              continue;
            } else if (i + 1 === length) {
              if ((units -= 3) > -1) bytes.push(239, 191, 189);
              continue;
            }
            leadSurrogate = codePoint;
            continue;
          }
          if (codePoint < 56320) {
            if ((units -= 3) > -1) bytes.push(239, 191, 189);
            leadSurrogate = codePoint;
            continue;
          }
          codePoint = (leadSurrogate - 55296 << 10 | codePoint - 56320) + 65536;
        } else if (leadSurrogate) {
          if ((units -= 3) > -1) bytes.push(239, 191, 189);
        }
        leadSurrogate = null;
        if (codePoint < 128) {
          if ((units -= 1) < 0) break;
          bytes.push(codePoint);
        } else if (codePoint < 2048) {
          if ((units -= 2) < 0) break;
          bytes.push(codePoint >> 6 | 192, codePoint & 63 | 128);
        } else if (codePoint < 65536) {
          if ((units -= 3) < 0) break;
          bytes.push(codePoint >> 12 | 224, codePoint >> 6 & 63 | 128, codePoint & 63 | 128);
        } else if (codePoint < 1114112) {
          if ((units -= 4) < 0) break;
          bytes.push(codePoint >> 18 | 240, codePoint >> 12 & 63 | 128, codePoint >> 6 & 63 | 128, codePoint & 63 | 128);
        } else {
          throw new Error("Invalid code point");
        }
      }
      return bytes;
    }
    $(utf8ToBytes);
    function asciiToBytes(str) {
      const byteArray = $Array.of();
      for (let i = 0; i < str.length; ++i) {
        byteArray.push(str.charCodeAt(i) & 255);
      }
      return byteArray;
    }
    $(asciiToBytes);
    function utf16leToBytes(str, units) {
      let c, hi, lo;
      const byteArray = $Array.of();
      for (let i = 0; i < str.length; ++i) {
        if ((units -= 2) < 0) break;
        c = str.charCodeAt(i);
        hi = c >> 8;
        lo = c % 256;
        byteArray.push(lo);
        byteArray.push(hi);
      }
      return byteArray;
    }
    $(utf16leToBytes);
    function base64ToBytes(str) {
      return base64.toByteArray(base64clean(str));
    }
    $(base64ToBytes);
    function blitBuffer(src, dst, offset, length) {
      let i;
      for (i = 0; i < length; ++i) {
        if (i + offset >= dst.length || i >= src.length) break;
        dst[i + offset] = src[i];
      }
      return i;
    }
    $(blitBuffer);
    function isInstance(obj, type) {
      return obj instanceof type || obj != null && obj.constructor != null && obj.constructor.name != null && obj.constructor.name === type.name;
    }
    $(isInstance);
    function numberIsNaN(obj) {
      return obj !== obj;
    }
    $(numberIsNaN);
    const hexSliceLookupTable = $(function () {
      const alphabet = "0123456789abcdef";
      const table = new Array(256);
      for (let i = 0; i < 16; ++i) {
        const i16 = i * 16;
        for (let j = 0; j < 16; ++j) {
          table[i16 + j] = alphabet[i] + alphabet[j];
        }
      }
      return table;
    })();
    function defineBigIntMethod(fn) {
      return typeof BigInt === "undefined" ? BufferBigIntNotDefined : fn;
    }
    $(defineBigIntMethod);
    function BufferBigIntNotDefined() {
      throw new Error("BigInt not supported");
    }
    $(BufferBigIntNotDefined);
  })(buffer);
  function number$2(n) {
    if (!Number.isSafeInteger(n) || n < 0) throw new Error(`positive integer expected, not ${n}`);
  }
  $(number$2);
  function isBytes$3(a) {
    return a instanceof Uint8Array || a != null && typeof a === "object" && a.constructor.name === "Uint8Array";
  }
  $(isBytes$3);
  function bytes$1(b, ...lengths) {
    if (!isBytes$3(b)) throw new Error("Uint8Array expected");
    if (lengths.length > 0 && !lengths.includes(b.length)) throw new Error(`Uint8Array expected of length ${lengths}, not of length=${b.length}`);
  }
  $(bytes$1);
  function hash(h) {
    if (typeof h !== "function" || typeof h.create !== "function") throw new Error("Hash should be wrapped by utils.wrapConstructor");
    number$2(h.outputLen);
    number$2(h.blockLen);
  }
  $(hash);
  function exists$1(instance, checkFinished = true) {
    if (instance.destroyed) throw new Error("Hash instance has been destroyed");
    if (checkFinished && instance.finished) throw new Error("Hash#digest() has already been called");
  }
  $(exists$1);
  function output$1(out, instance) {
    bytes$1(out);
    const min = instance.outputLen;
    if (out.length < min) {
      throw new Error(`digestInto() expects output buffer of length at least ${min}`);
    }
  }
  $(output$1);
  const crypto$1 = typeof globalThis === "object" && "crypto" in globalThis ? globalThis.crypto : undefined;
  /*! noble-hashes - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  const createView$1 = arr => new DataView(arr.buffer, arr.byteOffset, arr.byteLength);
  $(createView$1);
  const rotr$1 = (word, shift) => word << 32 - shift | word >>> shift;
  $(rotr$1);
  new Uint8Array(new Uint32Array($Array.of(287454020)).buffer)[0] === 68;
  function utf8ToBytes$2(str) {
    if (typeof str !== "string") throw new Error(`utf8ToBytes expected string, got ${typeof str}`);
    return new Uint8Array(new TextEncoder().encode(str));
  }
  $(utf8ToBytes$2);
  function toBytes$1(data) {
    if (typeof data === "string") data = utf8ToBytes$2(data);
    bytes$1(data);
    return data;
  }
  $(toBytes$1);
  function concatBytes$1(...arrays) {
    let sum = 0;
    for (let i = 0; i < arrays.length; i++) {
      const a = arrays[i];
      bytes$1(a);
      sum += a.length;
    }
    const res = new Uint8Array(sum);
    for (let i = 0, pad = 0; i < arrays.length; i++) {
      const a = arrays[i];
      res.set(a, pad);
      pad += a.length;
    }
    return res;
  }
  $(concatBytes$1);
  let Hash$1 = class Hash {
    clone() {
      return this._cloneInto();
    }
  };
  function wrapConstructor$1(hashCons) {
    const hashC = msg => hashCons().update(toBytes$1(msg)).digest();
    $(hashC);
    const tmp = hashCons();
    hashC.outputLen = tmp.outputLen;
    hashC.blockLen = tmp.blockLen;
    hashC.create = $(() => hashCons());
    return hashC;
  }
  $(wrapConstructor$1);
  function randomBytes(bytesLength = 32) {
    if (crypto$1 && typeof crypto$1.getRandomValues === "function") {
      return crypto$1.getRandomValues(new Uint8Array(bytesLength));
    }
    throw new Error("crypto.getRandomValues must be defined");
  }
  $(randomBytes);
  function setBigUint64$1(view, byteOffset, value, isLE) {
    if (typeof view.setBigUint64 === "function") return view.setBigUint64(byteOffset, value, isLE);
    const _32n = BigInt(32);
    const _u32_max = BigInt(4294967295);
    const wh = Number(value >> _32n & _u32_max);
    const wl = Number(value & _u32_max);
    const h = isLE ? 4 : 0;
    const l = isLE ? 0 : 4;
    view.setUint32(byteOffset + h, wh, isLE);
    view.setUint32(byteOffset + l, wl, isLE);
  }
  $(setBigUint64$1);
  const Chi$1 = (a, b, c) => a & b ^ ~a & c;
  $(Chi$1);
  const Maj$1 = (a, b, c) => a & b ^ a & c ^ b & c;
  $(Maj$1);
  class HashMD extends Hash$1 {
    constructor(blockLen, outputLen, padOffset, isLE) {
      super();
      this.blockLen = blockLen;
      this.outputLen = outputLen;
      this.padOffset = padOffset;
      this.isLE = isLE;
      this.finished = false;
      this.length = 0;
      this.pos = 0;
      this.destroyed = false;
      this.buffer = new Uint8Array(blockLen);
      this.view = createView$1(this.buffer);
    }
    update(data) {
      exists$1(this);
      const {
        view: view,
        buffer: buffer,
        blockLen: blockLen
      } = this;
      data = toBytes$1(data);
      const len = data.length;
      for (let pos = 0; pos < len;) {
        const take = Math.min(blockLen - this.pos, len - pos);
        if (take === blockLen) {
          const dataView = createView$1(data);
          for (; blockLen <= len - pos; pos += blockLen) this.process(dataView, pos);
          continue;
        }
        buffer.set(data.subarray(pos, pos + take), this.pos);
        this.pos += take;
        pos += take;
        if (this.pos === blockLen) {
          this.process(view, 0);
          this.pos = 0;
        }
      }
      this.length += data.length;
      this.roundClean();
      return this;
    }
    digestInto(out) {
      exists$1(this);
      output$1(out, this);
      this.finished = true;
      const {
        buffer: buffer,
        view: view,
        blockLen: blockLen,
        isLE: isLE
      } = this;
      let {
        pos: pos
      } = this;
      buffer[pos++] = 128;
      this.buffer.subarray(pos).fill(0);
      if (this.padOffset > blockLen - pos) {
        this.process(view, 0);
        pos = 0;
      }
      for (let i = pos; i < blockLen; i++) buffer[i] = 0;
      setBigUint64$1(view, blockLen - 8, BigInt(this.length * 8), isLE);
      this.process(view, 0);
      const oview = createView$1(out);
      const len = this.outputLen;
      if (len % 4) throw new Error("_sha2: outputLen should be aligned to 32bit");
      const outLen = len / 4;
      const state = this.get();
      if (outLen > state.length) throw new Error("_sha2: outputLen bigger than state");
      for (let i = 0; i < outLen; i++) oview.setUint32(4 * i, state[i], isLE);
    }
    digest() {
      const {
        buffer: buffer,
        outputLen: outputLen
      } = this;
      this.digestInto(buffer);
      const res = buffer.slice(0, outputLen);
      this.destroy();
      return res;
    }
    _cloneInto(to) {
      to || (to = new this.constructor());
      to.set(...this.get());
      const {
        blockLen: blockLen,
        buffer: buffer,
        length: length,
        finished: finished,
        destroyed: destroyed,
        pos: pos
      } = this;
      to.length = length;
      to.pos = pos;
      to.finished = finished;
      to.destroyed = destroyed;
      if (length % blockLen) to.buffer.set(buffer);
      return to;
    }
  }
  const U32_MASK64$1 = BigInt(2 ** 32 - 1);
  const _32n$1 = BigInt(32);
  function fromBig$1(n, le = false) {
    if (le) return $(function () {
      let result = $Object.create(null, undefined);
      result.h = Number(n & U32_MASK64$1);
      result.l = Number(n >> _32n$1 & U32_MASK64$1);
      return result;
    })();
    return $(function () {
      let result = $Object.create(null, undefined);
      result.h = Number(n >> _32n$1 & U32_MASK64$1) | 0;
      result.l = Number(n & U32_MASK64$1) | 0;
      return result;
    })();
  }
  $(fromBig$1);
  function split$1(lst, le = false) {
    let Ah = new Uint32Array(lst.length);
    let Al = new Uint32Array(lst.length);
    for (let i = 0; i < lst.length; i++) {
      const {
        h: h,
        l: l
      } = fromBig$1(lst[i], le);
      [Ah[i], Al[i]] = $Array.of(h, l);
    }
    return $Array.of(Ah, Al);
  }
  $(split$1);
  const toBig = (h, l) => BigInt(h >>> 0) << _32n$1 | BigInt(l >>> 0);
  $(toBig);
  const shrSH = (h, _l, s) => h >>> s;
  $(shrSH);
  const shrSL = (h, l, s) => h << 32 - s | l >>> s;
  $(shrSL);
  const rotrSH = (h, l, s) => h >>> s | l << 32 - s;
  $(rotrSH);
  const rotrSL = (h, l, s) => h << 32 - s | l >>> s;
  $(rotrSL);
  const rotrBH = (h, l, s) => h << 64 - s | l >>> s - 32;
  $(rotrBH);
  const rotrBL = (h, l, s) => h >>> s - 32 | l << 64 - s;
  $(rotrBL);
  const rotr32H = (_h, l) => l;
  $(rotr32H);
  const rotr32L = (h, _l) => h;
  $(rotr32L);
  const rotlSH$1 = (h, l, s) => h << s | l >>> 32 - s;
  $(rotlSH$1);
  const rotlSL$1 = (h, l, s) => l << s | h >>> 32 - s;
  $(rotlSL$1);
  const rotlBH$1 = (h, l, s) => l << s - 32 | h >>> 64 - s;
  $(rotlBH$1);
  const rotlBL$1 = (h, l, s) => h << s - 32 | l >>> 64 - s;
  $(rotlBL$1);
  function add(Ah, Al, Bh, Bl) {
    const l = (Al >>> 0) + (Bl >>> 0);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.h = Ah + Bh + (l / 2 ** 32 | 0) | 0;
      result.l = l | 0;
      return result;
    })();
  }
  $(add);
  const add3L = (Al, Bl, Cl) => (Al >>> 0) + (Bl >>> 0) + (Cl >>> 0);
  $(add3L);
  const add3H = (low, Ah, Bh, Ch) => Ah + Bh + Ch + (low / 2 ** 32 | 0) | 0;
  $(add3H);
  const add4L = (Al, Bl, Cl, Dl) => (Al >>> 0) + (Bl >>> 0) + (Cl >>> 0) + (Dl >>> 0);
  $(add4L);
  const add4H = (low, Ah, Bh, Ch, Dh) => Ah + Bh + Ch + Dh + (low / 2 ** 32 | 0) | 0;
  $(add4H);
  const add5L = (Al, Bl, Cl, Dl, El) => (Al >>> 0) + (Bl >>> 0) + (Cl >>> 0) + (Dl >>> 0) + (El >>> 0);
  $(add5L);
  const add5H = (low, Ah, Bh, Ch, Dh, Eh) => Ah + Bh + Ch + Dh + Eh + (low / 2 ** 32 | 0) | 0;
  $(add5H);
  const u64$1 = $(function () {
    let result = $Object.create(null, undefined);
    result.fromBig = fromBig$1;
    result.split = split$1;
    result.toBig = toBig;
    result.shrSH = shrSH;
    result.shrSL = shrSL;
    result.rotrSH = rotrSH;
    result.rotrSL = rotrSL;
    result.rotrBH = rotrBH;
    result.rotrBL = rotrBL;
    result.rotr32H = rotr32H;
    result.rotr32L = rotr32L;
    result.rotlSH = rotlSH$1;
    result.rotlSL = rotlSL$1;
    result.rotlBH = rotlBH$1;
    result.rotlBL = rotlBL$1;
    result.add = add;
    result.add3L = add3L;
    result.add3H = add3H;
    result.add4L = add4L;
    result.add4H = add4H;
    result.add5H = add5H;
    result.add5L = add5L;
    return result;
  })();
  const [SHA512_Kh, SHA512_Kl] = $(() => u64$1.split($Array.of("0x428a2f98d728ae22", "0x7137449123ef65cd", "0xb5c0fbcfec4d3b2f", "0xe9b5dba58189dbbc", "0x3956c25bf348b538", "0x59f111f1b605d019", "0x923f82a4af194f9b", "0xab1c5ed5da6d8118", "0xd807aa98a3030242", "0x12835b0145706fbe", "0x243185be4ee4b28c", "0x550c7dc3d5ffb4e2", "0x72be5d74f27b896f", "0x80deb1fe3b1696b1", "0x9bdc06a725c71235", "0xc19bf174cf692694", "0xe49b69c19ef14ad2", "0xefbe4786384f25e3", "0x0fc19dc68b8cd5b5", "0x240ca1cc77ac9c65", "0x2de92c6f592b0275", "0x4a7484aa6ea6e483", "0x5cb0a9dcbd41fbd4", "0x76f988da831153b5", "0x983e5152ee66dfab", "0xa831c66d2db43210", "0xb00327c898fb213f", "0xbf597fc7beef0ee4", "0xc6e00bf33da88fc2", "0xd5a79147930aa725", "0x06ca6351e003826f", "0x142929670a0e6e70", "0x27b70a8546d22ffc", "0x2e1b21385c26c926", "0x4d2c6dfc5ac42aed", "0x53380d139d95b3df", "0x650a73548baf63de", "0x766a0abb3c77b2a8", "0x81c2c92e47edaee6", "0x92722c851482353b", "0xa2bfe8a14cf10364", "0xa81a664bbc423001", "0xc24b8b70d0f89791", "0xc76c51a30654be30", "0xd192e819d6ef5218", "0xd69906245565a910", "0xf40e35855771202a", "0x106aa07032bbd1b8", "0x19a4c116b8d2d0c8", "0x1e376c085141ab53", "0x2748774cdf8eeb99", "0x34b0bcb5e19b48a8", "0x391c0cb3c5c95a63", "0x4ed8aa4ae3418acb", "0x5b9cca4f7763e373", "0x682e6ff3d6b2b8a3", "0x748f82ee5defb2fc", "0x78a5636f43172f60", "0x84c87814a1f0ab72", "0x8cc702081a6439ec", "0x90befffa23631e28", "0xa4506cebde82bde9", "0xbef9a3f7b2c67915", "0xc67178f2e372532b", "0xca273eceea26619c", "0xd186b8c721c0c207", "0xeada7dd6cde0eb1e", "0xf57d4f7fee6ed178", "0x06f067aa72176fba", "0x0a637dc5a2c898a6", "0x113f9804bef90dae", "0x1b710b35131c471b", "0x28db77f523047d84", "0x32caab7b40c72493", "0x3c9ebe0a15c9bebc", "0x431d67c49c100d4c", "0x4cc5d4becb3e42b6", "0x597f299cfc657e2a", "0x5fcb6fab3ad6faec", "0x6c44198c4a475817").map($(n => BigInt(n)))))();
  const SHA512_W_H = new Uint32Array(80);
  const SHA512_W_L = new Uint32Array(80);
  class SHA512 extends HashMD {
    constructor() {
      super(128, 64, 16, false);
      this.Ah = 1779033703 | 0;
      this.Al = 4089235720 | 0;
      this.Bh = 3144134277 | 0;
      this.Bl = 2227873595 | 0;
      this.Ch = 1013904242 | 0;
      this.Cl = 4271175723 | 0;
      this.Dh = 2773480762 | 0;
      this.Dl = 1595750129 | 0;
      this.Eh = 1359893119 | 0;
      this.El = 2917565137 | 0;
      this.Fh = 2600822924 | 0;
      this.Fl = 725511199 | 0;
      this.Gh = 528734635 | 0;
      this.Gl = 4215389547 | 0;
      this.Hh = 1541459225 | 0;
      this.Hl = 327033209 | 0;
    }
    get() {
      const {
        Ah: Ah,
        Al: Al,
        Bh: Bh,
        Bl: Bl,
        Ch: Ch,
        Cl: Cl,
        Dh: Dh,
        Dl: Dl,
        Eh: Eh,
        El: El,
        Fh: Fh,
        Fl: Fl,
        Gh: Gh,
        Gl: Gl,
        Hh: Hh,
        Hl: Hl
      } = this;
      return $Array.of(Ah, Al, Bh, Bl, Ch, Cl, Dh, Dl, Eh, El, Fh, Fl, Gh, Gl, Hh, Hl);
    }
    set(Ah, Al, Bh, Bl, Ch, Cl, Dh, Dl, Eh, El, Fh, Fl, Gh, Gl, Hh, Hl) {
      this.Ah = Ah | 0;
      this.Al = Al | 0;
      this.Bh = Bh | 0;
      this.Bl = Bl | 0;
      this.Ch = Ch | 0;
      this.Cl = Cl | 0;
      this.Dh = Dh | 0;
      this.Dl = Dl | 0;
      this.Eh = Eh | 0;
      this.El = El | 0;
      this.Fh = Fh | 0;
      this.Fl = Fl | 0;
      this.Gh = Gh | 0;
      this.Gl = Gl | 0;
      this.Hh = Hh | 0;
      this.Hl = Hl | 0;
    }
    process(view, offset) {
      for (let i = 0; i < 16; i++, offset += 4) {
        SHA512_W_H[i] = view.getUint32(offset);
        SHA512_W_L[i] = view.getUint32(offset += 4);
      }
      for (let i = 16; i < 80; i++) {
        const W15h = SHA512_W_H[i - 15] | 0;
        const W15l = SHA512_W_L[i - 15] | 0;
        const s0h = u64$1.rotrSH(W15h, W15l, 1) ^ u64$1.rotrSH(W15h, W15l, 8) ^ u64$1.shrSH(W15h, W15l, 7);
        const s0l = u64$1.rotrSL(W15h, W15l, 1) ^ u64$1.rotrSL(W15h, W15l, 8) ^ u64$1.shrSL(W15h, W15l, 7);
        const W2h = SHA512_W_H[i - 2] | 0;
        const W2l = SHA512_W_L[i - 2] | 0;
        const s1h = u64$1.rotrSH(W2h, W2l, 19) ^ u64$1.rotrBH(W2h, W2l, 61) ^ u64$1.shrSH(W2h, W2l, 6);
        const s1l = u64$1.rotrSL(W2h, W2l, 19) ^ u64$1.rotrBL(W2h, W2l, 61) ^ u64$1.shrSL(W2h, W2l, 6);
        const SUMl = u64$1.add4L(s0l, s1l, SHA512_W_L[i - 7], SHA512_W_L[i - 16]);
        const SUMh = u64$1.add4H(SUMl, s0h, s1h, SHA512_W_H[i - 7], SHA512_W_H[i - 16]);
        SHA512_W_H[i] = SUMh | 0;
        SHA512_W_L[i] = SUMl | 0;
      }
      let {
        Ah: Ah,
        Al: Al,
        Bh: Bh,
        Bl: Bl,
        Ch: Ch,
        Cl: Cl,
        Dh: Dh,
        Dl: Dl,
        Eh: Eh,
        El: El,
        Fh: Fh,
        Fl: Fl,
        Gh: Gh,
        Gl: Gl,
        Hh: Hh,
        Hl: Hl
      } = this;
      for (let i = 0; i < 80; i++) {
        const sigma1h = u64$1.rotrSH(Eh, El, 14) ^ u64$1.rotrSH(Eh, El, 18) ^ u64$1.rotrBH(Eh, El, 41);
        const sigma1l = u64$1.rotrSL(Eh, El, 14) ^ u64$1.rotrSL(Eh, El, 18) ^ u64$1.rotrBL(Eh, El, 41);
        const CHIh = Eh & Fh ^ ~Eh & Gh;
        const CHIl = El & Fl ^ ~El & Gl;
        const T1ll = u64$1.add5L(Hl, sigma1l, CHIl, SHA512_Kl[i], SHA512_W_L[i]);
        const T1h = u64$1.add5H(T1ll, Hh, sigma1h, CHIh, SHA512_Kh[i], SHA512_W_H[i]);
        const T1l = T1ll | 0;
        const sigma0h = u64$1.rotrSH(Ah, Al, 28) ^ u64$1.rotrBH(Ah, Al, 34) ^ u64$1.rotrBH(Ah, Al, 39);
        const sigma0l = u64$1.rotrSL(Ah, Al, 28) ^ u64$1.rotrBL(Ah, Al, 34) ^ u64$1.rotrBL(Ah, Al, 39);
        const MAJh = Ah & Bh ^ Ah & Ch ^ Bh & Ch;
        const MAJl = Al & Bl ^ Al & Cl ^ Bl & Cl;
        Hh = Gh | 0;
        Hl = Gl | 0;
        Gh = Fh | 0;
        Gl = Fl | 0;
        Fh = Eh | 0;
        Fl = El | 0;
        ({
          h: Eh,
          l: El
        } = u64$1.add(Dh | 0, Dl | 0, T1h | 0, T1l | 0));
        Dh = Ch | 0;
        Dl = Cl | 0;
        Ch = Bh | 0;
        Cl = Bl | 0;
        Bh = Ah | 0;
        Bl = Al | 0;
        const All = u64$1.add3L(T1l, sigma0l, MAJl);
        Ah = u64$1.add3H(All, T1h, sigma0h, MAJh);
        Al = All | 0;
      }
      ({
        h: Ah,
        l: Al
      } = u64$1.add(this.Ah | 0, this.Al | 0, Ah | 0, Al | 0));
      ({
        h: Bh,
        l: Bl
      } = u64$1.add(this.Bh | 0, this.Bl | 0, Bh | 0, Bl | 0));
      ({
        h: Ch,
        l: Cl
      } = u64$1.add(this.Ch | 0, this.Cl | 0, Ch | 0, Cl | 0));
      ({
        h: Dh,
        l: Dl
      } = u64$1.add(this.Dh | 0, this.Dl | 0, Dh | 0, Dl | 0));
      ({
        h: Eh,
        l: El
      } = u64$1.add(this.Eh | 0, this.El | 0, Eh | 0, El | 0));
      ({
        h: Fh,
        l: Fl
      } = u64$1.add(this.Fh | 0, this.Fl | 0, Fh | 0, Fl | 0));
      ({
        h: Gh,
        l: Gl
      } = u64$1.add(this.Gh | 0, this.Gl | 0, Gh | 0, Gl | 0));
      ({
        h: Hh,
        l: Hl
      } = u64$1.add(this.Hh | 0, this.Hl | 0, Hh | 0, Hl | 0));
      this.set(Ah, Al, Bh, Bl, Ch, Cl, Dh, Dl, Eh, El, Fh, Fl, Gh, Gl, Hh, Hl);
    }
    roundClean() {
      SHA512_W_H.fill(0);
      SHA512_W_L.fill(0);
    }
    destroy() {
      this.buffer.fill(0);
      this.set(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  }
  const sha512 = wrapConstructor$1($(() => new SHA512()));
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  const _0n$5 = BigInt(0);
  const _1n$7 = BigInt(1);
  const _2n$5 = BigInt(2);
  function isBytes$2(a) {
    return a instanceof Uint8Array || a != null && typeof a === "object" && a.constructor.name === "Uint8Array";
  }
  $(isBytes$2);
  function abytes(item) {
    if (!isBytes$2(item)) throw new Error("Uint8Array expected");
  }
  $(abytes);
  const hexes = $Array.from($(function () {
    let result = $Object.create(null, undefined);
    result.length = 256;
    return result;
  })(), $((_, i) => i.toString(16).padStart(2, "0")));
  function bytesToHex(bytes) {
    abytes(bytes);
    let hex = "";
    for (let i = 0; i < bytes.length; i++) {
      hex += hexes[bytes[i]];
    }
    return hex;
  }
  $(bytesToHex);
  function numberToHexUnpadded(num) {
    const hex = num.toString(16);
    return hex.length & 1 ? `0${hex}` : hex;
  }
  $(numberToHexUnpadded);
  function hexToNumber(hex) {
    if (typeof hex !== "string") throw new Error("hex string expected, got " + typeof hex);
    return BigInt(hex === "" ? "0" : `0x${hex}`);
  }
  $(hexToNumber);
  const asciis = $(function () {
    let result = $Object.create(null, undefined);
    result._0 = 48;
    result._9 = 57;
    result._A = 65;
    result._F = 70;
    result._a = 97;
    result._f = 102;
    return result;
  })();
  function asciiToBase16(char) {
    if (char >= asciis._0 && char <= asciis._9) return char - asciis._0;
    if (char >= asciis._A && char <= asciis._F) return char - (asciis._A - 10);
    if (char >= asciis._a && char <= asciis._f) return char - (asciis._a - 10);
    return;
  }
  $(asciiToBase16);
  function hexToBytes(hex) {
    if (typeof hex !== "string") throw new Error("hex string expected, got " + typeof hex);
    const hl = hex.length;
    const al = hl / 2;
    if (hl % 2) throw new Error("padded hex string expected, got unpadded hex of length " + hl);
    const array = new Uint8Array(al);
    for (let ai = 0, hi = 0; ai < al; ai++, hi += 2) {
      const n1 = asciiToBase16(hex.charCodeAt(hi));
      const n2 = asciiToBase16(hex.charCodeAt(hi + 1));
      if (n1 === undefined || n2 === undefined) {
        const char = hex[hi] + hex[hi + 1];
        throw new Error('hex string expected, got non-hex character "' + char + '" at index ' + hi);
      }
      array[ai] = n1 * 16 + n2;
    }
    return array;
  }
  $(hexToBytes);
  function bytesToNumberBE(bytes) {
    return hexToNumber(bytesToHex(bytes));
  }
  $(bytesToNumberBE);
  function bytesToNumberLE(bytes) {
    abytes(bytes);
    return hexToNumber(bytesToHex(Uint8Array.from(bytes).reverse()));
  }
  $(bytesToNumberLE);
  function numberToBytesBE(n, len) {
    return hexToBytes(n.toString(16).padStart(len * 2, "0"));
  }
  $(numberToBytesBE);
  function numberToBytesLE(n, len) {
    return numberToBytesBE(n, len).reverse();
  }
  $(numberToBytesLE);
  function numberToVarBytesBE(n) {
    return hexToBytes(numberToHexUnpadded(n));
  }
  $(numberToVarBytesBE);
  function ensureBytes(title, hex, expectedLength) {
    let res;
    if (typeof hex === "string") {
      try {
        res = hexToBytes(hex);
      } catch (e) {
        throw new Error(`${title} must be valid hex string, got "${hex}". Cause: ${e}`);
      }
    } else if (isBytes$2(hex)) {
      res = Uint8Array.from(hex);
    } else {
      throw new Error(`${title} must be hex string or Uint8Array`);
    }
    const len = res.length;
    if (typeof expectedLength === "number" && len !== expectedLength) throw new Error(`${title} expected ${expectedLength} bytes, got ${len}`);
    return res;
  }
  $(ensureBytes);
  function concatBytes(...arrays) {
    let sum = 0;
    for (let i = 0; i < arrays.length; i++) {
      const a = arrays[i];
      abytes(a);
      sum += a.length;
    }
    const res = new Uint8Array(sum);
    for (let i = 0, pad = 0; i < arrays.length; i++) {
      const a = arrays[i];
      res.set(a, pad);
      pad += a.length;
    }
    return res;
  }
  $(concatBytes);
  function equalBytes(a, b) {
    if (a.length !== b.length) return false;
    let diff = 0;
    for (let i = 0; i < a.length; i++) diff |= a[i] ^ b[i];
    return diff === 0;
  }
  $(equalBytes);
  function utf8ToBytes$1(str) {
    if (typeof str !== "string") throw new Error(`utf8ToBytes expected string, got ${typeof str}`);
    return new Uint8Array(new TextEncoder().encode(str));
  }
  $(utf8ToBytes$1);
  function bitLen(n) {
    let len;
    for (len = 0; n > _0n$5; n >>= _1n$7, len += 1);
    return len;
  }
  $(bitLen);
  function bitGet(n, pos) {
    return n >> BigInt(pos) & _1n$7;
  }
  $(bitGet);
  function bitSet(n, pos, value) {
    return n | (value ? _1n$7 : _0n$5) << BigInt(pos);
  }
  $(bitSet);
  const bitMask = n => (_2n$5 << BigInt(n - 1)) - _1n$7;
  $(bitMask);
  const u8n = data => new Uint8Array(data);
  $(u8n);
  const u8fr = arr => Uint8Array.from(arr);
  $(u8fr);
  function createHmacDrbg(hashLen, qByteLen, hmacFn) {
    if (typeof hashLen !== "number" || hashLen < 2) throw new Error("hashLen must be a number");
    if (typeof qByteLen !== "number" || qByteLen < 2) throw new Error("qByteLen must be a number");
    if (typeof hmacFn !== "function") throw new Error("hmacFn must be a function");
    let v = u8n(hashLen);
    let k = u8n(hashLen);
    let i = 0;
    const reset = () => {
      v.fill(1);
      k.fill(0);
      i = 0;
    };
    $(reset);
    const h = (...b) => hmacFn(k, v, ...b);
    $(h);
    const reseed = (seed = u8n()) => {
      k = h(u8fr($Array.of(0)), seed);
      v = h();
      if (seed.length === 0) return;
      k = h(u8fr($Array.of(1)), seed);
      v = h();
    };
    $(reseed);
    const gen = () => {
      if (i++ >= 1e3) throw new Error("drbg: tried 1000 values");
      let len = 0;
      const out = $Array.of();
      while (len < qByteLen) {
        v = h();
        const sl = v.slice();
        out.push(sl);
        len += v.length;
      }
      return concatBytes(...out);
    };
    $(gen);
    const genUntil = (seed, pred) => {
      reset();
      reseed(seed);
      let res = undefined;
      while (!(res = pred(gen()))) reseed();
      reset();
      return res;
    };
    $(genUntil);
    return genUntil;
  }
  $(createHmacDrbg);
  const validatorFns = $(function () {
    let result = $Object.create(null, undefined);
    result.bigint = $(val => typeof val === "bigint");
    result.function = $(val => typeof val === "function");
    result.boolean = $(val => typeof val === "boolean");
    result.string = $(val => typeof val === "string");
    result.stringOrUint8Array = $(val => typeof val === "string" || isBytes$2(val));
    result.isSafeInteger = $(val => Number.isSafeInteger(val));
    result.array = $(val => $Array.isArray(val));
    result.field = $((val, object) => object.Fp.isValid(val));
    result.hash = $(val => typeof val === "function" && Number.isSafeInteger(val.outputLen));
    return result;
  })();
  function validateObject(object, validators, optValidators = $Object.create(null, undefined)) {
    const checkField = (fieldName, type, isOptional) => {
      const checkVal = validatorFns[type];
      if (typeof checkVal !== "function") throw new Error(`Invalid validator "${type}", expected function`);
      const val = object[fieldName];
      if (isOptional && val === undefined) return;
      if (!checkVal(val, object)) {
        throw new Error(`Invalid param ${String(fieldName)}=${val} (${typeof val}), expected ${type}`);
      }
    };
    $(checkField);
    for (const [fieldName, type] of $Object.entries(validators)) checkField(fieldName, type, false);
    for (const [fieldName, type] of $Object.entries(optValidators)) checkField(fieldName, type, true);
    return object;
  }
  $(validateObject);
  var ut = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.__proto__ = null;
    result.abytes = abytes;
    result.bitGet = bitGet;
    result.bitLen = bitLen;
    result.bitMask = bitMask;
    result.bitSet = bitSet;
    result.bytesToHex = bytesToHex;
    result.bytesToNumberBE = bytesToNumberBE;
    result.bytesToNumberLE = bytesToNumberLE;
    result.concatBytes = concatBytes;
    result.createHmacDrbg = createHmacDrbg;
    result.ensureBytes = ensureBytes;
    result.equalBytes = equalBytes;
    result.hexToBytes = hexToBytes;
    result.hexToNumber = hexToNumber;
    result.isBytes = isBytes$2;
    result.numberToBytesBE = numberToBytesBE;
    result.numberToBytesLE = numberToBytesLE;
    result.numberToHexUnpadded = numberToHexUnpadded;
    result.numberToVarBytesBE = numberToVarBytesBE;
    result.utf8ToBytes = utf8ToBytes$1;
    result.validateObject = validateObject;
    return result;
  })());
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  const _0n$4 = BigInt(0),
    _1n$6 = BigInt(1),
    _2n$4 = BigInt(2),
    _3n$1 = BigInt(3);
  const _4n = BigInt(4),
    _5n$1 = BigInt(5),
    _8n$1 = BigInt(8);
  BigInt(9);
  BigInt(16);
  function mod(a, b) {
    const result = a % b;
    return result >= _0n$4 ? result : b + result;
  }
  $(mod);
  function pow(num, power, modulo) {
    if (modulo <= _0n$4 || power < _0n$4) throw new Error("Expected power/modulo > 0");
    if (modulo === _1n$6) return _0n$4;
    let res = _1n$6;
    while (power > _0n$4) {
      if (power & _1n$6) res = res * num % modulo;
      num = num * num % modulo;
      power >>= _1n$6;
    }
    return res;
  }
  $(pow);
  function pow2(x, power, modulo) {
    let res = x;
    while (power-- > _0n$4) {
      res *= res;
      res %= modulo;
    }
    return res;
  }
  $(pow2);
  function invert(number, modulo) {
    if (number === _0n$4 || modulo <= _0n$4) {
      throw new Error(`invert: expected positive integers, got n=${number} mod=${modulo}`);
    }
    let a = mod(number, modulo);
    let b = modulo;
    let x = _0n$4,
      u = _1n$6;
    while (a !== _0n$4) {
      const q = b / a;
      const r = b % a;
      const m = x - u * q;
      b = a, a = r, x = u, u = m;
    }
    const gcd = b;
    if (gcd !== _1n$6) throw new Error("invert: does not exist");
    return mod(x, modulo);
  }
  $(invert);
  function tonelliShanks(P) {
    const legendreC = (P - _1n$6) / _2n$4;
    let Q, S, Z;
    for (Q = P - _1n$6, S = 0; Q % _2n$4 === _0n$4; Q /= _2n$4, S++);
    for (Z = _2n$4; Z < P && pow(Z, legendreC, P) !== P - _1n$6; Z++);
    if (S === 1) {
      const p1div4 = (P + _1n$6) / _4n;
      return $(function tonelliFast(Fp, n) {
        const root = Fp.pow(n, p1div4);
        if (!Fp.eql(Fp.sqr(root), n)) throw new Error("Cannot find square root");
        return root;
      });
    }
    const Q1div2 = (Q + _1n$6) / _2n$4;
    return $(function tonelliSlow(Fp, n) {
      if (Fp.pow(n, legendreC) === Fp.neg(Fp.ONE)) throw new Error("Cannot find square root");
      let r = S;
      let g = Fp.pow(Fp.mul(Fp.ONE, Z), Q);
      let x = Fp.pow(n, Q1div2);
      let b = Fp.pow(n, Q);
      while (!Fp.eql(b, Fp.ONE)) {
        if (Fp.eql(b, Fp.ZERO)) return Fp.ZERO;
        let m = 1;
        for (let t2 = Fp.sqr(b); m < r; m++) {
          if (Fp.eql(t2, Fp.ONE)) break;
          t2 = Fp.sqr(t2);
        }
        const ge = Fp.pow(g, _1n$6 << BigInt(r - m - 1));
        g = Fp.sqr(ge);
        x = Fp.mul(x, ge);
        b = Fp.mul(b, g);
        r = m;
      }
      return x;
    });
  }
  $(tonelliShanks);
  function FpSqrt(P) {
    if (P % _4n === _3n$1) {
      const p1div4 = (P + _1n$6) / _4n;
      return $(function sqrt3mod4(Fp, n) {
        const root = Fp.pow(n, p1div4);
        if (!Fp.eql(Fp.sqr(root), n)) throw new Error("Cannot find square root");
        return root;
      });
    }
    if (P % _8n$1 === _5n$1) {
      const c1 = (P - _5n$1) / _8n$1;
      return $(function sqrt5mod8(Fp, n) {
        const n2 = Fp.mul(n, _2n$4);
        const v = Fp.pow(n2, c1);
        const nv = Fp.mul(n, v);
        const i = Fp.mul(Fp.mul(nv, _2n$4), v);
        const root = Fp.mul(nv, Fp.sub(i, Fp.ONE));
        if (!Fp.eql(Fp.sqr(root), n)) throw new Error("Cannot find square root");
        return root;
      });
    }
    return tonelliShanks(P);
  }
  $(FpSqrt);
  const isNegativeLE = (num, modulo) => (mod(num, modulo) & _1n$6) === _1n$6;
  $(isNegativeLE);
  const FIELD_FIELDS = $Array.of("create", "isValid", "is0", "neg", "inv", "sqrt", "sqr", "eql", "add", "sub", "mul", "pow", "div", "addN", "subN", "mulN", "sqrN");
  function validateField(field) {
    const initial = $(function () {
      let result = $Object.create(null, undefined);
      result.ORDER = "bigint";
      result.MASK = "bigint";
      result.BYTES = "isSafeInteger";
      result.BITS = "isSafeInteger";
      return result;
    })();
    const opts = FIELD_FIELDS.reduce($((map, val) => {
      map[val] = "function";
      return map;
    }), initial);
    return validateObject(field, opts);
  }
  $(validateField);
  function FpPow(f, num, power) {
    if (power < _0n$4) throw new Error("Expected power > 0");
    if (power === _0n$4) return f.ONE;
    if (power === _1n$6) return num;
    let p = f.ONE;
    let d = num;
    while (power > _0n$4) {
      if (power & _1n$6) p = f.mul(p, d);
      d = f.sqr(d);
      power >>= _1n$6;
    }
    return p;
  }
  $(FpPow);
  function FpInvertBatch(f, nums) {
    const tmp = new Array(nums.length);
    const lastMultiplied = nums.reduce($((acc, num, i) => {
      if (f.is0(num)) return acc;
      tmp[i] = acc;
      return f.mul(acc, num);
    }), f.ONE);
    const inverted = f.inv(lastMultiplied);
    nums.reduceRight($((acc, num, i) => {
      if (f.is0(num)) return acc;
      tmp[i] = f.mul(acc, tmp[i]);
      return f.mul(acc, num);
    }), inverted);
    return tmp;
  }
  $(FpInvertBatch);
  function nLength(n, nBitLength) {
    const _nBitLength = nBitLength !== undefined ? nBitLength : n.toString(2).length;
    const nByteLength = Math.ceil(_nBitLength / 8);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.nBitLength = _nBitLength;
      result.nByteLength = nByteLength;
      return result;
    })();
  }
  $(nLength);
  function Field(ORDER, bitLen, isLE = false, redef = $Object.create(null, undefined)) {
    if (ORDER <= _0n$4) throw new Error(`Expected Field ORDER > 0, got ${ORDER}`);
    const {
      nBitLength: BITS,
      nByteLength: BYTES
    } = nLength(ORDER, bitLen);
    if (BYTES > 2048) throw new Error("Field lengths over 2048 bytes are not supported");
    const sqrtP = FpSqrt(ORDER);
    const f = $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      result.ORDER = ORDER;
      result.BITS = BITS;
      result.BYTES = BYTES;
      result.MASK = bitMask(BITS);
      result.ZERO = _0n$4;
      result.ONE = _1n$6;
      result.create = $(num => mod(num, ORDER));
      result.isValid = $(num => {
        if (typeof num !== "bigint") throw new Error(`Invalid field element: expected bigint, got ${typeof num}`);
        return _0n$4 <= num && num < ORDER;
      });
      result.is0 = $(num => num === _0n$4);
      result.isOdd = $(num => (num & _1n$6) === _1n$6);
      result.neg = $(num => mod(-num, ORDER));
      result.eql = $((lhs, rhs) => lhs === rhs);
      result.sqr = $(num => mod(num * num, ORDER));
      result.add = $((lhs, rhs) => mod(lhs + rhs, ORDER));
      result.sub = $((lhs, rhs) => mod(lhs - rhs, ORDER));
      result.mul = $((lhs, rhs) => mod(lhs * rhs, ORDER));
      result.pow = $((num, power) => FpPow(f, num, power));
      result.div = $((lhs, rhs) => mod(lhs * invert(rhs, ORDER), ORDER));
      result.sqrN = $(num => num * num);
      result.addN = $((lhs, rhs) => lhs + rhs);
      result.subN = $((lhs, rhs) => lhs - rhs);
      result.mulN = $((lhs, rhs) => lhs * rhs);
      result.inv = $(num => invert(num, ORDER));
      result.sqrt = redef.sqrt || $(n => sqrtP(f, n));
      result.invertBatch = $(lst => FpInvertBatch(f, lst));
      result.cmov = $((a, b, c) => c ? b : a);
      result.toBytes = $(num => isLE ? numberToBytesLE(num, BYTES) : numberToBytesBE(num, BYTES));
      result.fromBytes = $(bytes => {
        if (bytes.length !== BYTES) throw new Error(`Fp.fromBytes: expected ${BYTES}, got ${bytes.length}`);
        return isLE ? bytesToNumberLE(bytes) : bytesToNumberBE(bytes);
      });
      return result;
    })());
    return $Object.freeze(f);
  }
  $(Field);
  function FpSqrtEven(Fp, elm) {
    if (!Fp.isOdd) throw new Error(`Field doesn't have isOdd`);
    const root = Fp.sqrt(elm);
    return Fp.isOdd(root) ? Fp.neg(root) : root;
  }
  $(FpSqrtEven);
  function getFieldBytesLength(fieldOrder) {
    if (typeof fieldOrder !== "bigint") throw new Error("field order must be bigint");
    const bitLength = fieldOrder.toString(2).length;
    return Math.ceil(bitLength / 8);
  }
  $(getFieldBytesLength);
  function getMinHashLength(fieldOrder) {
    const length = getFieldBytesLength(fieldOrder);
    return length + Math.ceil(length / 2);
  }
  $(getMinHashLength);
  function mapHashToField(key, fieldOrder, isLE = false) {
    const len = key.length;
    const fieldLen = getFieldBytesLength(fieldOrder);
    const minLen = getMinHashLength(fieldOrder);
    if (len < 16 || len < minLen || len > 1024) throw new Error(`expected ${minLen}-1024 bytes of input, got ${len}`);
    const num = isLE ? bytesToNumberBE(key) : bytesToNumberLE(key);
    const reduced = mod(num, fieldOrder - _1n$6) + _1n$6;
    return isLE ? numberToBytesLE(reduced, fieldLen) : numberToBytesBE(reduced, fieldLen);
  }
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  $(mapHashToField);
  const _0n$3 = BigInt(0);
  const _1n$5 = BigInt(1);
  function wNAF(c, bits) {
    const constTimeNegate = (condition, item) => {
      const neg = item.negate();
      return condition ? neg : item;
    };
    $(constTimeNegate);
    const opts = W => {
      const windows = Math.ceil(bits / W) + 1;
      const windowSize = 2 ** (W - 1);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.windows = windows;
        result.windowSize = windowSize;
        return result;
      })();
    };
    $(opts);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.constTimeNegate = constTimeNegate;
      result.unsafeLadder = $(function (elm, n) {
        let p = c.ZERO;
        let d = elm;
        while (n > _0n$3) {
          if (n & _1n$5) p = p.add(d);
          d = d.double();
          n >>= _1n$5;
        }
        return p;
      });
      result.precomputeWindow = $(function (elm, W) {
        const {
          windows: windows,
          windowSize: windowSize
        } = opts(W);
        const points = $Array.of();
        let p = elm;
        let base = p;
        for (let window = 0; window < windows; window++) {
          base = p;
          points.push(base);
          for (let i = 1; i < windowSize; i++) {
            base = base.add(p);
            points.push(base);
          }
          p = base.double();
        }
        return points;
      });
      result.wNAF = $(function (W, precomputes, n) {
        const {
          windows: windows,
          windowSize: windowSize
        } = opts(W);
        let p = c.ZERO;
        let f = c.BASE;
        const mask = BigInt(2 ** W - 1);
        const maxNumber = 2 ** W;
        const shiftBy = BigInt(W);
        for (let window = 0; window < windows; window++) {
          const offset = window * windowSize;
          let wbits = Number(n & mask);
          n >>= shiftBy;
          if (wbits > windowSize) {
            wbits -= maxNumber;
            n += _1n$5;
          }
          const offset1 = offset;
          const offset2 = offset + Math.abs(wbits) - 1;
          const cond1 = window % 2 !== 0;
          const cond2 = wbits < 0;
          if (wbits === 0) {
            f = f.add(constTimeNegate(cond1, precomputes[offset1]));
          } else {
            p = p.add(constTimeNegate(cond2, precomputes[offset2]));
          }
        }
        return $(function () {
          let result = $Object.create(null, undefined);
          result.p = p;
          result.f = f;
          return result;
        })();
      });
      result.wNAFCached = $(function (P, precomputesMap, n, transform) {
        const W = P._WINDOW_SIZE || 1;
        let comp = precomputesMap.get(P);
        if (!comp) {
          comp = this.precomputeWindow(P, W);
          if (W !== 1) {
            precomputesMap.set(P, transform(comp));
          }
        }
        return this.wNAF(W, comp, n);
      });
      return result;
    })();
  }
  $(wNAF);
  function validateBasic(curve) {
    validateField(curve.Fp);
    validateObject(curve, $(function () {
      let result = $Object.create(null, undefined);
      result.n = "bigint";
      result.h = "bigint";
      result.Gx = "field";
      result.Gy = "field";
      return result;
    })(), $(function () {
      let result = $Object.create(null, undefined);
      result.nBitLength = "isSafeInteger";
      result.nByteLength = "isSafeInteger";
      return result;
    })());
    return $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, nLength(curve.n, curve.nBitLength));
      $Object.assign(result, curve);
      $Object.assign(result, $(function () {
        let result = $Object.create(null, undefined);
        result.p = curve.Fp.ORDER;
        return result;
      })());
      return result;
    })());
  }
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  $(validateBasic);
  const _0n$2 = BigInt(0),
    _1n$4 = BigInt(1),
    _2n$3 = BigInt(2),
    _8n = BigInt(8);
  const VERIFY_DEFAULT = $(function () {
    let result = $Object.create(null, undefined);
    result.zip215 = true;
    return result;
  })();
  function validateOpts$1(curve) {
    const opts = validateBasic(curve);
    validateObject(curve, $(function () {
      let result = $Object.create(null, undefined);
      result.hash = "function";
      result.a = "bigint";
      result.d = "bigint";
      result.randomBytes = "function";
      return result;
    })(), $(function () {
      let result = $Object.create(null, undefined);
      result.adjustScalarBytes = "function";
      result.domain = "function";
      result.uvRatio = "function";
      result.mapToCurve = "function";
      return result;
    })());
    return $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, opts);
      return result;
    })());
  }
  $(validateOpts$1);
  function twistedEdwards(curveDef) {
    const CURVE = validateOpts$1(curveDef);
    const {
      Fp: Fp,
      n: CURVE_ORDER,
      prehash: prehash,
      hash: cHash,
      randomBytes: randomBytes,
      nByteLength: nByteLength,
      h: cofactor
    } = CURVE;
    const MASK = _2n$3 << BigInt(nByteLength * 8) - _1n$4;
    const modP = Fp.create;
    const uvRatio = CURVE.uvRatio || $((u, v) => {
      try {
        return $(function () {
          let result = $Object.create(null, undefined);
          result.isValid = true;
          result.value = Fp.sqrt(u * Fp.inv(v));
          return result;
        })();
      } catch (e) {
        return $(function () {
          let result = $Object.create(null, undefined);
          result.isValid = false;
          result.value = _0n$2;
          return result;
        })();
      }
    });
    const adjustScalarBytes = CURVE.adjustScalarBytes || $(bytes => bytes);
    const domain = CURVE.domain || $((data, ctx, phflag) => {
      if (ctx.length || phflag) throw new Error("Contexts/pre-hash are not supported");
      return data;
    });
    const inBig = n => typeof n === "bigint" && _0n$2 < n;
    $(inBig);
    const inRange = (n, max) => inBig(n) && inBig(max) && n < max;
    $(inRange);
    const in0MaskRange = n => n === _0n$2 || inRange(n, MASK);
    $(in0MaskRange);
    function assertInRange(n, max) {
      if (inRange(n, max)) return n;
      throw new Error(`Expected valid scalar < ${max}, got ${typeof n} ${n}`);
    }
    $(assertInRange);
    function assertGE0(n) {
      return n === _0n$2 ? n : assertInRange(n, CURVE_ORDER);
    }
    $(assertGE0);
    const pointPrecomputes = new Map();
    function isPoint(other) {
      if (!(other instanceof Point)) throw new Error("ExtendedPoint expected");
    }
    $(isPoint);
    class Point {
      constructor(ex, ey, ez, et) {
        this.ex = ex;
        this.ey = ey;
        this.ez = ez;
        this.et = et;
        if (!in0MaskRange(ex)) throw new Error("x required");
        if (!in0MaskRange(ey)) throw new Error("y required");
        if (!in0MaskRange(ez)) throw new Error("z required");
        if (!in0MaskRange(et)) throw new Error("t required");
      }
      get x() {
        return this.toAffine().x;
      }
      get y() {
        return this.toAffine().y;
      }
      static fromAffine(p) {
        if (p instanceof Point) throw new Error("extended point not allowed");
        const {
          x: x,
          y: y
        } = p || $Object.create(null, undefined);
        if (!in0MaskRange(x) || !in0MaskRange(y)) throw new Error("invalid affine point");
        return new Point(x, y, _1n$4, modP(x * y));
      }
      static normalizeZ(points) {
        const toInv = Fp.invertBatch(points.map($(p => p.ez)));
        return points.map($((p, i) => p.toAffine(toInv[i]))).map(Point.fromAffine);
      }
      _setWindowSize(windowSize) {
        this._WINDOW_SIZE = windowSize;
        pointPrecomputes.delete(this);
      }
      assertValidity() {
        const {
          a: a,
          d: d
        } = CURVE;
        if (this.is0()) throw new Error("bad point: ZERO");
        const {
          ex: X,
          ey: Y,
          ez: Z,
          et: T
        } = this;
        const X2 = modP(X * X);
        const Y2 = modP(Y * Y);
        const Z2 = modP(Z * Z);
        const Z4 = modP(Z2 * Z2);
        const aX2 = modP(X2 * a);
        const left = modP(Z2 * modP(aX2 + Y2));
        const right = modP(Z4 + modP(d * modP(X2 * Y2)));
        if (left !== right) throw new Error("bad point: equation left != right (1)");
        const XY = modP(X * Y);
        const ZT = modP(Z * T);
        if (XY !== ZT) throw new Error("bad point: equation left != right (2)");
      }
      equals(other) {
        isPoint(other);
        const {
          ex: X1,
          ey: Y1,
          ez: Z1
        } = this;
        const {
          ex: X2,
          ey: Y2,
          ez: Z2
        } = other;
        const X1Z2 = modP(X1 * Z2);
        const X2Z1 = modP(X2 * Z1);
        const Y1Z2 = modP(Y1 * Z2);
        const Y2Z1 = modP(Y2 * Z1);
        return X1Z2 === X2Z1 && Y1Z2 === Y2Z1;
      }
      is0() {
        return this.equals(Point.ZERO);
      }
      negate() {
        return new Point(modP(-this.ex), this.ey, this.ez, modP(-this.et));
      }
      double() {
        const {
          a: a
        } = CURVE;
        const {
          ex: X1,
          ey: Y1,
          ez: Z1
        } = this;
        const A = modP(X1 * X1);
        const B = modP(Y1 * Y1);
        const C = modP(_2n$3 * modP(Z1 * Z1));
        const D = modP(a * A);
        const x1y1 = X1 + Y1;
        const E = modP(modP(x1y1 * x1y1) - A - B);
        const G = D + B;
        const F = G - C;
        const H = D - B;
        const X3 = modP(E * F);
        const Y3 = modP(G * H);
        const T3 = modP(E * H);
        const Z3 = modP(F * G);
        return new Point(X3, Y3, Z3, T3);
      }
      add(other) {
        isPoint(other);
        const {
          a: a,
          d: d
        } = CURVE;
        const {
          ex: X1,
          ey: Y1,
          ez: Z1,
          et: T1
        } = this;
        const {
          ex: X2,
          ey: Y2,
          ez: Z2,
          et: T2
        } = other;
        if (a === BigInt(-1)) {
          const A = modP((Y1 - X1) * (Y2 + X2));
          const B = modP((Y1 + X1) * (Y2 - X2));
          const F = modP(B - A);
          if (F === _0n$2) return this.double();
          const C = modP(Z1 * _2n$3 * T2);
          const D = modP(T1 * _2n$3 * Z2);
          const E = D + C;
          const G = B + A;
          const H = D - C;
          const X3 = modP(E * F);
          const Y3 = modP(G * H);
          const T3 = modP(E * H);
          const Z3 = modP(F * G);
          return new Point(X3, Y3, Z3, T3);
        }
        const A = modP(X1 * X2);
        const B = modP(Y1 * Y2);
        const C = modP(T1 * d * T2);
        const D = modP(Z1 * Z2);
        const E = modP((X1 + Y1) * (X2 + Y2) - A - B);
        const F = D - C;
        const G = D + C;
        const H = modP(B - a * A);
        const X3 = modP(E * F);
        const Y3 = modP(G * H);
        const T3 = modP(E * H);
        const Z3 = modP(F * G);
        return new Point(X3, Y3, Z3, T3);
      }
      subtract(other) {
        return this.add(other.negate());
      }
      wNAF(n) {
        return wnaf.wNAFCached(this, pointPrecomputes, n, Point.normalizeZ);
      }
      multiply(scalar) {
        const {
          p: p,
          f: f
        } = this.wNAF(assertInRange(scalar, CURVE_ORDER));
        return Point.normalizeZ($Array.of(p, f))[0];
      }
      multiplyUnsafe(scalar) {
        let n = assertGE0(scalar);
        if (n === _0n$2) return I;
        if (this.equals(I) || n === _1n$4) return this;
        if (this.equals(G)) return this.wNAF(n).p;
        return wnaf.unsafeLadder(this, n);
      }
      isSmallOrder() {
        return this.multiplyUnsafe(cofactor).is0();
      }
      isTorsionFree() {
        return wnaf.unsafeLadder(this, CURVE_ORDER).is0();
      }
      toAffine(iz) {
        const {
          ex: x,
          ey: y,
          ez: z
        } = this;
        const is0 = this.is0();
        if (iz == null) iz = is0 ? _8n : Fp.inv(z);
        const ax = modP(x * iz);
        const ay = modP(y * iz);
        const zz = modP(z * iz);
        if (is0) return $(function () {
          let result = $Object.create(null, undefined);
          result.x = _0n$2;
          result.y = _1n$4;
          return result;
        })();
        if (zz !== _1n$4) throw new Error("invZ was invalid");
        return $(function () {
          let result = $Object.create(null, undefined);
          result.x = ax;
          result.y = ay;
          return result;
        })();
      }
      clearCofactor() {
        const {
          h: cofactor
        } = CURVE;
        if (cofactor === _1n$4) return this;
        return this.multiplyUnsafe(cofactor);
      }
      static fromHex(hex, zip215 = false) {
        const {
          d: d,
          a: a
        } = CURVE;
        const len = Fp.BYTES;
        hex = ensureBytes("pointHex", hex, len);
        const normed = hex.slice();
        const lastByte = hex[len - 1];
        normed[len - 1] = lastByte & ~128;
        const y = bytesToNumberLE(normed);
        if (y === _0n$2) ;else {
          if (zip215) assertInRange(y, MASK);else assertInRange(y, Fp.ORDER);
        }
        const y2 = modP(y * y);
        const u = modP(y2 - _1n$4);
        const v = modP(d * y2 - a);
        let {
          isValid: isValid,
          value: x
        } = uvRatio(u, v);
        if (!isValid) throw new Error("Point.fromHex: invalid y coordinate");
        const isXOdd = (x & _1n$4) === _1n$4;
        const isLastByteOdd = (lastByte & 128) !== 0;
        if (!zip215 && x === _0n$2 && isLastByteOdd) throw new Error("Point.fromHex: x=0 and x_0=1");
        if (isLastByteOdd !== isXOdd) x = modP(-x);
        return Point.fromAffine($(function () {
          let result = $Object.create(null, undefined);
          result.x = x;
          result.y = y;
          return result;
        })());
      }
      static fromPrivateKey(privKey) {
        return getExtendedPublicKey(privKey).point;
      }
      toRawBytes() {
        const {
          x: x,
          y: y
        } = this.toAffine();
        const bytes = numberToBytesLE(y, Fp.BYTES);
        bytes[bytes.length - 1] |= x & _1n$4 ? 128 : 0;
        return bytes;
      }
      toHex() {
        return bytesToHex(this.toRawBytes());
      }
    }
    Point.BASE = new Point(CURVE.Gx, CURVE.Gy, _1n$4, modP(CURVE.Gx * CURVE.Gy));
    Point.ZERO = new Point(_0n$2, _1n$4, _1n$4, _0n$2);
    const {
      BASE: G,
      ZERO: I
    } = Point;
    const wnaf = wNAF(Point, nByteLength * 8);
    function modN(a) {
      return mod(a, CURVE_ORDER);
    }
    $(modN);
    function modN_LE(hash) {
      return modN(bytesToNumberLE(hash));
    }
    $(modN_LE);
    function getExtendedPublicKey(key) {
      const len = nByteLength;
      key = ensureBytes("private key", key, len);
      const hashed = ensureBytes("hashed private key", cHash(key), 2 * len);
      const head = adjustScalarBytes(hashed.slice(0, len));
      const prefix = hashed.slice(len, 2 * len);
      const scalar = modN_LE(head);
      const point = G.multiply(scalar);
      const pointBytes = point.toRawBytes();
      return $(function () {
        let result = $Object.create(null, undefined);
        result.head = head;
        result.prefix = prefix;
        result.scalar = scalar;
        result.point = point;
        result.pointBytes = pointBytes;
        return result;
      })();
    }
    $(getExtendedPublicKey);
    function getPublicKey(privKey) {
      return getExtendedPublicKey(privKey).pointBytes;
    }
    $(getPublicKey);
    function hashDomainToScalar(context = new Uint8Array(), ...msgs) {
      const msg = concatBytes(...msgs);
      return modN_LE(cHash(domain(msg, ensureBytes("context", context), !!prehash)));
    }
    $(hashDomainToScalar);
    function sign(msg, privKey, options = $Object.create(null, undefined)) {
      msg = ensureBytes("message", msg);
      if (prehash) msg = prehash(msg);
      const {
        prefix: prefix,
        scalar: scalar,
        pointBytes: pointBytes
      } = getExtendedPublicKey(privKey);
      const r = hashDomainToScalar(options.context, prefix, msg);
      const R = G.multiply(r).toRawBytes();
      const k = hashDomainToScalar(options.context, R, pointBytes, msg);
      const s = modN(r + k * scalar);
      assertGE0(s);
      const res = concatBytes(R, numberToBytesLE(s, Fp.BYTES));
      return ensureBytes("result", res, nByteLength * 2);
    }
    $(sign);
    const verifyOpts = VERIFY_DEFAULT;
    function verify(sig, msg, publicKey, options = verifyOpts) {
      const {
        context: context,
        zip215: zip215
      } = options;
      const len = Fp.BYTES;
      sig = ensureBytes("signature", sig, 2 * len);
      msg = ensureBytes("message", msg);
      if (prehash) msg = prehash(msg);
      const s = bytesToNumberLE(sig.slice(len, 2 * len));
      let A, R, SB;
      try {
        A = Point.fromHex(publicKey, zip215);
        R = Point.fromHex(sig.slice(0, len), zip215);
        SB = G.multiplyUnsafe(s);
      } catch (error) {
        return false;
      }
      if (!zip215 && A.isSmallOrder()) return false;
      const k = hashDomainToScalar(context, R.toRawBytes(), A.toRawBytes(), msg);
      const RkA = R.add(A.multiplyUnsafe(k));
      return RkA.subtract(SB).clearCofactor().equals(Point.ZERO);
    }
    $(verify);
    G._setWindowSize(8);
    const utils = $(function () {
      let result = $Object.create(null, undefined);
      result.getExtendedPublicKey = getExtendedPublicKey;
      result.randomPrivateKey = $(() => randomBytes(Fp.BYTES));
      result.precompute = $(function (windowSize = 8, point = Point.BASE) {
        point._setWindowSize(windowSize);
        point.multiply(BigInt(3));
        return point;
      });
      return result;
    })();
    return $(function () {
      let result = $Object.create(null, undefined);
      result.CURVE = CURVE;
      result.getPublicKey = getPublicKey;
      result.sign = sign;
      result.verify = verify;
      result.ExtendedPoint = Point;
      result.utils = utils;
      return result;
    })();
  }
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  $(twistedEdwards);
  const ED25519_P = BigInt("57896044618658097711785492504343953926634992332820282019728792003956564819949");
  const ED25519_SQRT_M1 = BigInt("19681161376707505956807079304988542015446066515923890162744021073123829784752");
  BigInt(0);
  const _1n$3 = BigInt(1),
    _2n$2 = BigInt(2),
    _5n = BigInt(5);
  const _10n = BigInt(10),
    _20n = BigInt(20),
    _40n = BigInt(40),
    _80n = BigInt(80);
  function ed25519_pow_2_252_3(x) {
    const P = ED25519_P;
    const x2 = x * x % P;
    const b2 = x2 * x % P;
    const b4 = pow2(b2, _2n$2, P) * b2 % P;
    const b5 = pow2(b4, _1n$3, P) * x % P;
    const b10 = pow2(b5, _5n, P) * b5 % P;
    const b20 = pow2(b10, _10n, P) * b10 % P;
    const b40 = pow2(b20, _20n, P) * b20 % P;
    const b80 = pow2(b40, _40n, P) * b40 % P;
    const b160 = pow2(b80, _80n, P) * b80 % P;
    const b240 = pow2(b160, _80n, P) * b80 % P;
    const b250 = pow2(b240, _10n, P) * b10 % P;
    const pow_p_5_8 = pow2(b250, _2n$2, P) * x % P;
    return $(function () {
      let result = $Object.create(null, undefined);
      result.pow_p_5_8 = pow_p_5_8;
      result.b2 = b2;
      return result;
    })();
  }
  $(ed25519_pow_2_252_3);
  function adjustScalarBytes(bytes) {
    bytes[0] &= 248;
    bytes[31] &= 127;
    bytes[31] |= 64;
    return bytes;
  }
  $(adjustScalarBytes);
  function uvRatio(u, v) {
    const P = ED25519_P;
    const v3 = mod(v * v * v, P);
    const v7 = mod(v3 * v3 * v, P);
    const pow = ed25519_pow_2_252_3(u * v7).pow_p_5_8;
    let x = mod(u * v3 * pow, P);
    const vx2 = mod(v * x * x, P);
    const root1 = x;
    const root2 = mod(x * ED25519_SQRT_M1, P);
    const useRoot1 = vx2 === u;
    const useRoot2 = vx2 === mod(-u, P);
    const noRoot = vx2 === mod(-u * ED25519_SQRT_M1, P);
    if (useRoot1) x = root1;
    if (useRoot2 || noRoot) x = root2;
    if (isNegativeLE(x, P)) x = mod(-x, P);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.isValid = useRoot1 || useRoot2;
      result.value = x;
      return result;
    })();
  }
  $(uvRatio);
  const Fp$1 = Field(ED25519_P, undefined, true);
  const ed25519Defaults = $(function () {
    let result = $Object.create(null, undefined);
    result.a = BigInt(-1);
    result.d = BigInt("37095705934669439343138083508754565189542113879843219016388785533085940283555");
    result.Fp = Fp$1;
    result.n = BigInt("7237005577332262213973186563042994240857116359379907606001950938285454250989");
    result.h = BigInt(8);
    result.Gx = BigInt("15112221349535400772501151409588531511454012693041857206046113283949847762202");
    result.Gy = BigInt("46316835694926478169428394003475163141307993866256225615783033603165251855960");
    result.hash = sha512;
    result.randomBytes = randomBytes;
    result.adjustScalarBytes = adjustScalarBytes;
    result.uvRatio = uvRatio;
    return result;
  })();
  const ed25519 = twistedEdwards(ed25519Defaults);
  function ed25519_domain(data, ctx, phflag) {
    if (ctx.length > 255) throw new Error("Context is too big");
    return concatBytes$1(utf8ToBytes$2("SigEd25519 no Ed25519 collisions"), new Uint8Array($Array.of(phflag ? 1 : 0, ctx.length)), ctx, data);
  }
  $(ed25519_domain);
  twistedEdwards($(function () {
    let result = $Object.create(null, undefined);
    $Object.assign(result, ed25519Defaults);
    result.domain = ed25519_domain;
    return result;
  })());
  twistedEdwards($(function () {
    let result = $Object.create(null, undefined);
    $Object.assign(result, ed25519Defaults);
    result.domain = ed25519_domain;
    result.prehash = sha512;
    return result;
  })());
  const ELL2_C1 = (Fp$1.ORDER + BigInt(3)) / BigInt(8);
  Fp$1.pow(_2n$2, ELL2_C1);
  Fp$1.sqrt(Fp$1.neg(Fp$1.ONE));
  (Fp$1.ORDER - BigInt(5)) / BigInt(8);
  BigInt(486662);
  FpSqrtEven(Fp$1, Fp$1.neg(BigInt(486664)));
  BigInt("25063068953384623474111414158702152701244531502492656460079210482610430750235");
  BigInt("54469307008909316920995813868745141605393597292927456921205312896311721017578");
  BigInt("1159843021668779879193775521855586647937357759715417654439879720876111806838");
  BigInt("40440834346308536858101042469323190826248399146238708352240133220865137265952");
  BigInt("0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
  const generatePrivateKey = ed25519.utils.randomPrivateKey;
  const generateKeypair = () => {
    const privateScalar = ed25519.utils.randomPrivateKey();
    const publicKey = getPublicKey(privateScalar);
    const secretKey = new Uint8Array(64);
    secretKey.set(privateScalar);
    secretKey.set(publicKey, 32);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.publicKey = publicKey;
      result.secretKey = secretKey;
      return result;
    })();
  };
  $(generateKeypair);
  const getPublicKey = ed25519.getPublicKey;
  function isOnCurve(publicKey) {
    try {
      ed25519.ExtendedPoint.fromHex(publicKey);
      return true;
    } catch {
      return false;
    }
  }
  $(isOnCurve);
  const sign = (message, secretKey) => ed25519.sign(message, secretKey.slice(0, 32));
  $(sign);
  const verify = ed25519.verify;
  const toBuffer = arr => {
    if (buffer.Buffer.isBuffer(arr)) {
      return arr;
    } else if (arr instanceof Uint8Array) {
      return buffer.Buffer.from(arr.buffer, arr.byteOffset, arr.byteLength);
    } else {
      return buffer.Buffer.from(arr);
    }
  };
  $(toBuffer);
  var bn = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var _nodeResolve_empty = $Object.create(null, undefined);
  var _nodeResolve_empty$1 = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.__proto__ = null;
    result.default = _nodeResolve_empty;
    return result;
  })());
  var require$$0$1 = getAugmentedNamespace(_nodeResolve_empty$1);
  bn.exports;
  $(function (module) {
    $(function (module, exports) {
      function assert(val, msg) {
        if (!val) throw new Error(msg || "Assertion failed");
      }
      $(assert);
      function inherits(ctor, superCtor) {
        ctor.super_ = superCtor;
        var TempCtor = function () {};
        $(TempCtor);
        TempCtor.prototype = superCtor.prototype;
        ctor.prototype = new TempCtor();
        ctor.prototype.constructor = ctor;
      }
      $(inherits);
      function BN(number, base, endian) {
        if (BN.isBN(number)) {
          return number;
        }
        this.negative = 0;
        this.words = null;
        this.length = 0;
        this.red = null;
        if (number !== null) {
          if (base === "le" || base === "be") {
            endian = base;
            base = 10;
          }
          this._init(number || 0, base || 10, endian || "be");
        }
      }
      $(BN);
      if (typeof module === "object") {
        module.exports = BN;
      } else {
        exports.BN = BN;
      }
      BN.BN = BN;
      BN.wordSize = 26;
      var Buffer;
      try {
        if (typeof window !== "undefined" && typeof window.Buffer !== "undefined") {
          Buffer = window.Buffer;
        } else {
          Buffer = require$$0$1.Buffer;
        }
      } catch (e) {}
      BN.isBN = $(function isBN(num) {
        if (num instanceof BN) {
          return true;
        }
        return num !== null && typeof num === "object" && num.constructor.wordSize === BN.wordSize && $Array.isArray(num.words);
      });
      BN.max = $(function max(left, right) {
        if (left.cmp(right) > 0) return left;
        return right;
      });
      BN.min = $(function min(left, right) {
        if (left.cmp(right) < 0) return left;
        return right;
      });
      BN.prototype._init = $(function init(number, base, endian) {
        if (typeof number === "number") {
          return this._initNumber(number, base, endian);
        }
        if (typeof number === "object") {
          return this._initArray(number, base, endian);
        }
        if (base === "hex") {
          base = 16;
        }
        assert(base === (base | 0) && base >= 2 && base <= 36);
        number = number.toString().replace(/\s+/g, "");
        var start = 0;
        if (number[0] === "-") {
          start++;
          this.negative = 1;
        }
        if (start < number.length) {
          if (base === 16) {
            this._parseHex(number, start, endian);
          } else {
            this._parseBase(number, base, start);
            if (endian === "le") {
              this._initArray(this.toArray(), base, endian);
            }
          }
        }
      });
      BN.prototype._initNumber = $(function _initNumber(number, base, endian) {
        if (number < 0) {
          this.negative = 1;
          number = -number;
        }
        if (number < 67108864) {
          this.words = $Array.of(number & 67108863);
          this.length = 1;
        } else if (number < 4503599627370496) {
          this.words = $Array.of(number & 67108863, number / 67108864 & 67108863);
          this.length = 2;
        } else {
          assert(number < 9007199254740992);
          this.words = $Array.of(number & 67108863, number / 67108864 & 67108863, 1);
          this.length = 3;
        }
        if (endian !== "le") return;
        this._initArray(this.toArray(), base, endian);
      });
      BN.prototype._initArray = $(function _initArray(number, base, endian) {
        assert(typeof number.length === "number");
        if (number.length <= 0) {
          this.words = $Array.of(0);
          this.length = 1;
          return this;
        }
        this.length = Math.ceil(number.length / 3);
        this.words = new Array(this.length);
        for (var i = 0; i < this.length; i++) {
          this.words[i] = 0;
        }
        var j, w;
        var off = 0;
        if (endian === "be") {
          for (i = number.length - 1, j = 0; i >= 0; i -= 3) {
            w = number[i] | number[i - 1] << 8 | number[i - 2] << 16;
            this.words[j] |= w << off & 67108863;
            this.words[j + 1] = w >>> 26 - off & 67108863;
            off += 24;
            if (off >= 26) {
              off -= 26;
              j++;
            }
          }
        } else if (endian === "le") {
          for (i = 0, j = 0; i < number.length; i += 3) {
            w = number[i] | number[i + 1] << 8 | number[i + 2] << 16;
            this.words[j] |= w << off & 67108863;
            this.words[j + 1] = w >>> 26 - off & 67108863;
            off += 24;
            if (off >= 26) {
              off -= 26;
              j++;
            }
          }
        }
        return this._strip();
      });
      function parseHex4Bits(string, index) {
        var c = string.charCodeAt(index);
        if (c >= 48 && c <= 57) {
          return c - 48;
        } else if (c >= 65 && c <= 70) {
          return c - 55;
        } else if (c >= 97 && c <= 102) {
          return c - 87;
        } else {
          assert(false, "Invalid character in " + string);
        }
      }
      $(parseHex4Bits);
      function parseHexByte(string, lowerBound, index) {
        var r = parseHex4Bits(string, index);
        if (index - 1 >= lowerBound) {
          r |= parseHex4Bits(string, index - 1) << 4;
        }
        return r;
      }
      $(parseHexByte);
      BN.prototype._parseHex = $(function _parseHex(number, start, endian) {
        this.length = Math.ceil((number.length - start) / 6);
        this.words = new Array(this.length);
        for (var i = 0; i < this.length; i++) {
          this.words[i] = 0;
        }
        var off = 0;
        var j = 0;
        var w;
        if (endian === "be") {
          for (i = number.length - 1; i >= start; i -= 2) {
            w = parseHexByte(number, start, i) << off;
            this.words[j] |= w & 67108863;
            if (off >= 18) {
              off -= 18;
              j += 1;
              this.words[j] |= w >>> 26;
            } else {
              off += 8;
            }
          }
        } else {
          var parseLength = number.length - start;
          for (i = parseLength % 2 === 0 ? start + 1 : start; i < number.length; i += 2) {
            w = parseHexByte(number, start, i) << off;
            this.words[j] |= w & 67108863;
            if (off >= 18) {
              off -= 18;
              j += 1;
              this.words[j] |= w >>> 26;
            } else {
              off += 8;
            }
          }
        }
        this._strip();
      });
      function parseBase(str, start, end, mul) {
        var r = 0;
        var b = 0;
        var len = Math.min(str.length, end);
        for (var i = start; i < len; i++) {
          var c = str.charCodeAt(i) - 48;
          r *= mul;
          if (c >= 49) {
            b = c - 49 + 10;
          } else if (c >= 17) {
            b = c - 17 + 10;
          } else {
            b = c;
          }
          assert(c >= 0 && b < mul, "Invalid character");
          r += b;
        }
        return r;
      }
      $(parseBase);
      BN.prototype._parseBase = $(function _parseBase(number, base, start) {
        this.words = $Array.of(0);
        this.length = 1;
        for (var limbLen = 0, limbPow = 1; limbPow <= 67108863; limbPow *= base) {
          limbLen++;
        }
        limbLen--;
        limbPow = limbPow / base | 0;
        var total = number.length - start;
        var mod = total % limbLen;
        var end = Math.min(total, total - mod) + start;
        var word = 0;
        for (var i = start; i < end; i += limbLen) {
          word = parseBase(number, i, i + limbLen, base);
          this.imuln(limbPow);
          if (this.words[0] + word < 67108864) {
            this.words[0] += word;
          } else {
            this._iaddn(word);
          }
        }
        if (mod !== 0) {
          var pow = 1;
          word = parseBase(number, i, number.length, base);
          for (i = 0; i < mod; i++) {
            pow *= base;
          }
          this.imuln(pow);
          if (this.words[0] + word < 67108864) {
            this.words[0] += word;
          } else {
            this._iaddn(word);
          }
        }
        this._strip();
      });
      BN.prototype.copy = $(function copy(dest) {
        dest.words = new Array(this.length);
        for (var i = 0; i < this.length; i++) {
          dest.words[i] = this.words[i];
        }
        dest.length = this.length;
        dest.negative = this.negative;
        dest.red = this.red;
      });
      function move(dest, src) {
        dest.words = src.words;
        dest.length = src.length;
        dest.negative = src.negative;
        dest.red = src.red;
      }
      $(move);
      BN.prototype._move = $(function _move(dest) {
        move(dest, this);
      });
      BN.prototype.clone = $(function clone() {
        var r = new BN(null);
        this.copy(r);
        return r;
      });
      BN.prototype._expand = $(function _expand(size) {
        while (this.length < size) {
          this.words[this.length++] = 0;
        }
        return this;
      });
      BN.prototype._strip = $(function strip() {
        while (this.length > 1 && this.words[this.length - 1] === 0) {
          this.length--;
        }
        return this._normSign();
      });
      BN.prototype._normSign = $(function _normSign() {
        if (this.length === 1 && this.words[0] === 0) {
          this.negative = 0;
        }
        return this;
      });
      if (typeof Symbol !== "undefined" && typeof Symbol.for === "function") {
        try {
          BN.prototype[Symbol.for("nodejs.util.inspect.custom")] = inspect;
        } catch (e) {
          BN.prototype.inspect = inspect;
        }
      } else {
        BN.prototype.inspect = inspect;
      }
      function inspect() {
        return (this.red ? "<BN-R: " : "<BN: ") + this.toString(16) + ">";
      }
      $(inspect);
      var zeros = $Array.of("", "0", "00", "000", "0000", "00000", "000000", "0000000", "00000000", "000000000", "0000000000", "00000000000", "000000000000", "0000000000000", "00000000000000", "000000000000000", "0000000000000000", "00000000000000000", "000000000000000000", "0000000000000000000", "00000000000000000000", "000000000000000000000", "0000000000000000000000", "00000000000000000000000", "000000000000000000000000", "0000000000000000000000000");
      var groupSizes = $Array.of(0, 0, 25, 16, 12, 11, 10, 9, 8, 8, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5);
      var groupBases = $Array.of(0, 0, 33554432, 43046721, 16777216, 48828125, 60466176, 40353607, 16777216, 43046721, 1e7, 19487171, 35831808, 62748517, 7529536, 11390625, 16777216, 24137569, 34012224, 47045881, 64e6, 4084101, 5153632, 6436343, 7962624, 9765625, 11881376, 14348907, 17210368, 20511149, 243e5, 28629151, 33554432, 39135393, 45435424, 52521875, 60466176);
      BN.prototype.toString = $(function toString(base, padding) {
        base = base || 10;
        padding = padding | 0 || 1;
        var out;
        if (base === 16 || base === "hex") {
          out = "";
          var off = 0;
          var carry = 0;
          for (var i = 0; i < this.length; i++) {
            var w = this.words[i];
            var word = ((w << off | carry) & 16777215).toString(16);
            carry = w >>> 24 - off & 16777215;
            off += 2;
            if (off >= 26) {
              off -= 26;
              i--;
            }
            if (carry !== 0 || i !== this.length - 1) {
              out = zeros[6 - word.length] + word + out;
            } else {
              out = word + out;
            }
          }
          if (carry !== 0) {
            out = carry.toString(16) + out;
          }
          while (out.length % padding !== 0) {
            out = "0" + out;
          }
          if (this.negative !== 0) {
            out = "-" + out;
          }
          return out;
        }
        if (base === (base | 0) && base >= 2 && base <= 36) {
          var groupSize = groupSizes[base];
          var groupBase = groupBases[base];
          out = "";
          var c = this.clone();
          c.negative = 0;
          while (!c.isZero()) {
            var r = c.modrn(groupBase).toString(base);
            c = c.idivn(groupBase);
            if (!c.isZero()) {
              out = zeros[groupSize - r.length] + r + out;
            } else {
              out = r + out;
            }
          }
          if (this.isZero()) {
            out = "0" + out;
          }
          while (out.length % padding !== 0) {
            out = "0" + out;
          }
          if (this.negative !== 0) {
            out = "-" + out;
          }
          return out;
        }
        assert(false, "Base should be between 2 and 36");
      });
      BN.prototype.toNumber = $(function toNumber() {
        var ret = this.words[0];
        if (this.length === 2) {
          ret += this.words[1] * 67108864;
        } else if (this.length === 3 && this.words[2] === 1) {
          ret += 4503599627370496 + this.words[1] * 67108864;
        } else if (this.length > 2) {
          assert(false, "Number can only safely store up to 53 bits");
        }
        return this.negative !== 0 ? -ret : ret;
      });
      BN.prototype.toJSON = $(function toJSON() {
        return this.toString(16, 2);
      });
      if (Buffer) {
        BN.prototype.toBuffer = $(function toBuffer(endian, length) {
          return this.toArrayLike(Buffer, endian, length);
        });
      }
      BN.prototype.toArray = $(function toArray(endian, length) {
        return this.toArrayLike(Array, endian, length);
      });
      var allocate = function allocate(ArrayType, size) {
        if (ArrayType.allocUnsafe) {
          return ArrayType.allocUnsafe(size);
        }
        return new ArrayType(size);
      };
      $(allocate);
      BN.prototype.toArrayLike = $(function toArrayLike(ArrayType, endian, length) {
        this._strip();
        var byteLength = this.byteLength();
        var reqLength = length || Math.max(1, byteLength);
        assert(byteLength <= reqLength, "byte array longer than desired length");
        assert(reqLength > 0, "Requested array length <= 0");
        var res = allocate(ArrayType, reqLength);
        var postfix = endian === "le" ? "LE" : "BE";
        this["_toArrayLike" + postfix](res, byteLength);
        return res;
      });
      BN.prototype._toArrayLikeLE = $(function _toArrayLikeLE(res, byteLength) {
        var position = 0;
        var carry = 0;
        for (var i = 0, shift = 0; i < this.length; i++) {
          var word = this.words[i] << shift | carry;
          res[position++] = word & 255;
          if (position < res.length) {
            res[position++] = word >> 8 & 255;
          }
          if (position < res.length) {
            res[position++] = word >> 16 & 255;
          }
          if (shift === 6) {
            if (position < res.length) {
              res[position++] = word >> 24 & 255;
            }
            carry = 0;
            shift = 0;
          } else {
            carry = word >>> 24;
            shift += 2;
          }
        }
        if (position < res.length) {
          res[position++] = carry;
          while (position < res.length) {
            res[position++] = 0;
          }
        }
      });
      BN.prototype._toArrayLikeBE = $(function _toArrayLikeBE(res, byteLength) {
        var position = res.length - 1;
        var carry = 0;
        for (var i = 0, shift = 0; i < this.length; i++) {
          var word = this.words[i] << shift | carry;
          res[position--] = word & 255;
          if (position >= 0) {
            res[position--] = word >> 8 & 255;
          }
          if (position >= 0) {
            res[position--] = word >> 16 & 255;
          }
          if (shift === 6) {
            if (position >= 0) {
              res[position--] = word >> 24 & 255;
            }
            carry = 0;
            shift = 0;
          } else {
            carry = word >>> 24;
            shift += 2;
          }
        }
        if (position >= 0) {
          res[position--] = carry;
          while (position >= 0) {
            res[position--] = 0;
          }
        }
      });
      if (Math.clz32) {
        BN.prototype._countBits = $(function _countBits(w) {
          return 32 - Math.clz32(w);
        });
      } else {
        BN.prototype._countBits = $(function _countBits(w) {
          var t = w;
          var r = 0;
          if (t >= 4096) {
            r += 13;
            t >>>= 13;
          }
          if (t >= 64) {
            r += 7;
            t >>>= 7;
          }
          if (t >= 8) {
            r += 4;
            t >>>= 4;
          }
          if (t >= 2) {
            r += 2;
            t >>>= 2;
          }
          return r + t;
        });
      }
      BN.prototype._zeroBits = $(function _zeroBits(w) {
        if (w === 0) return 26;
        var t = w;
        var r = 0;
        if ((t & 8191) === 0) {
          r += 13;
          t >>>= 13;
        }
        if ((t & 127) === 0) {
          r += 7;
          t >>>= 7;
        }
        if ((t & 15) === 0) {
          r += 4;
          t >>>= 4;
        }
        if ((t & 3) === 0) {
          r += 2;
          t >>>= 2;
        }
        if ((t & 1) === 0) {
          r++;
        }
        return r;
      });
      BN.prototype.bitLength = $(function bitLength() {
        var w = this.words[this.length - 1];
        var hi = this._countBits(w);
        return (this.length - 1) * 26 + hi;
      });
      function toBitArray(num) {
        var w = new Array(num.bitLength());
        for (var bit = 0; bit < w.length; bit++) {
          var off = bit / 26 | 0;
          var wbit = bit % 26;
          w[bit] = num.words[off] >>> wbit & 1;
        }
        return w;
      }
      $(toBitArray);
      BN.prototype.zeroBits = $(function zeroBits() {
        if (this.isZero()) return 0;
        var r = 0;
        for (var i = 0; i < this.length; i++) {
          var b = this._zeroBits(this.words[i]);
          r += b;
          if (b !== 26) break;
        }
        return r;
      });
      BN.prototype.byteLength = $(function byteLength() {
        return Math.ceil(this.bitLength() / 8);
      });
      BN.prototype.toTwos = $(function toTwos(width) {
        if (this.negative !== 0) {
          return this.abs().inotn(width).iaddn(1);
        }
        return this.clone();
      });
      BN.prototype.fromTwos = $(function fromTwos(width) {
        if (this.testn(width - 1)) {
          return this.notn(width).iaddn(1).ineg();
        }
        return this.clone();
      });
      BN.prototype.isNeg = $(function isNeg() {
        return this.negative !== 0;
      });
      BN.prototype.neg = $(function neg() {
        return this.clone().ineg();
      });
      BN.prototype.ineg = $(function ineg() {
        if (!this.isZero()) {
          this.negative ^= 1;
        }
        return this;
      });
      BN.prototype.iuor = $(function iuor(num) {
        while (this.length < num.length) {
          this.words[this.length++] = 0;
        }
        for (var i = 0; i < num.length; i++) {
          this.words[i] = this.words[i] | num.words[i];
        }
        return this._strip();
      });
      BN.prototype.ior = $(function ior(num) {
        assert((this.negative | num.negative) === 0);
        return this.iuor(num);
      });
      BN.prototype.or = $(function or(num) {
        if (this.length > num.length) return this.clone().ior(num);
        return num.clone().ior(this);
      });
      BN.prototype.uor = $(function uor(num) {
        if (this.length > num.length) return this.clone().iuor(num);
        return num.clone().iuor(this);
      });
      BN.prototype.iuand = $(function iuand(num) {
        var b;
        if (this.length > num.length) {
          b = num;
        } else {
          b = this;
        }
        for (var i = 0; i < b.length; i++) {
          this.words[i] = this.words[i] & num.words[i];
        }
        this.length = b.length;
        return this._strip();
      });
      BN.prototype.iand = $(function iand(num) {
        assert((this.negative | num.negative) === 0);
        return this.iuand(num);
      });
      BN.prototype.and = $(function and(num) {
        if (this.length > num.length) return this.clone().iand(num);
        return num.clone().iand(this);
      });
      BN.prototype.uand = $(function uand(num) {
        if (this.length > num.length) return this.clone().iuand(num);
        return num.clone().iuand(this);
      });
      BN.prototype.iuxor = $(function iuxor(num) {
        var a;
        var b;
        if (this.length > num.length) {
          a = this;
          b = num;
        } else {
          a = num;
          b = this;
        }
        for (var i = 0; i < b.length; i++) {
          this.words[i] = a.words[i] ^ b.words[i];
        }
        if (this !== a) {
          for (; i < a.length; i++) {
            this.words[i] = a.words[i];
          }
        }
        this.length = a.length;
        return this._strip();
      });
      BN.prototype.ixor = $(function ixor(num) {
        assert((this.negative | num.negative) === 0);
        return this.iuxor(num);
      });
      BN.prototype.xor = $(function xor(num) {
        if (this.length > num.length) return this.clone().ixor(num);
        return num.clone().ixor(this);
      });
      BN.prototype.uxor = $(function uxor(num) {
        if (this.length > num.length) return this.clone().iuxor(num);
        return num.clone().iuxor(this);
      });
      BN.prototype.inotn = $(function inotn(width) {
        assert(typeof width === "number" && width >= 0);
        var bytesNeeded = Math.ceil(width / 26) | 0;
        var bitsLeft = width % 26;
        this._expand(bytesNeeded);
        if (bitsLeft > 0) {
          bytesNeeded--;
        }
        for (var i = 0; i < bytesNeeded; i++) {
          this.words[i] = ~this.words[i] & 67108863;
        }
        if (bitsLeft > 0) {
          this.words[i] = ~this.words[i] & 67108863 >> 26 - bitsLeft;
        }
        return this._strip();
      });
      BN.prototype.notn = $(function notn(width) {
        return this.clone().inotn(width);
      });
      BN.prototype.setn = $(function setn(bit, val) {
        assert(typeof bit === "number" && bit >= 0);
        var off = bit / 26 | 0;
        var wbit = bit % 26;
        this._expand(off + 1);
        if (val) {
          this.words[off] = this.words[off] | 1 << wbit;
        } else {
          this.words[off] = this.words[off] & ~(1 << wbit);
        }
        return this._strip();
      });
      BN.prototype.iadd = $(function iadd(num) {
        var r;
        if (this.negative !== 0 && num.negative === 0) {
          this.negative = 0;
          r = this.isub(num);
          this.negative ^= 1;
          return this._normSign();
        } else if (this.negative === 0 && num.negative !== 0) {
          num.negative = 0;
          r = this.isub(num);
          num.negative = 1;
          return r._normSign();
        }
        var a, b;
        if (this.length > num.length) {
          a = this;
          b = num;
        } else {
          a = num;
          b = this;
        }
        var carry = 0;
        for (var i = 0; i < b.length; i++) {
          r = (a.words[i] | 0) + (b.words[i] | 0) + carry;
          this.words[i] = r & 67108863;
          carry = r >>> 26;
        }
        for (; carry !== 0 && i < a.length; i++) {
          r = (a.words[i] | 0) + carry;
          this.words[i] = r & 67108863;
          carry = r >>> 26;
        }
        this.length = a.length;
        if (carry !== 0) {
          this.words[this.length] = carry;
          this.length++;
        } else if (a !== this) {
          for (; i < a.length; i++) {
            this.words[i] = a.words[i];
          }
        }
        return this;
      });
      BN.prototype.add = $(function add(num) {
        var res;
        if (num.negative !== 0 && this.negative === 0) {
          num.negative = 0;
          res = this.sub(num);
          num.negative ^= 1;
          return res;
        } else if (num.negative === 0 && this.negative !== 0) {
          this.negative = 0;
          res = num.sub(this);
          this.negative = 1;
          return res;
        }
        if (this.length > num.length) return this.clone().iadd(num);
        return num.clone().iadd(this);
      });
      BN.prototype.isub = $(function isub(num) {
        if (num.negative !== 0) {
          num.negative = 0;
          var r = this.iadd(num);
          num.negative = 1;
          return r._normSign();
        } else if (this.negative !== 0) {
          this.negative = 0;
          this.iadd(num);
          this.negative = 1;
          return this._normSign();
        }
        var cmp = this.cmp(num);
        if (cmp === 0) {
          this.negative = 0;
          this.length = 1;
          this.words[0] = 0;
          return this;
        }
        var a, b;
        if (cmp > 0) {
          a = this;
          b = num;
        } else {
          a = num;
          b = this;
        }
        var carry = 0;
        for (var i = 0; i < b.length; i++) {
          r = (a.words[i] | 0) - (b.words[i] | 0) + carry;
          carry = r >> 26;
          this.words[i] = r & 67108863;
        }
        for (; carry !== 0 && i < a.length; i++) {
          r = (a.words[i] | 0) + carry;
          carry = r >> 26;
          this.words[i] = r & 67108863;
        }
        if (carry === 0 && i < a.length && a !== this) {
          for (; i < a.length; i++) {
            this.words[i] = a.words[i];
          }
        }
        this.length = Math.max(this.length, i);
        if (a !== this) {
          this.negative = 1;
        }
        return this._strip();
      });
      BN.prototype.sub = $(function sub(num) {
        return this.clone().isub(num);
      });
      function smallMulTo(self, num, out) {
        out.negative = num.negative ^ self.negative;
        var len = self.length + num.length | 0;
        out.length = len;
        len = len - 1 | 0;
        var a = self.words[0] | 0;
        var b = num.words[0] | 0;
        var r = a * b;
        var lo = r & 67108863;
        var carry = r / 67108864 | 0;
        out.words[0] = lo;
        for (var k = 1; k < len; k++) {
          var ncarry = carry >>> 26;
          var rword = carry & 67108863;
          var maxJ = Math.min(k, num.length - 1);
          for (var j = Math.max(0, k - self.length + 1); j <= maxJ; j++) {
            var i = k - j | 0;
            a = self.words[i] | 0;
            b = num.words[j] | 0;
            r = a * b + rword;
            ncarry += r / 67108864 | 0;
            rword = r & 67108863;
          }
          out.words[k] = rword | 0;
          carry = ncarry | 0;
        }
        if (carry !== 0) {
          out.words[k] = carry | 0;
        } else {
          out.length--;
        }
        return out._strip();
      }
      $(smallMulTo);
      var comb10MulTo = function comb10MulTo(self, num, out) {
        var a = self.words;
        var b = num.words;
        var o = out.words;
        var c = 0;
        var lo;
        var mid;
        var hi;
        var a0 = a[0] | 0;
        var al0 = a0 & 8191;
        var ah0 = a0 >>> 13;
        var a1 = a[1] | 0;
        var al1 = a1 & 8191;
        var ah1 = a1 >>> 13;
        var a2 = a[2] | 0;
        var al2 = a2 & 8191;
        var ah2 = a2 >>> 13;
        var a3 = a[3] | 0;
        var al3 = a3 & 8191;
        var ah3 = a3 >>> 13;
        var a4 = a[4] | 0;
        var al4 = a4 & 8191;
        var ah4 = a4 >>> 13;
        var a5 = a[5] | 0;
        var al5 = a5 & 8191;
        var ah5 = a5 >>> 13;
        var a6 = a[6] | 0;
        var al6 = a6 & 8191;
        var ah6 = a6 >>> 13;
        var a7 = a[7] | 0;
        var al7 = a7 & 8191;
        var ah7 = a7 >>> 13;
        var a8 = a[8] | 0;
        var al8 = a8 & 8191;
        var ah8 = a8 >>> 13;
        var a9 = a[9] | 0;
        var al9 = a9 & 8191;
        var ah9 = a9 >>> 13;
        var b0 = b[0] | 0;
        var bl0 = b0 & 8191;
        var bh0 = b0 >>> 13;
        var b1 = b[1] | 0;
        var bl1 = b1 & 8191;
        var bh1 = b1 >>> 13;
        var b2 = b[2] | 0;
        var bl2 = b2 & 8191;
        var bh2 = b2 >>> 13;
        var b3 = b[3] | 0;
        var bl3 = b3 & 8191;
        var bh3 = b3 >>> 13;
        var b4 = b[4] | 0;
        var bl4 = b4 & 8191;
        var bh4 = b4 >>> 13;
        var b5 = b[5] | 0;
        var bl5 = b5 & 8191;
        var bh5 = b5 >>> 13;
        var b6 = b[6] | 0;
        var bl6 = b6 & 8191;
        var bh6 = b6 >>> 13;
        var b7 = b[7] | 0;
        var bl7 = b7 & 8191;
        var bh7 = b7 >>> 13;
        var b8 = b[8] | 0;
        var bl8 = b8 & 8191;
        var bh8 = b8 >>> 13;
        var b9 = b[9] | 0;
        var bl9 = b9 & 8191;
        var bh9 = b9 >>> 13;
        out.negative = self.negative ^ num.negative;
        out.length = 19;
        lo = Math.imul(al0, bl0);
        mid = Math.imul(al0, bh0);
        mid = mid + Math.imul(ah0, bl0) | 0;
        hi = Math.imul(ah0, bh0);
        var w0 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w0 >>> 26) | 0;
        w0 &= 67108863;
        lo = Math.imul(al1, bl0);
        mid = Math.imul(al1, bh0);
        mid = mid + Math.imul(ah1, bl0) | 0;
        hi = Math.imul(ah1, bh0);
        lo = lo + Math.imul(al0, bl1) | 0;
        mid = mid + Math.imul(al0, bh1) | 0;
        mid = mid + Math.imul(ah0, bl1) | 0;
        hi = hi + Math.imul(ah0, bh1) | 0;
        var w1 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w1 >>> 26) | 0;
        w1 &= 67108863;
        lo = Math.imul(al2, bl0);
        mid = Math.imul(al2, bh0);
        mid = mid + Math.imul(ah2, bl0) | 0;
        hi = Math.imul(ah2, bh0);
        lo = lo + Math.imul(al1, bl1) | 0;
        mid = mid + Math.imul(al1, bh1) | 0;
        mid = mid + Math.imul(ah1, bl1) | 0;
        hi = hi + Math.imul(ah1, bh1) | 0;
        lo = lo + Math.imul(al0, bl2) | 0;
        mid = mid + Math.imul(al0, bh2) | 0;
        mid = mid + Math.imul(ah0, bl2) | 0;
        hi = hi + Math.imul(ah0, bh2) | 0;
        var w2 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w2 >>> 26) | 0;
        w2 &= 67108863;
        lo = Math.imul(al3, bl0);
        mid = Math.imul(al3, bh0);
        mid = mid + Math.imul(ah3, bl0) | 0;
        hi = Math.imul(ah3, bh0);
        lo = lo + Math.imul(al2, bl1) | 0;
        mid = mid + Math.imul(al2, bh1) | 0;
        mid = mid + Math.imul(ah2, bl1) | 0;
        hi = hi + Math.imul(ah2, bh1) | 0;
        lo = lo + Math.imul(al1, bl2) | 0;
        mid = mid + Math.imul(al1, bh2) | 0;
        mid = mid + Math.imul(ah1, bl2) | 0;
        hi = hi + Math.imul(ah1, bh2) | 0;
        lo = lo + Math.imul(al0, bl3) | 0;
        mid = mid + Math.imul(al0, bh3) | 0;
        mid = mid + Math.imul(ah0, bl3) | 0;
        hi = hi + Math.imul(ah0, bh3) | 0;
        var w3 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w3 >>> 26) | 0;
        w3 &= 67108863;
        lo = Math.imul(al4, bl0);
        mid = Math.imul(al4, bh0);
        mid = mid + Math.imul(ah4, bl0) | 0;
        hi = Math.imul(ah4, bh0);
        lo = lo + Math.imul(al3, bl1) | 0;
        mid = mid + Math.imul(al3, bh1) | 0;
        mid = mid + Math.imul(ah3, bl1) | 0;
        hi = hi + Math.imul(ah3, bh1) | 0;
        lo = lo + Math.imul(al2, bl2) | 0;
        mid = mid + Math.imul(al2, bh2) | 0;
        mid = mid + Math.imul(ah2, bl2) | 0;
        hi = hi + Math.imul(ah2, bh2) | 0;
        lo = lo + Math.imul(al1, bl3) | 0;
        mid = mid + Math.imul(al1, bh3) | 0;
        mid = mid + Math.imul(ah1, bl3) | 0;
        hi = hi + Math.imul(ah1, bh3) | 0;
        lo = lo + Math.imul(al0, bl4) | 0;
        mid = mid + Math.imul(al0, bh4) | 0;
        mid = mid + Math.imul(ah0, bl4) | 0;
        hi = hi + Math.imul(ah0, bh4) | 0;
        var w4 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w4 >>> 26) | 0;
        w4 &= 67108863;
        lo = Math.imul(al5, bl0);
        mid = Math.imul(al5, bh0);
        mid = mid + Math.imul(ah5, bl0) | 0;
        hi = Math.imul(ah5, bh0);
        lo = lo + Math.imul(al4, bl1) | 0;
        mid = mid + Math.imul(al4, bh1) | 0;
        mid = mid + Math.imul(ah4, bl1) | 0;
        hi = hi + Math.imul(ah4, bh1) | 0;
        lo = lo + Math.imul(al3, bl2) | 0;
        mid = mid + Math.imul(al3, bh2) | 0;
        mid = mid + Math.imul(ah3, bl2) | 0;
        hi = hi + Math.imul(ah3, bh2) | 0;
        lo = lo + Math.imul(al2, bl3) | 0;
        mid = mid + Math.imul(al2, bh3) | 0;
        mid = mid + Math.imul(ah2, bl3) | 0;
        hi = hi + Math.imul(ah2, bh3) | 0;
        lo = lo + Math.imul(al1, bl4) | 0;
        mid = mid + Math.imul(al1, bh4) | 0;
        mid = mid + Math.imul(ah1, bl4) | 0;
        hi = hi + Math.imul(ah1, bh4) | 0;
        lo = lo + Math.imul(al0, bl5) | 0;
        mid = mid + Math.imul(al0, bh5) | 0;
        mid = mid + Math.imul(ah0, bl5) | 0;
        hi = hi + Math.imul(ah0, bh5) | 0;
        var w5 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w5 >>> 26) | 0;
        w5 &= 67108863;
        lo = Math.imul(al6, bl0);
        mid = Math.imul(al6, bh0);
        mid = mid + Math.imul(ah6, bl0) | 0;
        hi = Math.imul(ah6, bh0);
        lo = lo + Math.imul(al5, bl1) | 0;
        mid = mid + Math.imul(al5, bh1) | 0;
        mid = mid + Math.imul(ah5, bl1) | 0;
        hi = hi + Math.imul(ah5, bh1) | 0;
        lo = lo + Math.imul(al4, bl2) | 0;
        mid = mid + Math.imul(al4, bh2) | 0;
        mid = mid + Math.imul(ah4, bl2) | 0;
        hi = hi + Math.imul(ah4, bh2) | 0;
        lo = lo + Math.imul(al3, bl3) | 0;
        mid = mid + Math.imul(al3, bh3) | 0;
        mid = mid + Math.imul(ah3, bl3) | 0;
        hi = hi + Math.imul(ah3, bh3) | 0;
        lo = lo + Math.imul(al2, bl4) | 0;
        mid = mid + Math.imul(al2, bh4) | 0;
        mid = mid + Math.imul(ah2, bl4) | 0;
        hi = hi + Math.imul(ah2, bh4) | 0;
        lo = lo + Math.imul(al1, bl5) | 0;
        mid = mid + Math.imul(al1, bh5) | 0;
        mid = mid + Math.imul(ah1, bl5) | 0;
        hi = hi + Math.imul(ah1, bh5) | 0;
        lo = lo + Math.imul(al0, bl6) | 0;
        mid = mid + Math.imul(al0, bh6) | 0;
        mid = mid + Math.imul(ah0, bl6) | 0;
        hi = hi + Math.imul(ah0, bh6) | 0;
        var w6 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w6 >>> 26) | 0;
        w6 &= 67108863;
        lo = Math.imul(al7, bl0);
        mid = Math.imul(al7, bh0);
        mid = mid + Math.imul(ah7, bl0) | 0;
        hi = Math.imul(ah7, bh0);
        lo = lo + Math.imul(al6, bl1) | 0;
        mid = mid + Math.imul(al6, bh1) | 0;
        mid = mid + Math.imul(ah6, bl1) | 0;
        hi = hi + Math.imul(ah6, bh1) | 0;
        lo = lo + Math.imul(al5, bl2) | 0;
        mid = mid + Math.imul(al5, bh2) | 0;
        mid = mid + Math.imul(ah5, bl2) | 0;
        hi = hi + Math.imul(ah5, bh2) | 0;
        lo = lo + Math.imul(al4, bl3) | 0;
        mid = mid + Math.imul(al4, bh3) | 0;
        mid = mid + Math.imul(ah4, bl3) | 0;
        hi = hi + Math.imul(ah4, bh3) | 0;
        lo = lo + Math.imul(al3, bl4) | 0;
        mid = mid + Math.imul(al3, bh4) | 0;
        mid = mid + Math.imul(ah3, bl4) | 0;
        hi = hi + Math.imul(ah3, bh4) | 0;
        lo = lo + Math.imul(al2, bl5) | 0;
        mid = mid + Math.imul(al2, bh5) | 0;
        mid = mid + Math.imul(ah2, bl5) | 0;
        hi = hi + Math.imul(ah2, bh5) | 0;
        lo = lo + Math.imul(al1, bl6) | 0;
        mid = mid + Math.imul(al1, bh6) | 0;
        mid = mid + Math.imul(ah1, bl6) | 0;
        hi = hi + Math.imul(ah1, bh6) | 0;
        lo = lo + Math.imul(al0, bl7) | 0;
        mid = mid + Math.imul(al0, bh7) | 0;
        mid = mid + Math.imul(ah0, bl7) | 0;
        hi = hi + Math.imul(ah0, bh7) | 0;
        var w7 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w7 >>> 26) | 0;
        w7 &= 67108863;
        lo = Math.imul(al8, bl0);
        mid = Math.imul(al8, bh0);
        mid = mid + Math.imul(ah8, bl0) | 0;
        hi = Math.imul(ah8, bh0);
        lo = lo + Math.imul(al7, bl1) | 0;
        mid = mid + Math.imul(al7, bh1) | 0;
        mid = mid + Math.imul(ah7, bl1) | 0;
        hi = hi + Math.imul(ah7, bh1) | 0;
        lo = lo + Math.imul(al6, bl2) | 0;
        mid = mid + Math.imul(al6, bh2) | 0;
        mid = mid + Math.imul(ah6, bl2) | 0;
        hi = hi + Math.imul(ah6, bh2) | 0;
        lo = lo + Math.imul(al5, bl3) | 0;
        mid = mid + Math.imul(al5, bh3) | 0;
        mid = mid + Math.imul(ah5, bl3) | 0;
        hi = hi + Math.imul(ah5, bh3) | 0;
        lo = lo + Math.imul(al4, bl4) | 0;
        mid = mid + Math.imul(al4, bh4) | 0;
        mid = mid + Math.imul(ah4, bl4) | 0;
        hi = hi + Math.imul(ah4, bh4) | 0;
        lo = lo + Math.imul(al3, bl5) | 0;
        mid = mid + Math.imul(al3, bh5) | 0;
        mid = mid + Math.imul(ah3, bl5) | 0;
        hi = hi + Math.imul(ah3, bh5) | 0;
        lo = lo + Math.imul(al2, bl6) | 0;
        mid = mid + Math.imul(al2, bh6) | 0;
        mid = mid + Math.imul(ah2, bl6) | 0;
        hi = hi + Math.imul(ah2, bh6) | 0;
        lo = lo + Math.imul(al1, bl7) | 0;
        mid = mid + Math.imul(al1, bh7) | 0;
        mid = mid + Math.imul(ah1, bl7) | 0;
        hi = hi + Math.imul(ah1, bh7) | 0;
        lo = lo + Math.imul(al0, bl8) | 0;
        mid = mid + Math.imul(al0, bh8) | 0;
        mid = mid + Math.imul(ah0, bl8) | 0;
        hi = hi + Math.imul(ah0, bh8) | 0;
        var w8 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w8 >>> 26) | 0;
        w8 &= 67108863;
        lo = Math.imul(al9, bl0);
        mid = Math.imul(al9, bh0);
        mid = mid + Math.imul(ah9, bl0) | 0;
        hi = Math.imul(ah9, bh0);
        lo = lo + Math.imul(al8, bl1) | 0;
        mid = mid + Math.imul(al8, bh1) | 0;
        mid = mid + Math.imul(ah8, bl1) | 0;
        hi = hi + Math.imul(ah8, bh1) | 0;
        lo = lo + Math.imul(al7, bl2) | 0;
        mid = mid + Math.imul(al7, bh2) | 0;
        mid = mid + Math.imul(ah7, bl2) | 0;
        hi = hi + Math.imul(ah7, bh2) | 0;
        lo = lo + Math.imul(al6, bl3) | 0;
        mid = mid + Math.imul(al6, bh3) | 0;
        mid = mid + Math.imul(ah6, bl3) | 0;
        hi = hi + Math.imul(ah6, bh3) | 0;
        lo = lo + Math.imul(al5, bl4) | 0;
        mid = mid + Math.imul(al5, bh4) | 0;
        mid = mid + Math.imul(ah5, bl4) | 0;
        hi = hi + Math.imul(ah5, bh4) | 0;
        lo = lo + Math.imul(al4, bl5) | 0;
        mid = mid + Math.imul(al4, bh5) | 0;
        mid = mid + Math.imul(ah4, bl5) | 0;
        hi = hi + Math.imul(ah4, bh5) | 0;
        lo = lo + Math.imul(al3, bl6) | 0;
        mid = mid + Math.imul(al3, bh6) | 0;
        mid = mid + Math.imul(ah3, bl6) | 0;
        hi = hi + Math.imul(ah3, bh6) | 0;
        lo = lo + Math.imul(al2, bl7) | 0;
        mid = mid + Math.imul(al2, bh7) | 0;
        mid = mid + Math.imul(ah2, bl7) | 0;
        hi = hi + Math.imul(ah2, bh7) | 0;
        lo = lo + Math.imul(al1, bl8) | 0;
        mid = mid + Math.imul(al1, bh8) | 0;
        mid = mid + Math.imul(ah1, bl8) | 0;
        hi = hi + Math.imul(ah1, bh8) | 0;
        lo = lo + Math.imul(al0, bl9) | 0;
        mid = mid + Math.imul(al0, bh9) | 0;
        mid = mid + Math.imul(ah0, bl9) | 0;
        hi = hi + Math.imul(ah0, bh9) | 0;
        var w9 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w9 >>> 26) | 0;
        w9 &= 67108863;
        lo = Math.imul(al9, bl1);
        mid = Math.imul(al9, bh1);
        mid = mid + Math.imul(ah9, bl1) | 0;
        hi = Math.imul(ah9, bh1);
        lo = lo + Math.imul(al8, bl2) | 0;
        mid = mid + Math.imul(al8, bh2) | 0;
        mid = mid + Math.imul(ah8, bl2) | 0;
        hi = hi + Math.imul(ah8, bh2) | 0;
        lo = lo + Math.imul(al7, bl3) | 0;
        mid = mid + Math.imul(al7, bh3) | 0;
        mid = mid + Math.imul(ah7, bl3) | 0;
        hi = hi + Math.imul(ah7, bh3) | 0;
        lo = lo + Math.imul(al6, bl4) | 0;
        mid = mid + Math.imul(al6, bh4) | 0;
        mid = mid + Math.imul(ah6, bl4) | 0;
        hi = hi + Math.imul(ah6, bh4) | 0;
        lo = lo + Math.imul(al5, bl5) | 0;
        mid = mid + Math.imul(al5, bh5) | 0;
        mid = mid + Math.imul(ah5, bl5) | 0;
        hi = hi + Math.imul(ah5, bh5) | 0;
        lo = lo + Math.imul(al4, bl6) | 0;
        mid = mid + Math.imul(al4, bh6) | 0;
        mid = mid + Math.imul(ah4, bl6) | 0;
        hi = hi + Math.imul(ah4, bh6) | 0;
        lo = lo + Math.imul(al3, bl7) | 0;
        mid = mid + Math.imul(al3, bh7) | 0;
        mid = mid + Math.imul(ah3, bl7) | 0;
        hi = hi + Math.imul(ah3, bh7) | 0;
        lo = lo + Math.imul(al2, bl8) | 0;
        mid = mid + Math.imul(al2, bh8) | 0;
        mid = mid + Math.imul(ah2, bl8) | 0;
        hi = hi + Math.imul(ah2, bh8) | 0;
        lo = lo + Math.imul(al1, bl9) | 0;
        mid = mid + Math.imul(al1, bh9) | 0;
        mid = mid + Math.imul(ah1, bl9) | 0;
        hi = hi + Math.imul(ah1, bh9) | 0;
        var w10 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w10 >>> 26) | 0;
        w10 &= 67108863;
        lo = Math.imul(al9, bl2);
        mid = Math.imul(al9, bh2);
        mid = mid + Math.imul(ah9, bl2) | 0;
        hi = Math.imul(ah9, bh2);
        lo = lo + Math.imul(al8, bl3) | 0;
        mid = mid + Math.imul(al8, bh3) | 0;
        mid = mid + Math.imul(ah8, bl3) | 0;
        hi = hi + Math.imul(ah8, bh3) | 0;
        lo = lo + Math.imul(al7, bl4) | 0;
        mid = mid + Math.imul(al7, bh4) | 0;
        mid = mid + Math.imul(ah7, bl4) | 0;
        hi = hi + Math.imul(ah7, bh4) | 0;
        lo = lo + Math.imul(al6, bl5) | 0;
        mid = mid + Math.imul(al6, bh5) | 0;
        mid = mid + Math.imul(ah6, bl5) | 0;
        hi = hi + Math.imul(ah6, bh5) | 0;
        lo = lo + Math.imul(al5, bl6) | 0;
        mid = mid + Math.imul(al5, bh6) | 0;
        mid = mid + Math.imul(ah5, bl6) | 0;
        hi = hi + Math.imul(ah5, bh6) | 0;
        lo = lo + Math.imul(al4, bl7) | 0;
        mid = mid + Math.imul(al4, bh7) | 0;
        mid = mid + Math.imul(ah4, bl7) | 0;
        hi = hi + Math.imul(ah4, bh7) | 0;
        lo = lo + Math.imul(al3, bl8) | 0;
        mid = mid + Math.imul(al3, bh8) | 0;
        mid = mid + Math.imul(ah3, bl8) | 0;
        hi = hi + Math.imul(ah3, bh8) | 0;
        lo = lo + Math.imul(al2, bl9) | 0;
        mid = mid + Math.imul(al2, bh9) | 0;
        mid = mid + Math.imul(ah2, bl9) | 0;
        hi = hi + Math.imul(ah2, bh9) | 0;
        var w11 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w11 >>> 26) | 0;
        w11 &= 67108863;
        lo = Math.imul(al9, bl3);
        mid = Math.imul(al9, bh3);
        mid = mid + Math.imul(ah9, bl3) | 0;
        hi = Math.imul(ah9, bh3);
        lo = lo + Math.imul(al8, bl4) | 0;
        mid = mid + Math.imul(al8, bh4) | 0;
        mid = mid + Math.imul(ah8, bl4) | 0;
        hi = hi + Math.imul(ah8, bh4) | 0;
        lo = lo + Math.imul(al7, bl5) | 0;
        mid = mid + Math.imul(al7, bh5) | 0;
        mid = mid + Math.imul(ah7, bl5) | 0;
        hi = hi + Math.imul(ah7, bh5) | 0;
        lo = lo + Math.imul(al6, bl6) | 0;
        mid = mid + Math.imul(al6, bh6) | 0;
        mid = mid + Math.imul(ah6, bl6) | 0;
        hi = hi + Math.imul(ah6, bh6) | 0;
        lo = lo + Math.imul(al5, bl7) | 0;
        mid = mid + Math.imul(al5, bh7) | 0;
        mid = mid + Math.imul(ah5, bl7) | 0;
        hi = hi + Math.imul(ah5, bh7) | 0;
        lo = lo + Math.imul(al4, bl8) | 0;
        mid = mid + Math.imul(al4, bh8) | 0;
        mid = mid + Math.imul(ah4, bl8) | 0;
        hi = hi + Math.imul(ah4, bh8) | 0;
        lo = lo + Math.imul(al3, bl9) | 0;
        mid = mid + Math.imul(al3, bh9) | 0;
        mid = mid + Math.imul(ah3, bl9) | 0;
        hi = hi + Math.imul(ah3, bh9) | 0;
        var w12 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w12 >>> 26) | 0;
        w12 &= 67108863;
        lo = Math.imul(al9, bl4);
        mid = Math.imul(al9, bh4);
        mid = mid + Math.imul(ah9, bl4) | 0;
        hi = Math.imul(ah9, bh4);
        lo = lo + Math.imul(al8, bl5) | 0;
        mid = mid + Math.imul(al8, bh5) | 0;
        mid = mid + Math.imul(ah8, bl5) | 0;
        hi = hi + Math.imul(ah8, bh5) | 0;
        lo = lo + Math.imul(al7, bl6) | 0;
        mid = mid + Math.imul(al7, bh6) | 0;
        mid = mid + Math.imul(ah7, bl6) | 0;
        hi = hi + Math.imul(ah7, bh6) | 0;
        lo = lo + Math.imul(al6, bl7) | 0;
        mid = mid + Math.imul(al6, bh7) | 0;
        mid = mid + Math.imul(ah6, bl7) | 0;
        hi = hi + Math.imul(ah6, bh7) | 0;
        lo = lo + Math.imul(al5, bl8) | 0;
        mid = mid + Math.imul(al5, bh8) | 0;
        mid = mid + Math.imul(ah5, bl8) | 0;
        hi = hi + Math.imul(ah5, bh8) | 0;
        lo = lo + Math.imul(al4, bl9) | 0;
        mid = mid + Math.imul(al4, bh9) | 0;
        mid = mid + Math.imul(ah4, bl9) | 0;
        hi = hi + Math.imul(ah4, bh9) | 0;
        var w13 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w13 >>> 26) | 0;
        w13 &= 67108863;
        lo = Math.imul(al9, bl5);
        mid = Math.imul(al9, bh5);
        mid = mid + Math.imul(ah9, bl5) | 0;
        hi = Math.imul(ah9, bh5);
        lo = lo + Math.imul(al8, bl6) | 0;
        mid = mid + Math.imul(al8, bh6) | 0;
        mid = mid + Math.imul(ah8, bl6) | 0;
        hi = hi + Math.imul(ah8, bh6) | 0;
        lo = lo + Math.imul(al7, bl7) | 0;
        mid = mid + Math.imul(al7, bh7) | 0;
        mid = mid + Math.imul(ah7, bl7) | 0;
        hi = hi + Math.imul(ah7, bh7) | 0;
        lo = lo + Math.imul(al6, bl8) | 0;
        mid = mid + Math.imul(al6, bh8) | 0;
        mid = mid + Math.imul(ah6, bl8) | 0;
        hi = hi + Math.imul(ah6, bh8) | 0;
        lo = lo + Math.imul(al5, bl9) | 0;
        mid = mid + Math.imul(al5, bh9) | 0;
        mid = mid + Math.imul(ah5, bl9) | 0;
        hi = hi + Math.imul(ah5, bh9) | 0;
        var w14 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w14 >>> 26) | 0;
        w14 &= 67108863;
        lo = Math.imul(al9, bl6);
        mid = Math.imul(al9, bh6);
        mid = mid + Math.imul(ah9, bl6) | 0;
        hi = Math.imul(ah9, bh6);
        lo = lo + Math.imul(al8, bl7) | 0;
        mid = mid + Math.imul(al8, bh7) | 0;
        mid = mid + Math.imul(ah8, bl7) | 0;
        hi = hi + Math.imul(ah8, bh7) | 0;
        lo = lo + Math.imul(al7, bl8) | 0;
        mid = mid + Math.imul(al7, bh8) | 0;
        mid = mid + Math.imul(ah7, bl8) | 0;
        hi = hi + Math.imul(ah7, bh8) | 0;
        lo = lo + Math.imul(al6, bl9) | 0;
        mid = mid + Math.imul(al6, bh9) | 0;
        mid = mid + Math.imul(ah6, bl9) | 0;
        hi = hi + Math.imul(ah6, bh9) | 0;
        var w15 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w15 >>> 26) | 0;
        w15 &= 67108863;
        lo = Math.imul(al9, bl7);
        mid = Math.imul(al9, bh7);
        mid = mid + Math.imul(ah9, bl7) | 0;
        hi = Math.imul(ah9, bh7);
        lo = lo + Math.imul(al8, bl8) | 0;
        mid = mid + Math.imul(al8, bh8) | 0;
        mid = mid + Math.imul(ah8, bl8) | 0;
        hi = hi + Math.imul(ah8, bh8) | 0;
        lo = lo + Math.imul(al7, bl9) | 0;
        mid = mid + Math.imul(al7, bh9) | 0;
        mid = mid + Math.imul(ah7, bl9) | 0;
        hi = hi + Math.imul(ah7, bh9) | 0;
        var w16 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w16 >>> 26) | 0;
        w16 &= 67108863;
        lo = Math.imul(al9, bl8);
        mid = Math.imul(al9, bh8);
        mid = mid + Math.imul(ah9, bl8) | 0;
        hi = Math.imul(ah9, bh8);
        lo = lo + Math.imul(al8, bl9) | 0;
        mid = mid + Math.imul(al8, bh9) | 0;
        mid = mid + Math.imul(ah8, bl9) | 0;
        hi = hi + Math.imul(ah8, bh9) | 0;
        var w17 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w17 >>> 26) | 0;
        w17 &= 67108863;
        lo = Math.imul(al9, bl9);
        mid = Math.imul(al9, bh9);
        mid = mid + Math.imul(ah9, bl9) | 0;
        hi = Math.imul(ah9, bh9);
        var w18 = (c + lo | 0) + ((mid & 8191) << 13) | 0;
        c = (hi + (mid >>> 13) | 0) + (w18 >>> 26) | 0;
        w18 &= 67108863;
        o[0] = w0;
        o[1] = w1;
        o[2] = w2;
        o[3] = w3;
        o[4] = w4;
        o[5] = w5;
        o[6] = w6;
        o[7] = w7;
        o[8] = w8;
        o[9] = w9;
        o[10] = w10;
        o[11] = w11;
        o[12] = w12;
        o[13] = w13;
        o[14] = w14;
        o[15] = w15;
        o[16] = w16;
        o[17] = w17;
        o[18] = w18;
        if (c !== 0) {
          o[19] = c;
          out.length++;
        }
        return out;
      };
      $(comb10MulTo);
      if (!Math.imul) {
        comb10MulTo = smallMulTo;
      }
      function bigMulTo(self, num, out) {
        out.negative = num.negative ^ self.negative;
        out.length = self.length + num.length;
        var carry = 0;
        var hncarry = 0;
        for (var k = 0; k < out.length - 1; k++) {
          var ncarry = hncarry;
          hncarry = 0;
          var rword = carry & 67108863;
          var maxJ = Math.min(k, num.length - 1);
          for (var j = Math.max(0, k - self.length + 1); j <= maxJ; j++) {
            var i = k - j;
            var a = self.words[i] | 0;
            var b = num.words[j] | 0;
            var r = a * b;
            var lo = r & 67108863;
            ncarry = ncarry + (r / 67108864 | 0) | 0;
            lo = lo + rword | 0;
            rword = lo & 67108863;
            ncarry = ncarry + (lo >>> 26) | 0;
            hncarry += ncarry >>> 26;
            ncarry &= 67108863;
          }
          out.words[k] = rword;
          carry = ncarry;
          ncarry = hncarry;
        }
        if (carry !== 0) {
          out.words[k] = carry;
        } else {
          out.length--;
        }
        return out._strip();
      }
      $(bigMulTo);
      function jumboMulTo(self, num, out) {
        return bigMulTo(self, num, out);
      }
      $(jumboMulTo);
      BN.prototype.mulTo = $(function mulTo(num, out) {
        var res;
        var len = this.length + num.length;
        if (this.length === 10 && num.length === 10) {
          res = comb10MulTo(this, num, out);
        } else if (len < 63) {
          res = smallMulTo(this, num, out);
        } else if (len < 1024) {
          res = bigMulTo(this, num, out);
        } else {
          res = jumboMulTo(this, num, out);
        }
        return res;
      });
      BN.prototype.mul = $(function mul(num) {
        var out = new BN(null);
        out.words = new Array(this.length + num.length);
        return this.mulTo(num, out);
      });
      BN.prototype.mulf = $(function mulf(num) {
        var out = new BN(null);
        out.words = new Array(this.length + num.length);
        return jumboMulTo(this, num, out);
      });
      BN.prototype.imul = $(function imul(num) {
        return this.clone().mulTo(num, this);
      });
      BN.prototype.imuln = $(function imuln(num) {
        var isNegNum = num < 0;
        if (isNegNum) num = -num;
        assert(typeof num === "number");
        assert(num < 67108864);
        var carry = 0;
        for (var i = 0; i < this.length; i++) {
          var w = (this.words[i] | 0) * num;
          var lo = (w & 67108863) + (carry & 67108863);
          carry >>= 26;
          carry += w / 67108864 | 0;
          carry += lo >>> 26;
          this.words[i] = lo & 67108863;
        }
        if (carry !== 0) {
          this.words[i] = carry;
          this.length++;
        }
        return isNegNum ? this.ineg() : this;
      });
      BN.prototype.muln = $(function muln(num) {
        return this.clone().imuln(num);
      });
      BN.prototype.sqr = $(function sqr() {
        return this.mul(this);
      });
      BN.prototype.isqr = $(function isqr() {
        return this.imul(this.clone());
      });
      BN.prototype.pow = $(function pow(num) {
        var w = toBitArray(num);
        if (w.length === 0) return new BN(1);
        var res = this;
        for (var i = 0; i < w.length; i++, res = res.sqr()) {
          if (w[i] !== 0) break;
        }
        if (++i < w.length) {
          for (var q = res.sqr(); i < w.length; i++, q = q.sqr()) {
            if (w[i] === 0) continue;
            res = res.mul(q);
          }
        }
        return res;
      });
      BN.prototype.iushln = $(function iushln(bits) {
        assert(typeof bits === "number" && bits >= 0);
        var r = bits % 26;
        var s = (bits - r) / 26;
        var carryMask = 67108863 >>> 26 - r << 26 - r;
        var i;
        if (r !== 0) {
          var carry = 0;
          for (i = 0; i < this.length; i++) {
            var newCarry = this.words[i] & carryMask;
            var c = (this.words[i] | 0) - newCarry << r;
            this.words[i] = c | carry;
            carry = newCarry >>> 26 - r;
          }
          if (carry) {
            this.words[i] = carry;
            this.length++;
          }
        }
        if (s !== 0) {
          for (i = this.length - 1; i >= 0; i--) {
            this.words[i + s] = this.words[i];
          }
          for (i = 0; i < s; i++) {
            this.words[i] = 0;
          }
          this.length += s;
        }
        return this._strip();
      });
      BN.prototype.ishln = $(function ishln(bits) {
        assert(this.negative === 0);
        return this.iushln(bits);
      });
      BN.prototype.iushrn = $(function iushrn(bits, hint, extended) {
        assert(typeof bits === "number" && bits >= 0);
        var h;
        if (hint) {
          h = (hint - hint % 26) / 26;
        } else {
          h = 0;
        }
        var r = bits % 26;
        var s = Math.min((bits - r) / 26, this.length);
        var mask = 67108863 ^ 67108863 >>> r << r;
        var maskedWords = extended;
        h -= s;
        h = Math.max(0, h);
        if (maskedWords) {
          for (var i = 0; i < s; i++) {
            maskedWords.words[i] = this.words[i];
          }
          maskedWords.length = s;
        }
        if (s === 0) ;else if (this.length > s) {
          this.length -= s;
          for (i = 0; i < this.length; i++) {
            this.words[i] = this.words[i + s];
          }
        } else {
          this.words[0] = 0;
          this.length = 1;
        }
        var carry = 0;
        for (i = this.length - 1; i >= 0 && (carry !== 0 || i >= h); i--) {
          var word = this.words[i] | 0;
          this.words[i] = carry << 26 - r | word >>> r;
          carry = word & mask;
        }
        if (maskedWords && carry !== 0) {
          maskedWords.words[maskedWords.length++] = carry;
        }
        if (this.length === 0) {
          this.words[0] = 0;
          this.length = 1;
        }
        return this._strip();
      });
      BN.prototype.ishrn = $(function ishrn(bits, hint, extended) {
        assert(this.negative === 0);
        return this.iushrn(bits, hint, extended);
      });
      BN.prototype.shln = $(function shln(bits) {
        return this.clone().ishln(bits);
      });
      BN.prototype.ushln = $(function ushln(bits) {
        return this.clone().iushln(bits);
      });
      BN.prototype.shrn = $(function shrn(bits) {
        return this.clone().ishrn(bits);
      });
      BN.prototype.ushrn = $(function ushrn(bits) {
        return this.clone().iushrn(bits);
      });
      BN.prototype.testn = $(function testn(bit) {
        assert(typeof bit === "number" && bit >= 0);
        var r = bit % 26;
        var s = (bit - r) / 26;
        var q = 1 << r;
        if (this.length <= s) return false;
        var w = this.words[s];
        return !!(w & q);
      });
      BN.prototype.imaskn = $(function imaskn(bits) {
        assert(typeof bits === "number" && bits >= 0);
        var r = bits % 26;
        var s = (bits - r) / 26;
        assert(this.negative === 0, "imaskn works only with positive numbers");
        if (this.length <= s) {
          return this;
        }
        if (r !== 0) {
          s++;
        }
        this.length = Math.min(s, this.length);
        if (r !== 0) {
          var mask = 67108863 ^ 67108863 >>> r << r;
          this.words[this.length - 1] &= mask;
        }
        return this._strip();
      });
      BN.prototype.maskn = $(function maskn(bits) {
        return this.clone().imaskn(bits);
      });
      BN.prototype.iaddn = $(function iaddn(num) {
        assert(typeof num === "number");
        assert(num < 67108864);
        if (num < 0) return this.isubn(-num);
        if (this.negative !== 0) {
          if (this.length === 1 && (this.words[0] | 0) <= num) {
            this.words[0] = num - (this.words[0] | 0);
            this.negative = 0;
            return this;
          }
          this.negative = 0;
          this.isubn(num);
          this.negative = 1;
          return this;
        }
        return this._iaddn(num);
      });
      BN.prototype._iaddn = $(function _iaddn(num) {
        this.words[0] += num;
        for (var i = 0; i < this.length && this.words[i] >= 67108864; i++) {
          this.words[i] -= 67108864;
          if (i === this.length - 1) {
            this.words[i + 1] = 1;
          } else {
            this.words[i + 1]++;
          }
        }
        this.length = Math.max(this.length, i + 1);
        return this;
      });
      BN.prototype.isubn = $(function isubn(num) {
        assert(typeof num === "number");
        assert(num < 67108864);
        if (num < 0) return this.iaddn(-num);
        if (this.negative !== 0) {
          this.negative = 0;
          this.iaddn(num);
          this.negative = 1;
          return this;
        }
        this.words[0] -= num;
        if (this.length === 1 && this.words[0] < 0) {
          this.words[0] = -this.words[0];
          this.negative = 1;
        } else {
          for (var i = 0; i < this.length && this.words[i] < 0; i++) {
            this.words[i] += 67108864;
            this.words[i + 1] -= 1;
          }
        }
        return this._strip();
      });
      BN.prototype.addn = $(function addn(num) {
        return this.clone().iaddn(num);
      });
      BN.prototype.subn = $(function subn(num) {
        return this.clone().isubn(num);
      });
      BN.prototype.iabs = $(function iabs() {
        this.negative = 0;
        return this;
      });
      BN.prototype.abs = $(function abs() {
        return this.clone().iabs();
      });
      BN.prototype._ishlnsubmul = $(function _ishlnsubmul(num, mul, shift) {
        var len = num.length + shift;
        var i;
        this._expand(len);
        var w;
        var carry = 0;
        for (i = 0; i < num.length; i++) {
          w = (this.words[i + shift] | 0) + carry;
          var right = (num.words[i] | 0) * mul;
          w -= right & 67108863;
          carry = (w >> 26) - (right / 67108864 | 0);
          this.words[i + shift] = w & 67108863;
        }
        for (; i < this.length - shift; i++) {
          w = (this.words[i + shift] | 0) + carry;
          carry = w >> 26;
          this.words[i + shift] = w & 67108863;
        }
        if (carry === 0) return this._strip();
        assert(carry === -1);
        carry = 0;
        for (i = 0; i < this.length; i++) {
          w = -(this.words[i] | 0) + carry;
          carry = w >> 26;
          this.words[i] = w & 67108863;
        }
        this.negative = 1;
        return this._strip();
      });
      BN.prototype._wordDiv = $(function _wordDiv(num, mode) {
        var shift = this.length - num.length;
        var a = this.clone();
        var b = num;
        var bhi = b.words[b.length - 1] | 0;
        var bhiBits = this._countBits(bhi);
        shift = 26 - bhiBits;
        if (shift !== 0) {
          b = b.ushln(shift);
          a.iushln(shift);
          bhi = b.words[b.length - 1] | 0;
        }
        var m = a.length - b.length;
        var q;
        if (mode !== "mod") {
          q = new BN(null);
          q.length = m + 1;
          q.words = new Array(q.length);
          for (var i = 0; i < q.length; i++) {
            q.words[i] = 0;
          }
        }
        var diff = a.clone()._ishlnsubmul(b, 1, m);
        if (diff.negative === 0) {
          a = diff;
          if (q) {
            q.words[m] = 1;
          }
        }
        for (var j = m - 1; j >= 0; j--) {
          var qj = (a.words[b.length + j] | 0) * 67108864 + (a.words[b.length + j - 1] | 0);
          qj = Math.min(qj / bhi | 0, 67108863);
          a._ishlnsubmul(b, qj, j);
          while (a.negative !== 0) {
            qj--;
            a.negative = 0;
            a._ishlnsubmul(b, 1, j);
            if (!a.isZero()) {
              a.negative ^= 1;
            }
          }
          if (q) {
            q.words[j] = qj;
          }
        }
        if (q) {
          q._strip();
        }
        a._strip();
        if (mode !== "div" && shift !== 0) {
          a.iushrn(shift);
        }
        return $(function () {
          let result = $Object.create(null, undefined);
          result.div = q || null;
          result.mod = a;
          return result;
        })();
      });
      BN.prototype.divmod = $(function divmod(num, mode, positive) {
        assert(!num.isZero());
        if (this.isZero()) {
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = new BN(0);
            result.mod = new BN(0);
            return result;
          })();
        }
        var div, mod, res;
        if (this.negative !== 0 && num.negative === 0) {
          res = this.neg().divmod(num, mode);
          if (mode !== "mod") {
            div = res.div.neg();
          }
          if (mode !== "div") {
            mod = res.mod.neg();
            if (positive && mod.negative !== 0) {
              mod.iadd(num);
            }
          }
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = div;
            result.mod = mod;
            return result;
          })();
        }
        if (this.negative === 0 && num.negative !== 0) {
          res = this.divmod(num.neg(), mode);
          if (mode !== "mod") {
            div = res.div.neg();
          }
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = div;
            result.mod = res.mod;
            return result;
          })();
        }
        if ((this.negative & num.negative) !== 0) {
          res = this.neg().divmod(num.neg(), mode);
          if (mode !== "div") {
            mod = res.mod.neg();
            if (positive && mod.negative !== 0) {
              mod.isub(num);
            }
          }
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = res.div;
            result.mod = mod;
            return result;
          })();
        }
        if (num.length > this.length || this.cmp(num) < 0) {
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = new BN(0);
            result.mod = this;
            return result;
          })();
        }
        if (num.length === 1) {
          if (mode === "div") {
            return $(function () {
              let result = $Object.create(null, undefined);
              result.div = this.divn(num.words[0]);
              result.mod = null;
              return result;
            })();
          }
          if (mode === "mod") {
            return $(function () {
              let result = $Object.create(null, undefined);
              result.div = null;
              result.mod = new BN(this.modrn(num.words[0]));
              return result;
            })();
          }
          return $(function () {
            let result = $Object.create(null, undefined);
            result.div = this.divn(num.words[0]);
            result.mod = new BN(this.modrn(num.words[0]));
            return result;
          })();
        }
        return this._wordDiv(num, mode);
      });
      BN.prototype.div = $(function div(num) {
        return this.divmod(num, "div", false).div;
      });
      BN.prototype.mod = $(function mod(num) {
        return this.divmod(num, "mod", false).mod;
      });
      BN.prototype.umod = $(function umod(num) {
        return this.divmod(num, "mod", true).mod;
      });
      BN.prototype.divRound = $(function divRound(num) {
        var dm = this.divmod(num);
        if (dm.mod.isZero()) return dm.div;
        var mod = dm.div.negative !== 0 ? dm.mod.isub(num) : dm.mod;
        var half = num.ushrn(1);
        var r2 = num.andln(1);
        var cmp = mod.cmp(half);
        if (cmp < 0 || r2 === 1 && cmp === 0) return dm.div;
        return dm.div.negative !== 0 ? dm.div.isubn(1) : dm.div.iaddn(1);
      });
      BN.prototype.modrn = $(function modrn(num) {
        var isNegNum = num < 0;
        if (isNegNum) num = -num;
        assert(num <= 67108863);
        var p = (1 << 26) % num;
        var acc = 0;
        for (var i = this.length - 1; i >= 0; i--) {
          acc = (p * acc + (this.words[i] | 0)) % num;
        }
        return isNegNum ? -acc : acc;
      });
      BN.prototype.modn = $(function modn(num) {
        return this.modrn(num);
      });
      BN.prototype.idivn = $(function idivn(num) {
        var isNegNum = num < 0;
        if (isNegNum) num = -num;
        assert(num <= 67108863);
        var carry = 0;
        for (var i = this.length - 1; i >= 0; i--) {
          var w = (this.words[i] | 0) + carry * 67108864;
          this.words[i] = w / num | 0;
          carry = w % num;
        }
        this._strip();
        return isNegNum ? this.ineg() : this;
      });
      BN.prototype.divn = $(function divn(num) {
        return this.clone().idivn(num);
      });
      BN.prototype.egcd = $(function egcd(p) {
        assert(p.negative === 0);
        assert(!p.isZero());
        var x = this;
        var y = p.clone();
        if (x.negative !== 0) {
          x = x.umod(p);
        } else {
          x = x.clone();
        }
        var A = new BN(1);
        var B = new BN(0);
        var C = new BN(0);
        var D = new BN(1);
        var g = 0;
        while (x.isEven() && y.isEven()) {
          x.iushrn(1);
          y.iushrn(1);
          ++g;
        }
        var yp = y.clone();
        var xp = x.clone();
        while (!x.isZero()) {
          for (var i = 0, im = 1; (x.words[0] & im) === 0 && i < 26; ++i, im <<= 1);
          if (i > 0) {
            x.iushrn(i);
            while (i-- > 0) {
              if (A.isOdd() || B.isOdd()) {
                A.iadd(yp);
                B.isub(xp);
              }
              A.iushrn(1);
              B.iushrn(1);
            }
          }
          for (var j = 0, jm = 1; (y.words[0] & jm) === 0 && j < 26; ++j, jm <<= 1);
          if (j > 0) {
            y.iushrn(j);
            while (j-- > 0) {
              if (C.isOdd() || D.isOdd()) {
                C.iadd(yp);
                D.isub(xp);
              }
              C.iushrn(1);
              D.iushrn(1);
            }
          }
          if (x.cmp(y) >= 0) {
            x.isub(y);
            A.isub(C);
            B.isub(D);
          } else {
            y.isub(x);
            C.isub(A);
            D.isub(B);
          }
        }
        return $(function () {
          let result = $Object.create(null, undefined);
          result.a = C;
          result.b = D;
          result.gcd = y.iushln(g);
          return result;
        })();
      });
      BN.prototype._invmp = $(function _invmp(p) {
        assert(p.negative === 0);
        assert(!p.isZero());
        var a = this;
        var b = p.clone();
        if (a.negative !== 0) {
          a = a.umod(p);
        } else {
          a = a.clone();
        }
        var x1 = new BN(1);
        var x2 = new BN(0);
        var delta = b.clone();
        while (a.cmpn(1) > 0 && b.cmpn(1) > 0) {
          for (var i = 0, im = 1; (a.words[0] & im) === 0 && i < 26; ++i, im <<= 1);
          if (i > 0) {
            a.iushrn(i);
            while (i-- > 0) {
              if (x1.isOdd()) {
                x1.iadd(delta);
              }
              x1.iushrn(1);
            }
          }
          for (var j = 0, jm = 1; (b.words[0] & jm) === 0 && j < 26; ++j, jm <<= 1);
          if (j > 0) {
            b.iushrn(j);
            while (j-- > 0) {
              if (x2.isOdd()) {
                x2.iadd(delta);
              }
              x2.iushrn(1);
            }
          }
          if (a.cmp(b) >= 0) {
            a.isub(b);
            x1.isub(x2);
          } else {
            b.isub(a);
            x2.isub(x1);
          }
        }
        var res;
        if (a.cmpn(1) === 0) {
          res = x1;
        } else {
          res = x2;
        }
        if (res.cmpn(0) < 0) {
          res.iadd(p);
        }
        return res;
      });
      BN.prototype.gcd = $(function gcd(num) {
        if (this.isZero()) return num.abs();
        if (num.isZero()) return this.abs();
        var a = this.clone();
        var b = num.clone();
        a.negative = 0;
        b.negative = 0;
        for (var shift = 0; a.isEven() && b.isEven(); shift++) {
          a.iushrn(1);
          b.iushrn(1);
        }
        do {
          while (a.isEven()) {
            a.iushrn(1);
          }
          while (b.isEven()) {
            b.iushrn(1);
          }
          var r = a.cmp(b);
          if (r < 0) {
            var t = a;
            a = b;
            b = t;
          } else if (r === 0 || b.cmpn(1) === 0) {
            break;
          }
          a.isub(b);
        } while (true);
        return b.iushln(shift);
      });
      BN.prototype.invm = $(function invm(num) {
        return this.egcd(num).a.umod(num);
      });
      BN.prototype.isEven = $(function isEven() {
        return (this.words[0] & 1) === 0;
      });
      BN.prototype.isOdd = $(function isOdd() {
        return (this.words[0] & 1) === 1;
      });
      BN.prototype.andln = $(function andln(num) {
        return this.words[0] & num;
      });
      BN.prototype.bincn = $(function bincn(bit) {
        assert(typeof bit === "number");
        var r = bit % 26;
        var s = (bit - r) / 26;
        var q = 1 << r;
        if (this.length <= s) {
          this._expand(s + 1);
          this.words[s] |= q;
          return this;
        }
        var carry = q;
        for (var i = s; carry !== 0 && i < this.length; i++) {
          var w = this.words[i] | 0;
          w += carry;
          carry = w >>> 26;
          w &= 67108863;
          this.words[i] = w;
        }
        if (carry !== 0) {
          this.words[i] = carry;
          this.length++;
        }
        return this;
      });
      BN.prototype.isZero = $(function isZero() {
        return this.length === 1 && this.words[0] === 0;
      });
      BN.prototype.cmpn = $(function cmpn(num) {
        var negative = num < 0;
        if (this.negative !== 0 && !negative) return -1;
        if (this.negative === 0 && negative) return 1;
        this._strip();
        var res;
        if (this.length > 1) {
          res = 1;
        } else {
          if (negative) {
            num = -num;
          }
          assert(num <= 67108863, "Number is too big");
          var w = this.words[0] | 0;
          res = w === num ? 0 : w < num ? -1 : 1;
        }
        if (this.negative !== 0) return -res | 0;
        return res;
      });
      BN.prototype.cmp = $(function cmp(num) {
        if (this.negative !== 0 && num.negative === 0) return -1;
        if (this.negative === 0 && num.negative !== 0) return 1;
        var res = this.ucmp(num);
        if (this.negative !== 0) return -res | 0;
        return res;
      });
      BN.prototype.ucmp = $(function ucmp(num) {
        if (this.length > num.length) return 1;
        if (this.length < num.length) return -1;
        var res = 0;
        for (var i = this.length - 1; i >= 0; i--) {
          var a = this.words[i] | 0;
          var b = num.words[i] | 0;
          if (a === b) continue;
          if (a < b) {
            res = -1;
          } else if (a > b) {
            res = 1;
          }
          break;
        }
        return res;
      });
      BN.prototype.gtn = $(function gtn(num) {
        return this.cmpn(num) === 1;
      });
      BN.prototype.gt = $(function gt(num) {
        return this.cmp(num) === 1;
      });
      BN.prototype.gten = $(function gten(num) {
        return this.cmpn(num) >= 0;
      });
      BN.prototype.gte = $(function gte(num) {
        return this.cmp(num) >= 0;
      });
      BN.prototype.ltn = $(function ltn(num) {
        return this.cmpn(num) === -1;
      });
      BN.prototype.lt = $(function lt(num) {
        return this.cmp(num) === -1;
      });
      BN.prototype.lten = $(function lten(num) {
        return this.cmpn(num) <= 0;
      });
      BN.prototype.lte = $(function lte(num) {
        return this.cmp(num) <= 0;
      });
      BN.prototype.eqn = $(function eqn(num) {
        return this.cmpn(num) === 0;
      });
      BN.prototype.eq = $(function eq(num) {
        return this.cmp(num) === 0;
      });
      BN.red = $(function red(num) {
        return new Red(num);
      });
      BN.prototype.toRed = $(function toRed(ctx) {
        assert(!this.red, "Already a number in reduction context");
        assert(this.negative === 0, "red works only with positives");
        return ctx.convertTo(this)._forceRed(ctx);
      });
      BN.prototype.fromRed = $(function fromRed() {
        assert(this.red, "fromRed works only with numbers in reduction context");
        return this.red.convertFrom(this);
      });
      BN.prototype._forceRed = $(function _forceRed(ctx) {
        this.red = ctx;
        return this;
      });
      BN.prototype.forceRed = $(function forceRed(ctx) {
        assert(!this.red, "Already a number in reduction context");
        return this._forceRed(ctx);
      });
      BN.prototype.redAdd = $(function redAdd(num) {
        assert(this.red, "redAdd works only with red numbers");
        return this.red.add(this, num);
      });
      BN.prototype.redIAdd = $(function redIAdd(num) {
        assert(this.red, "redIAdd works only with red numbers");
        return this.red.iadd(this, num);
      });
      BN.prototype.redSub = $(function redSub(num) {
        assert(this.red, "redSub works only with red numbers");
        return this.red.sub(this, num);
      });
      BN.prototype.redISub = $(function redISub(num) {
        assert(this.red, "redISub works only with red numbers");
        return this.red.isub(this, num);
      });
      BN.prototype.redShl = $(function redShl(num) {
        assert(this.red, "redShl works only with red numbers");
        return this.red.shl(this, num);
      });
      BN.prototype.redMul = $(function redMul(num) {
        assert(this.red, "redMul works only with red numbers");
        this.red._verify2(this, num);
        return this.red.mul(this, num);
      });
      BN.prototype.redIMul = $(function redIMul(num) {
        assert(this.red, "redMul works only with red numbers");
        this.red._verify2(this, num);
        return this.red.imul(this, num);
      });
      BN.prototype.redSqr = $(function redSqr() {
        assert(this.red, "redSqr works only with red numbers");
        this.red._verify1(this);
        return this.red.sqr(this);
      });
      BN.prototype.redISqr = $(function redISqr() {
        assert(this.red, "redISqr works only with red numbers");
        this.red._verify1(this);
        return this.red.isqr(this);
      });
      BN.prototype.redSqrt = $(function redSqrt() {
        assert(this.red, "redSqrt works only with red numbers");
        this.red._verify1(this);
        return this.red.sqrt(this);
      });
      BN.prototype.redInvm = $(function redInvm() {
        assert(this.red, "redInvm works only with red numbers");
        this.red._verify1(this);
        return this.red.invm(this);
      });
      BN.prototype.redNeg = $(function redNeg() {
        assert(this.red, "redNeg works only with red numbers");
        this.red._verify1(this);
        return this.red.neg(this);
      });
      BN.prototype.redPow = $(function redPow(num) {
        assert(this.red && !num.red, "redPow(normalNum)");
        this.red._verify1(this);
        return this.red.pow(this, num);
      });
      var primes = $(function () {
        let result = $Object.create(null, undefined);
        result.k256 = null;
        result.p224 = null;
        result.p192 = null;
        result.p25519 = null;
        return result;
      })();
      function MPrime(name, p) {
        this.name = name;
        this.p = new BN(p, 16);
        this.n = this.p.bitLength();
        this.k = new BN(1).iushln(this.n).isub(this.p);
        this.tmp = this._tmp();
      }
      $(MPrime);
      MPrime.prototype._tmp = $(function _tmp() {
        var tmp = new BN(null);
        tmp.words = new Array(Math.ceil(this.n / 13));
        return tmp;
      });
      MPrime.prototype.ireduce = $(function ireduce(num) {
        var r = num;
        var rlen;
        do {
          this.split(r, this.tmp);
          r = this.imulK(r);
          r = r.iadd(this.tmp);
          rlen = r.bitLength();
        } while (rlen > this.n);
        var cmp = rlen < this.n ? -1 : r.ucmp(this.p);
        if (cmp === 0) {
          r.words[0] = 0;
          r.length = 1;
        } else if (cmp > 0) {
          r.isub(this.p);
        } else {
          if (r.strip !== undefined) {
            r.strip();
          } else {
            r._strip();
          }
        }
        return r;
      });
      MPrime.prototype.split = $(function split(input, out) {
        input.iushrn(this.n, 0, out);
      });
      MPrime.prototype.imulK = $(function imulK(num) {
        return num.imul(this.k);
      });
      function K256() {
        MPrime.call(this, "k256", "ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff fffffffe fffffc2f");
      }
      $(K256);
      inherits(K256, MPrime);
      K256.prototype.split = $(function split(input, output) {
        var mask = 4194303;
        var outLen = Math.min(input.length, 9);
        for (var i = 0; i < outLen; i++) {
          output.words[i] = input.words[i];
        }
        output.length = outLen;
        if (input.length <= 9) {
          input.words[0] = 0;
          input.length = 1;
          return;
        }
        var prev = input.words[9];
        output.words[output.length++] = prev & mask;
        for (i = 10; i < input.length; i++) {
          var next = input.words[i] | 0;
          input.words[i - 10] = (next & mask) << 4 | prev >>> 22;
          prev = next;
        }
        prev >>>= 22;
        input.words[i - 10] = prev;
        if (prev === 0 && input.length > 10) {
          input.length -= 10;
        } else {
          input.length -= 9;
        }
      });
      K256.prototype.imulK = $(function imulK(num) {
        num.words[num.length] = 0;
        num.words[num.length + 1] = 0;
        num.length += 2;
        var lo = 0;
        for (var i = 0; i < num.length; i++) {
          var w = num.words[i] | 0;
          lo += w * 977;
          num.words[i] = lo & 67108863;
          lo = w * 64 + (lo / 67108864 | 0);
        }
        if (num.words[num.length - 1] === 0) {
          num.length--;
          if (num.words[num.length - 1] === 0) {
            num.length--;
          }
        }
        return num;
      });
      function P224() {
        MPrime.call(this, "p224", "ffffffff ffffffff ffffffff ffffffff 00000000 00000000 00000001");
      }
      $(P224);
      inherits(P224, MPrime);
      function P192() {
        MPrime.call(this, "p192", "ffffffff ffffffff ffffffff fffffffe ffffffff ffffffff");
      }
      $(P192);
      inherits(P192, MPrime);
      function P25519() {
        MPrime.call(this, "25519", "7fffffffffffffff ffffffffffffffff ffffffffffffffff ffffffffffffffed");
      }
      $(P25519);
      inherits(P25519, MPrime);
      P25519.prototype.imulK = $(function imulK(num) {
        var carry = 0;
        for (var i = 0; i < num.length; i++) {
          var hi = (num.words[i] | 0) * 19 + carry;
          var lo = hi & 67108863;
          hi >>>= 26;
          num.words[i] = lo;
          carry = hi;
        }
        if (carry !== 0) {
          num.words[num.length++] = carry;
        }
        return num;
      });
      BN._prime = $(function prime(name) {
        if (primes[name]) return primes[name];
        var prime;
        if (name === "k256") {
          prime = new K256();
        } else if (name === "p224") {
          prime = new P224();
        } else if (name === "p192") {
          prime = new P192();
        } else if (name === "p25519") {
          prime = new P25519();
        } else {
          throw new Error("Unknown prime " + name);
        }
        primes[name] = prime;
        return prime;
      });
      function Red(m) {
        if (typeof m === "string") {
          var prime = BN._prime(m);
          this.m = prime.p;
          this.prime = prime;
        } else {
          assert(m.gtn(1), "modulus must be greater than 1");
          this.m = m;
          this.prime = null;
        }
      }
      $(Red);
      Red.prototype._verify1 = $(function _verify1(a) {
        assert(a.negative === 0, "red works only with positives");
        assert(a.red, "red works only with red numbers");
      });
      Red.prototype._verify2 = $(function _verify2(a, b) {
        assert((a.negative | b.negative) === 0, "red works only with positives");
        assert(a.red && a.red === b.red, "red works only with red numbers");
      });
      Red.prototype.imod = $(function imod(a) {
        if (this.prime) return this.prime.ireduce(a)._forceRed(this);
        move(a, a.umod(this.m)._forceRed(this));
        return a;
      });
      Red.prototype.neg = $(function neg(a) {
        if (a.isZero()) {
          return a.clone();
        }
        return this.m.sub(a)._forceRed(this);
      });
      Red.prototype.add = $(function add(a, b) {
        this._verify2(a, b);
        var res = a.add(b);
        if (res.cmp(this.m) >= 0) {
          res.isub(this.m);
        }
        return res._forceRed(this);
      });
      Red.prototype.iadd = $(function iadd(a, b) {
        this._verify2(a, b);
        var res = a.iadd(b);
        if (res.cmp(this.m) >= 0) {
          res.isub(this.m);
        }
        return res;
      });
      Red.prototype.sub = $(function sub(a, b) {
        this._verify2(a, b);
        var res = a.sub(b);
        if (res.cmpn(0) < 0) {
          res.iadd(this.m);
        }
        return res._forceRed(this);
      });
      Red.prototype.isub = $(function isub(a, b) {
        this._verify2(a, b);
        var res = a.isub(b);
        if (res.cmpn(0) < 0) {
          res.iadd(this.m);
        }
        return res;
      });
      Red.prototype.shl = $(function shl(a, num) {
        this._verify1(a);
        return this.imod(a.ushln(num));
      });
      Red.prototype.imul = $(function imul(a, b) {
        this._verify2(a, b);
        return this.imod(a.imul(b));
      });
      Red.prototype.mul = $(function mul(a, b) {
        this._verify2(a, b);
        return this.imod(a.mul(b));
      });
      Red.prototype.isqr = $(function isqr(a) {
        return this.imul(a, a.clone());
      });
      Red.prototype.sqr = $(function sqr(a) {
        return this.mul(a, a);
      });
      Red.prototype.sqrt = $(function sqrt(a) {
        if (a.isZero()) return a.clone();
        var mod3 = this.m.andln(3);
        assert(mod3 % 2 === 1);
        if (mod3 === 3) {
          var pow = this.m.add(new BN(1)).iushrn(2);
          return this.pow(a, pow);
        }
        var q = this.m.subn(1);
        var s = 0;
        while (!q.isZero() && q.andln(1) === 0) {
          s++;
          q.iushrn(1);
        }
        assert(!q.isZero());
        var one = new BN(1).toRed(this);
        var nOne = one.redNeg();
        var lpow = this.m.subn(1).iushrn(1);
        var z = this.m.bitLength();
        z = new BN(2 * z * z).toRed(this);
        while (this.pow(z, lpow).cmp(nOne) !== 0) {
          z.redIAdd(nOne);
        }
        var c = this.pow(z, q);
        var r = this.pow(a, q.addn(1).iushrn(1));
        var t = this.pow(a, q);
        var m = s;
        while (t.cmp(one) !== 0) {
          var tmp = t;
          for (var i = 0; tmp.cmp(one) !== 0; i++) {
            tmp = tmp.redSqr();
          }
          assert(i < m);
          var b = this.pow(c, new BN(1).iushln(m - i - 1));
          r = r.redMul(b);
          c = b.redSqr();
          t = t.redMul(c);
          m = i;
        }
        return r;
      });
      Red.prototype.invm = $(function invm(a) {
        var inv = a._invmp(this.m);
        if (inv.negative !== 0) {
          inv.negative = 0;
          return this.imod(inv).redNeg();
        } else {
          return this.imod(inv);
        }
      });
      Red.prototype.pow = $(function pow(a, num) {
        if (num.isZero()) return new BN(1).toRed(this);
        if (num.cmpn(1) === 0) return a.clone();
        var windowSize = 4;
        var wnd = new Array(1 << windowSize);
        wnd[0] = new BN(1).toRed(this);
        wnd[1] = a;
        for (var i = 2; i < wnd.length; i++) {
          wnd[i] = this.mul(wnd[i - 1], a);
        }
        var res = wnd[0];
        var current = 0;
        var currentLen = 0;
        var start = num.bitLength() % 26;
        if (start === 0) {
          start = 26;
        }
        for (i = num.length - 1; i >= 0; i--) {
          var word = num.words[i];
          for (var j = start - 1; j >= 0; j--) {
            var bit = word >> j & 1;
            if (res !== wnd[0]) {
              res = this.sqr(res);
            }
            if (bit === 0 && current === 0) {
              currentLen = 0;
              continue;
            }
            current <<= 1;
            current |= bit;
            currentLen++;
            if (currentLen !== windowSize && (i !== 0 || j !== 0)) continue;
            res = this.mul(res, wnd[current]);
            currentLen = 0;
            current = 0;
          }
          start = 26;
        }
        return res;
      });
      Red.prototype.convertTo = $(function convertTo(num) {
        var r = num.umod(this.m);
        return r === num ? r.clone() : r;
      });
      Red.prototype.convertFrom = $(function convertFrom(num) {
        var res = num.clone();
        res.red = null;
        return res;
      });
      BN.mont = $(function mont(num) {
        return new Mont(num);
      });
      function Mont(m) {
        Red.call(this, m);
        this.shift = this.m.bitLength();
        if (this.shift % 26 !== 0) {
          this.shift += 26 - this.shift % 26;
        }
        this.r = new BN(1).iushln(this.shift);
        this.r2 = this.imod(this.r.sqr());
        this.rinv = this.r._invmp(this.m);
        this.minv = this.rinv.mul(this.r).isubn(1).div(this.m);
        this.minv = this.minv.umod(this.r);
        this.minv = this.r.sub(this.minv);
      }
      $(Mont);
      inherits(Mont, Red);
      Mont.prototype.convertTo = $(function convertTo(num) {
        return this.imod(num.ushln(this.shift));
      });
      Mont.prototype.convertFrom = $(function convertFrom(num) {
        var r = this.imod(num.mul(this.rinv));
        r.red = null;
        return r;
      });
      Mont.prototype.imul = $(function imul(a, b) {
        if (a.isZero() || b.isZero()) {
          a.words[0] = 0;
          a.length = 1;
          return a;
        }
        var t = a.imul(b);
        var c = t.maskn(this.shift).mul(this.minv).imaskn(this.shift).mul(this.m);
        var u = t.isub(c).iushrn(this.shift);
        var res = u;
        if (u.cmp(this.m) >= 0) {
          res = u.isub(this.m);
        } else if (u.cmpn(0) < 0) {
          res = u.iadd(this.m);
        }
        return res._forceRed(this);
      });
      Mont.prototype.mul = $(function mul(a, b) {
        if (a.isZero() || b.isZero()) return new BN(0)._forceRed(this);
        var t = a.mul(b);
        var c = t.maskn(this.shift).mul(this.minv).imaskn(this.shift).mul(this.m);
        var u = t.isub(c).iushrn(this.shift);
        var res = u;
        if (u.cmp(this.m) >= 0) {
          res = u.isub(this.m);
        } else if (u.cmpn(0) < 0) {
          res = u.iadd(this.m);
        }
        return res._forceRed(this);
      });
      Mont.prototype.invm = $(function invm(a) {
        var res = this.imod(a._invmp(this.m).mul(this.r2));
        return res._forceRed(this);
      });
    })(module, commonjsGlobal);
  })(bn);
  var bnExports = bn.exports;
  var BN = getDefaultExportFromCjs(bnExports);
  var safeBuffer = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  /*! safe-buffer. MIT License. Feross Aboukhadijeh <https://feross.org/opensource> */
  $(function (module, exports) {
    var buffer$1 = buffer;
    var Buffer = buffer$1.Buffer;
    function copyProps(src, dst) {
      for (var key in src) {
        dst[key] = src[key];
      }
    }
    $(copyProps);
    if (Buffer.from && Buffer.alloc && Buffer.allocUnsafe && Buffer.allocUnsafeSlow) {
      module.exports = buffer$1;
    } else {
      copyProps(buffer$1, exports);
      exports.Buffer = SafeBuffer;
    }
    function SafeBuffer(arg, encodingOrOffset, length) {
      return Buffer(arg, encodingOrOffset, length);
    }
    $(SafeBuffer);
    SafeBuffer.prototype = $Object.create(Buffer.prototype);
    copyProps(Buffer, SafeBuffer);
    SafeBuffer.from = $(function (arg, encodingOrOffset, length) {
      if (typeof arg === "number") {
        throw new TypeError("Argument must not be a number");
      }
      return Buffer(arg, encodingOrOffset, length);
    });
    SafeBuffer.alloc = $(function (size, fill, encoding) {
      if (typeof size !== "number") {
        throw new TypeError("Argument must be a number");
      }
      var buf = Buffer(size);
      if (fill !== undefined) {
        if (typeof encoding === "string") {
          buf.fill(fill, encoding);
        } else {
          buf.fill(fill);
        }
      } else {
        buf.fill(0);
      }
      return buf;
    });
    SafeBuffer.allocUnsafe = $(function (size) {
      if (typeof size !== "number") {
        throw new TypeError("Argument must be a number");
      }
      return Buffer(size);
    });
    SafeBuffer.allocUnsafeSlow = $(function (size) {
      if (typeof size !== "number") {
        throw new TypeError("Argument must be a number");
      }
      return buffer$1.SlowBuffer(size);
    });
  })(safeBuffer, safeBuffer.exports);
  var safeBufferExports = safeBuffer.exports;
  var _Buffer = safeBufferExports.Buffer;
  function base(ALPHABET) {
    if (ALPHABET.length >= 255) {
      throw new TypeError("Alphabet too long");
    }
    var BASE_MAP = new Uint8Array(256);
    for (var j = 0; j < BASE_MAP.length; j++) {
      BASE_MAP[j] = 255;
    }
    for (var i = 0; i < ALPHABET.length; i++) {
      var x = ALPHABET.charAt(i);
      var xc = x.charCodeAt(0);
      if (BASE_MAP[xc] !== 255) {
        throw new TypeError(x + " is ambiguous");
      }
      BASE_MAP[xc] = i;
    }
    var BASE = ALPHABET.length;
    var LEADER = ALPHABET.charAt(0);
    var FACTOR = Math.log(BASE) / Math.log(256);
    var iFACTOR = Math.log(256) / Math.log(BASE);
    function encode(source) {
      if ($Array.isArray(source) || source instanceof Uint8Array) {
        source = _Buffer.from(source);
      }
      if (!_Buffer.isBuffer(source)) {
        throw new TypeError("Expected Buffer");
      }
      if (source.length === 0) {
        return "";
      }
      var zeroes = 0;
      var length = 0;
      var pbegin = 0;
      var pend = source.length;
      while (pbegin !== pend && source[pbegin] === 0) {
        pbegin++;
        zeroes++;
      }
      var size = (pend - pbegin) * iFACTOR + 1 >>> 0;
      var b58 = new Uint8Array(size);
      while (pbegin !== pend) {
        var carry = source[pbegin];
        var i = 0;
        for (var it1 = size - 1; (carry !== 0 || i < length) && it1 !== -1; it1--, i++) {
          carry += 256 * b58[it1] >>> 0;
          b58[it1] = carry % BASE >>> 0;
          carry = carry / BASE >>> 0;
        }
        if (carry !== 0) {
          throw new Error("Non-zero carry");
        }
        length = i;
        pbegin++;
      }
      var it2 = size - length;
      while (it2 !== size && b58[it2] === 0) {
        it2++;
      }
      var str = LEADER.repeat(zeroes);
      for (; it2 < size; ++it2) {
        str += ALPHABET.charAt(b58[it2]);
      }
      return str;
    }
    $(encode);
    function decodeUnsafe(source) {
      if (typeof source !== "string") {
        throw new TypeError("Expected String");
      }
      if (source.length === 0) {
        return _Buffer.alloc(0);
      }
      var psz = 0;
      var zeroes = 0;
      var length = 0;
      while (source[psz] === LEADER) {
        zeroes++;
        psz++;
      }
      var size = (source.length - psz) * FACTOR + 1 >>> 0;
      var b256 = new Uint8Array(size);
      while (source[psz]) {
        var carry = BASE_MAP[source.charCodeAt(psz)];
        if (carry === 255) {
          return;
        }
        var i = 0;
        for (var it3 = size - 1; (carry !== 0 || i < length) && it3 !== -1; it3--, i++) {
          carry += BASE * b256[it3] >>> 0;
          b256[it3] = carry % 256 >>> 0;
          carry = carry / 256 >>> 0;
        }
        if (carry !== 0) {
          throw new Error("Non-zero carry");
        }
        length = i;
        psz++;
      }
      var it4 = size - length;
      while (it4 !== size && b256[it4] === 0) {
        it4++;
      }
      var vch = _Buffer.allocUnsafe(zeroes + (size - it4));
      vch.fill(0, 0, zeroes);
      var j = zeroes;
      while (it4 !== size) {
        vch[j++] = b256[it4++];
      }
      return vch;
    }
    $(decodeUnsafe);
    function decode(string) {
      var buffer = decodeUnsafe(string);
      if (buffer) {
        return buffer;
      }
      throw new Error("Non-base" + BASE + " character");
    }
    $(decode);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.encode = encode;
      result.decodeUnsafe = decodeUnsafe;
      result.decode = decode;
      return result;
    })();
  }
  $(base);
  var src = base;
  var basex = src;
  var ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
  var bs58 = basex(ALPHABET);
  var bs58$1 = getDefaultExportFromCjs(bs58);
  function number$1(n) {
    if (!Number.isSafeInteger(n) || n < 0) throw new Error(`Wrong positive integer: ${n}`);
  }
  $(number$1);
  function isBytes$1(a) {
    return a instanceof Uint8Array || a != null && typeof a === "object" && a.constructor.name === "Uint8Array";
  }
  $(isBytes$1);
  function bytes(b, ...lengths) {
    if (!isBytes$1(b)) throw new Error("Expected Uint8Array");
    if (lengths.length > 0 && !lengths.includes(b.length)) throw new Error(`Expected Uint8Array of length ${lengths}, not of length=${b.length}`);
  }
  $(bytes);
  function exists(instance, checkFinished = true) {
    if (instance.destroyed) throw new Error("Hash instance has been destroyed");
    if (checkFinished && instance.finished) throw new Error("Hash#digest() has already been called");
  }
  $(exists);
  function output(out, instance) {
    bytes(out);
    const min = instance.outputLen;
    if (out.length < min) {
      throw new Error(`digestInto() expects output buffer of length at least ${min}`);
    }
  }
  /*! noble-hashes - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  $(output);
  const u32$1 = arr => new Uint32Array(arr.buffer, arr.byteOffset, Math.floor(arr.byteLength / 4));
  $(u32$1);
  function isBytes(a) {
    return a instanceof Uint8Array || a != null && typeof a === "object" && a.constructor.name === "Uint8Array";
  }
  $(isBytes);
  const createView = arr => new DataView(arr.buffer, arr.byteOffset, arr.byteLength);
  $(createView);
  const rotr = (word, shift) => word << 32 - shift | word >>> shift;
  $(rotr);
  const isLE = new Uint8Array(new Uint32Array($Array.of(287454020)).buffer)[0] === 68;
  if (!isLE) throw new Error("Non little-endian hardware is not supported");
  function utf8ToBytes(str) {
    if (typeof str !== "string") throw new Error(`utf8ToBytes expected string, got ${typeof str}`);
    return new Uint8Array(new TextEncoder().encode(str));
  }
  $(utf8ToBytes);
  function toBytes(data) {
    if (typeof data === "string") data = utf8ToBytes(data);
    if (!isBytes(data)) throw new Error(`expected Uint8Array, got ${typeof data}`);
    return data;
  }
  $(toBytes);
  class Hash {
    clone() {
      return this._cloneInto();
    }
  }
  function wrapConstructor(hashCons) {
    const hashC = msg => hashCons().update(toBytes(msg)).digest();
    $(hashC);
    const tmp = hashCons();
    hashC.outputLen = tmp.outputLen;
    hashC.blockLen = tmp.blockLen;
    hashC.create = $(() => hashCons());
    return hashC;
  }
  $(wrapConstructor);
  function setBigUint64(view, byteOffset, value, isLE) {
    if (typeof view.setBigUint64 === "function") return view.setBigUint64(byteOffset, value, isLE);
    const _32n = BigInt(32);
    const _u32_max = BigInt(4294967295);
    const wh = Number(value >> _32n & _u32_max);
    const wl = Number(value & _u32_max);
    const h = isLE ? 4 : 0;
    const l = isLE ? 0 : 4;
    view.setUint32(byteOffset + h, wh, isLE);
    view.setUint32(byteOffset + l, wl, isLE);
  }
  $(setBigUint64);
  class SHA2 extends Hash {
    constructor(blockLen, outputLen, padOffset, isLE) {
      super();
      this.blockLen = blockLen;
      this.outputLen = outputLen;
      this.padOffset = padOffset;
      this.isLE = isLE;
      this.finished = false;
      this.length = 0;
      this.pos = 0;
      this.destroyed = false;
      this.buffer = new Uint8Array(blockLen);
      this.view = createView(this.buffer);
    }
    update(data) {
      exists(this);
      const {
        view: view,
        buffer: buffer,
        blockLen: blockLen
      } = this;
      data = toBytes(data);
      const len = data.length;
      for (let pos = 0; pos < len;) {
        const take = Math.min(blockLen - this.pos, len - pos);
        if (take === blockLen) {
          const dataView = createView(data);
          for (; blockLen <= len - pos; pos += blockLen) this.process(dataView, pos);
          continue;
        }
        buffer.set(data.subarray(pos, pos + take), this.pos);
        this.pos += take;
        pos += take;
        if (this.pos === blockLen) {
          this.process(view, 0);
          this.pos = 0;
        }
      }
      this.length += data.length;
      this.roundClean();
      return this;
    }
    digestInto(out) {
      exists(this);
      output(out, this);
      this.finished = true;
      const {
        buffer: buffer,
        view: view,
        blockLen: blockLen,
        isLE: isLE
      } = this;
      let {
        pos: pos
      } = this;
      buffer[pos++] = 128;
      this.buffer.subarray(pos).fill(0);
      if (this.padOffset > blockLen - pos) {
        this.process(view, 0);
        pos = 0;
      }
      for (let i = pos; i < blockLen; i++) buffer[i] = 0;
      setBigUint64(view, blockLen - 8, BigInt(this.length * 8), isLE);
      this.process(view, 0);
      const oview = createView(out);
      const len = this.outputLen;
      if (len % 4) throw new Error("_sha2: outputLen should be aligned to 32bit");
      const outLen = len / 4;
      const state = this.get();
      if (outLen > state.length) throw new Error("_sha2: outputLen bigger than state");
      for (let i = 0; i < outLen; i++) oview.setUint32(4 * i, state[i], isLE);
    }
    digest() {
      const {
        buffer: buffer,
        outputLen: outputLen
      } = this;
      this.digestInto(buffer);
      const res = buffer.slice(0, outputLen);
      this.destroy();
      return res;
    }
    _cloneInto(to) {
      to || (to = new this.constructor());
      to.set(...this.get());
      const {
        blockLen: blockLen,
        buffer: buffer,
        length: length,
        finished: finished,
        destroyed: destroyed,
        pos: pos
      } = this;
      to.length = length;
      to.pos = pos;
      to.finished = finished;
      to.destroyed = destroyed;
      if (length % blockLen) to.buffer.set(buffer);
      return to;
    }
  }
  const Chi = (a, b, c) => a & b ^ ~a & c;
  $(Chi);
  const Maj = (a, b, c) => a & b ^ a & c ^ b & c;
  $(Maj);
  const SHA256_K$1 = new Uint32Array($Array.of(1116352408, 1899447441, 3049323471, 3921009573, 961987163, 1508970993, 2453635748, 2870763221, 3624381080, 310598401, 607225278, 1426881987, 1925078388, 2162078206, 2614888103, 3248222580, 3835390401, 4022224774, 264347078, 604807628, 770255983, 1249150122, 1555081692, 1996064986, 2554220882, 2821834349, 2952996808, 3210313671, 3336571891, 3584528711, 113926993, 338241895, 666307205, 773529912, 1294757372, 1396182291, 1695183700, 1986661051, 2177026350, 2456956037, 2730485921, 2820302411, 3259730800, 3345764771, 3516065817, 3600352804, 4094571909, 275423344, 430227734, 506948616, 659060556, 883997877, 958139571, 1322822218, 1537002063, 1747873779, 1955562222, 2024104815, 2227730452, 2361852424, 2428436474, 2756734187, 3204031479, 3329325298));
  const IV = new Uint32Array($Array.of(1779033703, 3144134277, 1013904242, 2773480762, 1359893119, 2600822924, 528734635, 1541459225));
  const SHA256_W$1 = new Uint32Array(64);
  let SHA256$1 = class SHA256 extends SHA2 {
    constructor() {
      super(64, 32, 8, false);
      this.A = IV[0] | 0;
      this.B = IV[1] | 0;
      this.C = IV[2] | 0;
      this.D = IV[3] | 0;
      this.E = IV[4] | 0;
      this.F = IV[5] | 0;
      this.G = IV[6] | 0;
      this.H = IV[7] | 0;
    }
    get() {
      const {
        A: A,
        B: B,
        C: C,
        D: D,
        E: E,
        F: F,
        G: G,
        H: H
      } = this;
      return $Array.of(A, B, C, D, E, F, G, H);
    }
    set(A, B, C, D, E, F, G, H) {
      this.A = A | 0;
      this.B = B | 0;
      this.C = C | 0;
      this.D = D | 0;
      this.E = E | 0;
      this.F = F | 0;
      this.G = G | 0;
      this.H = H | 0;
    }
    process(view, offset) {
      for (let i = 0; i < 16; i++, offset += 4) SHA256_W$1[i] = view.getUint32(offset, false);
      for (let i = 16; i < 64; i++) {
        const W15 = SHA256_W$1[i - 15];
        const W2 = SHA256_W$1[i - 2];
        const s0 = rotr(W15, 7) ^ rotr(W15, 18) ^ W15 >>> 3;
        const s1 = rotr(W2, 17) ^ rotr(W2, 19) ^ W2 >>> 10;
        SHA256_W$1[i] = s1 + SHA256_W$1[i - 7] + s0 + SHA256_W$1[i - 16] | 0;
      }
      let {
        A: A,
        B: B,
        C: C,
        D: D,
        E: E,
        F: F,
        G: G,
        H: H
      } = this;
      for (let i = 0; i < 64; i++) {
        const sigma1 = rotr(E, 6) ^ rotr(E, 11) ^ rotr(E, 25);
        const T1 = H + sigma1 + Chi(E, F, G) + SHA256_K$1[i] + SHA256_W$1[i] | 0;
        const sigma0 = rotr(A, 2) ^ rotr(A, 13) ^ rotr(A, 22);
        const T2 = sigma0 + Maj(A, B, C) | 0;
        H = G;
        G = F;
        F = E;
        E = D + T1 | 0;
        D = C;
        C = B;
        B = A;
        A = T1 + T2 | 0;
      }
      A = A + this.A | 0;
      B = B + this.B | 0;
      C = C + this.C | 0;
      D = D + this.D | 0;
      E = E + this.E | 0;
      F = F + this.F | 0;
      G = G + this.G | 0;
      H = H + this.H | 0;
      this.set(A, B, C, D, E, F, G, H);
    }
    roundClean() {
      SHA256_W$1.fill(0);
    }
    destroy() {
      this.set(0, 0, 0, 0, 0, 0, 0, 0);
      this.buffer.fill(0);
    }
  };
  const sha256$1 = wrapConstructor($(() => new SHA256$1()));
  var lib = $Object.create(null, undefined);
  var encoding_lib = $Object.create(null, undefined);
  function inRange(a, min, max) {
    return min <= a && a <= max;
  }
  $(inRange);
  function ToDictionary(o) {
    if (o === undefined) return $Object.create(null, undefined);
    if (o === Object(o)) return o;
    throw TypeError("Could not convert argument to dictionary");
  }
  $(ToDictionary);
  function stringToCodePoints(string) {
    var s = String(string);
    var n = s.length;
    var i = 0;
    var u = $Array.of();
    while (i < n) {
      var c = s.charCodeAt(i);
      if (c < 55296 || c > 57343) {
        u.push(c);
      } else if (56320 <= c && c <= 57343) {
        u.push(65533);
      } else if (55296 <= c && c <= 56319) {
        if (i === n - 1) {
          u.push(65533);
        } else {
          var d = string.charCodeAt(i + 1);
          if (56320 <= d && d <= 57343) {
            var a = c & 1023;
            var b = d & 1023;
            u.push(65536 + (a << 10) + b);
            i += 1;
          } else {
            u.push(65533);
          }
        }
      }
      i += 1;
    }
    return u;
  }
  $(stringToCodePoints);
  function codePointsToString(code_points) {
    var s = "";
    for (var i = 0; i < code_points.length; ++i) {
      var cp = code_points[i];
      if (cp <= 65535) {
        s += String.fromCharCode(cp);
      } else {
        cp -= 65536;
        s += String.fromCharCode((cp >> 10) + 55296, (cp & 1023) + 56320);
      }
    }
    return s;
  }
  $(codePointsToString);
  var end_of_stream = -1;
  function Stream(tokens) {
    this.tokens = $Array.of().slice.call(tokens);
  }
  $(Stream);
  Stream.prototype = $(function () {
    let result = $Object.create(null, undefined);
    result.endOfStream = $(function () {
      return !this.tokens.length;
    });
    result.read = $(function () {
      if (!this.tokens.length) return end_of_stream;
      return this.tokens.shift();
    });
    result.prepend = $(function (token) {
      if ($Array.isArray(token)) {
        var tokens = token;
        while (tokens.length) this.tokens.unshift(tokens.pop());
      } else {
        this.tokens.unshift(token);
      }
    });
    result.push = $(function (token) {
      if ($Array.isArray(token)) {
        var tokens = token;
        while (tokens.length) this.tokens.push(tokens.shift());
      } else {
        this.tokens.push(token);
      }
    });
    return result;
  })();
  var finished = -1;
  function decoderError(fatal, opt_code_point) {
    if (fatal) throw TypeError("Decoder error");
    return opt_code_point || 65533;
  }
  $(decoderError);
  var DEFAULT_ENCODING = "utf-8";
  function TextDecoder$1(encoding, options) {
    if (!(this instanceof TextDecoder$1)) {
      return new TextDecoder$1(encoding, options);
    }
    encoding = encoding !== undefined ? String(encoding).toLowerCase() : DEFAULT_ENCODING;
    if (encoding !== DEFAULT_ENCODING) {
      throw new Error("Encoding not supported. Only utf-8 is supported");
    }
    options = ToDictionary(options);
    this._streaming = false;
    this._BOMseen = false;
    this._decoder = null;
    this._fatal = Boolean(options["fatal"]);
    this._ignoreBOM = Boolean(options["ignoreBOM"]);
    $Object.defineProperty(this, "encoding", $(function () {
      let result = $Object.create(null, undefined);
      result.value = "utf-8";
      return result;
    })());
    $Object.defineProperty(this, "fatal", $(function () {
      let result = $Object.create(null, undefined);
      result.value = this._fatal;
      return result;
    })());
    $Object.defineProperty(this, "ignoreBOM", $(function () {
      let result = $Object.create(null, undefined);
      result.value = this._ignoreBOM;
      return result;
    })());
  }
  $(TextDecoder$1);
  TextDecoder$1.prototype = $(function () {
    let result = $Object.create(null, undefined);
    result.decode = $(function decode(input, options) {
      var bytes;
      if (typeof input === "object" && input instanceof ArrayBuffer) {
        bytes = new Uint8Array(input);
      } else if (typeof input === "object" && "buffer" in input && input.buffer instanceof ArrayBuffer) {
        bytes = new Uint8Array(input.buffer, input.byteOffset, input.byteLength);
      } else {
        bytes = new Uint8Array(0);
      }
      options = ToDictionary(options);
      if (!this._streaming) {
        this._decoder = new UTF8Decoder($(function () {
          let result = $Object.create(null, undefined);
          result.fatal = this._fatal;
          return result;
        })());
        this._BOMseen = false;
      }
      this._streaming = Boolean(options["stream"]);
      var input_stream = new Stream(bytes);
      var code_points = $Array.of();
      var result;
      while (!input_stream.endOfStream()) {
        result = this._decoder.handler(input_stream, input_stream.read());
        if (result === finished) break;
        if (result === null) continue;
        if ($Array.isArray(result)) code_points.push.apply(code_points, result);else code_points.push(result);
      }
      if (!this._streaming) {
        do {
          result = this._decoder.handler(input_stream, input_stream.read());
          if (result === finished) break;
          if (result === null) continue;
          if ($Array.isArray(result)) code_points.push.apply(code_points, result);else code_points.push(result);
        } while (!input_stream.endOfStream());
        this._decoder = null;
      }
      if (code_points.length) {
        if ($Array.of("utf-8").indexOf(this.encoding) !== -1 && !this._ignoreBOM && !this._BOMseen) {
          if (code_points[0] === 65279) {
            this._BOMseen = true;
            code_points.shift();
          } else {
            this._BOMseen = true;
          }
        }
      }
      return codePointsToString(code_points);
    });
    return result;
  })();
  function TextEncoder$1(encoding, options) {
    if (!(this instanceof TextEncoder$1)) return new TextEncoder$1(encoding, options);
    encoding = encoding !== undefined ? String(encoding).toLowerCase() : DEFAULT_ENCODING;
    if (encoding !== DEFAULT_ENCODING) {
      throw new Error("Encoding not supported. Only utf-8 is supported");
    }
    options = ToDictionary(options);
    this._streaming = false;
    this._encoder = null;
    this._options = $(function () {
      let result = $Object.create(null, undefined);
      result.fatal = Boolean(options["fatal"]);
      return result;
    })();
    $Object.defineProperty(this, "encoding", $(function () {
      let result = $Object.create(null, undefined);
      result.value = "utf-8";
      return result;
    })());
  }
  $(TextEncoder$1);
  TextEncoder$1.prototype = $(function () {
    let result = $Object.create(null, undefined);
    result.encode = $(function encode(opt_string, options) {
      opt_string = opt_string ? String(opt_string) : "";
      options = ToDictionary(options);
      if (!this._streaming) this._encoder = new UTF8Encoder(this._options);
      this._streaming = Boolean(options["stream"]);
      var bytes = $Array.of();
      var input_stream = new Stream(stringToCodePoints(opt_string));
      var result;
      while (!input_stream.endOfStream()) {
        result = this._encoder.handler(input_stream, input_stream.read());
        if (result === finished) break;
        if ($Array.isArray(result)) bytes.push.apply(bytes, result);else bytes.push(result);
      }
      if (!this._streaming) {
        while (true) {
          result = this._encoder.handler(input_stream, input_stream.read());
          if (result === finished) break;
          if ($Array.isArray(result)) bytes.push.apply(bytes, result);else bytes.push(result);
        }
        this._encoder = null;
      }
      return new Uint8Array(bytes);
    });
    return result;
  })();
  function UTF8Decoder(options) {
    var fatal = options.fatal;
    var utf8_code_point = 0,
      utf8_bytes_seen = 0,
      utf8_bytes_needed = 0,
      utf8_lower_boundary = 128,
      utf8_upper_boundary = 191;
    this.handler = $(function (stream, bite) {
      if (bite === end_of_stream && utf8_bytes_needed !== 0) {
        utf8_bytes_needed = 0;
        return decoderError(fatal);
      }
      if (bite === end_of_stream) return finished;
      if (utf8_bytes_needed === 0) {
        if (inRange(bite, 0, 127)) {
          return bite;
        }
        if (inRange(bite, 194, 223)) {
          utf8_bytes_needed = 1;
          utf8_code_point = bite - 192;
        } else if (inRange(bite, 224, 239)) {
          if (bite === 224) utf8_lower_boundary = 160;
          if (bite === 237) utf8_upper_boundary = 159;
          utf8_bytes_needed = 2;
          utf8_code_point = bite - 224;
        } else if (inRange(bite, 240, 244)) {
          if (bite === 240) utf8_lower_boundary = 144;
          if (bite === 244) utf8_upper_boundary = 143;
          utf8_bytes_needed = 3;
          utf8_code_point = bite - 240;
        } else {
          return decoderError(fatal);
        }
        utf8_code_point = utf8_code_point << 6 * utf8_bytes_needed;
        return null;
      }
      if (!inRange(bite, utf8_lower_boundary, utf8_upper_boundary)) {
        utf8_code_point = utf8_bytes_needed = utf8_bytes_seen = 0;
        utf8_lower_boundary = 128;
        utf8_upper_boundary = 191;
        stream.prepend(bite);
        return decoderError(fatal);
      }
      utf8_lower_boundary = 128;
      utf8_upper_boundary = 191;
      utf8_bytes_seen += 1;
      utf8_code_point += bite - 128 << 6 * (utf8_bytes_needed - utf8_bytes_seen);
      if (utf8_bytes_seen !== utf8_bytes_needed) return null;
      var code_point = utf8_code_point;
      utf8_code_point = utf8_bytes_needed = utf8_bytes_seen = 0;
      return code_point;
    });
  }
  $(UTF8Decoder);
  function UTF8Encoder(options) {
    options.fatal;
    this.handler = $(function (stream, code_point) {
      if (code_point === end_of_stream) return finished;
      if (inRange(code_point, 0, 127)) return code_point;
      var count, offset;
      if (inRange(code_point, 128, 2047)) {
        count = 1;
        offset = 192;
      } else if (inRange(code_point, 2048, 65535)) {
        count = 2;
        offset = 224;
      } else if (inRange(code_point, 65536, 1114111)) {
        count = 3;
        offset = 240;
      }
      var bytes = $Array.of((code_point >> 6 * count) + offset);
      while (count > 0) {
        var temp = code_point >> 6 * (count - 1);
        bytes.push(128 | temp & 63);
        count -= 1;
      }
      return bytes;
    });
  }
  $(UTF8Encoder);
  encoding_lib.TextEncoder = TextEncoder$1;
  encoding_lib.TextDecoder = TextDecoder$1;
  var __createBinding = commonjsGlobal && commonjsGlobal.__createBinding || ($Object.create ? $(function (o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    $Object.defineProperty(o, k2, $(function () {
      let result = $Object.create(null, undefined);
      result.enumerable = true;
      result.get = $(function () {
        return m[k];
      });
      return result;
    })());
  }) : $(function (o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
  }));
  var __setModuleDefault = commonjsGlobal && commonjsGlobal.__setModuleDefault || ($Object.create ? $(function (o, v) {
    $Object.defineProperty(o, "default", $(function () {
      let result = $Object.create(null, undefined);
      result.enumerable = true;
      result.value = v;
      return result;
    })());
  }) : $(function (o, v) {
    o["default"] = v;
  }));
  var __decorate = commonjsGlobal && commonjsGlobal.__decorate || $(function (decorators, target, key, desc) {
    var c = arguments.length,
      r = c < 3 ? target : desc === null ? desc = $Object.getOwnPropertyDescriptor(target, key) : desc,
      d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && $Object.defineProperty(target, key, r), r;
  });
  var __importStar = commonjsGlobal && commonjsGlobal.__importStar || $(function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = $Object.create(null, undefined);
    if (mod != null) for (var k in mod) if (k !== "default" && $Object.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
  });
  var __importDefault = commonjsGlobal && commonjsGlobal.__importDefault || $(function (mod) {
    return mod && mod.__esModule ? mod : $(function () {
      let result = $Object.create(null, undefined);
      result.default = mod;
      return result;
    })();
  });
  $Object.defineProperty(lib, "__esModule", $(function () {
    let result = $Object.create(null, undefined);
    result.value = true;
    return result;
  })());
  var deserializeUnchecked_1 = lib.deserializeUnchecked = deserialize_1 = lib.deserialize = serialize_1 = lib.serialize = lib.BinaryReader = lib.BinaryWriter = lib.BorshError = lib.baseDecode = lib.baseEncode = void 0;
  const bn_js_1 = __importDefault(bnExports);
  const bs58_1 = __importDefault(bs58);
  const encoding = __importStar(encoding_lib);
  const ResolvedTextDecoder = typeof TextDecoder !== "function" ? encoding.TextDecoder : TextDecoder;
  const textDecoder = new ResolvedTextDecoder("utf-8", $(function () {
    let result = $Object.create(null, undefined);
    result.fatal = true;
    return result;
  })());
  function baseEncode(value) {
    if (typeof value === "string") {
      value = Buffer.from(value, "utf8");
    }
    return bs58_1.default.encode(Buffer.from(value));
  }
  $(baseEncode);
  lib.baseEncode = baseEncode;
  function baseDecode(value) {
    return Buffer.from(bs58_1.default.decode(value));
  }
  $(baseDecode);
  lib.baseDecode = baseDecode;
  const INITIAL_LENGTH = 1024;
  class BorshError extends Error {
    constructor(message) {
      super(message);
      this.fieldPath = $Array.of();
      this.originalMessage = message;
    }
    addToFieldPath(fieldName) {
      this.fieldPath.splice(0, 0, fieldName);
      this.message = this.originalMessage + ": " + this.fieldPath.join(".");
    }
  }
  lib.BorshError = BorshError;
  class BinaryWriter {
    constructor() {
      this.buf = Buffer.alloc(INITIAL_LENGTH);
      this.length = 0;
    }
    maybeResize() {
      if (this.buf.length < 16 + this.length) {
        this.buf = Buffer.concat($Array.of(this.buf, Buffer.alloc(INITIAL_LENGTH)));
      }
    }
    writeU8(value) {
      this.maybeResize();
      this.buf.writeUInt8(value, this.length);
      this.length += 1;
    }
    writeU16(value) {
      this.maybeResize();
      this.buf.writeUInt16LE(value, this.length);
      this.length += 2;
    }
    writeU32(value) {
      this.maybeResize();
      this.buf.writeUInt32LE(value, this.length);
      this.length += 4;
    }
    writeU64(value) {
      this.maybeResize();
      this.writeBuffer(Buffer.from(new bn_js_1.default(value).toArray("le", 8)));
    }
    writeU128(value) {
      this.maybeResize();
      this.writeBuffer(Buffer.from(new bn_js_1.default(value).toArray("le", 16)));
    }
    writeU256(value) {
      this.maybeResize();
      this.writeBuffer(Buffer.from(new bn_js_1.default(value).toArray("le", 32)));
    }
    writeU512(value) {
      this.maybeResize();
      this.writeBuffer(Buffer.from(new bn_js_1.default(value).toArray("le", 64)));
    }
    writeBuffer(buffer) {
      this.buf = Buffer.concat($Array.of(Buffer.from(this.buf.subarray(0, this.length)), buffer, Buffer.alloc(INITIAL_LENGTH)));
      this.length += buffer.length;
    }
    writeString(str) {
      this.maybeResize();
      const b = Buffer.from(str, "utf8");
      this.writeU32(b.length);
      this.writeBuffer(b);
    }
    writeFixedArray(array) {
      this.writeBuffer(Buffer.from(array));
    }
    writeArray(array, fn) {
      this.maybeResize();
      this.writeU32(array.length);
      for (const elem of array) {
        this.maybeResize();
        fn(elem);
      }
    }
    toArray() {
      return this.buf.subarray(0, this.length);
    }
  }
  lib.BinaryWriter = BinaryWriter;
  function handlingRangeError(target, propertyKey, propertyDescriptor) {
    const originalMethod = propertyDescriptor.value;
    propertyDescriptor.value = $(function (...args) {
      try {
        return originalMethod.apply(this, args);
      } catch (e) {
        if (e instanceof RangeError) {
          const code = e.code;
          if ($Array.of("ERR_BUFFER_OUT_OF_BOUNDS", "ERR_OUT_OF_RANGE").indexOf(code) >= 0) {
            throw new BorshError("Reached the end of buffer when deserializing");
          }
        }
        throw e;
      }
    });
  }
  $(handlingRangeError);
  class BinaryReader {
    constructor(buf) {
      this.buf = buf;
      this.offset = 0;
    }
    readU8() {
      const value = this.buf.readUInt8(this.offset);
      this.offset += 1;
      return value;
    }
    readU16() {
      const value = this.buf.readUInt16LE(this.offset);
      this.offset += 2;
      return value;
    }
    readU32() {
      const value = this.buf.readUInt32LE(this.offset);
      this.offset += 4;
      return value;
    }
    readU64() {
      const buf = this.readBuffer(8);
      return new bn_js_1.default(buf, "le");
    }
    readU128() {
      const buf = this.readBuffer(16);
      return new bn_js_1.default(buf, "le");
    }
    readU256() {
      const buf = this.readBuffer(32);
      return new bn_js_1.default(buf, "le");
    }
    readU512() {
      const buf = this.readBuffer(64);
      return new bn_js_1.default(buf, "le");
    }
    readBuffer(len) {
      if (this.offset + len > this.buf.length) {
        throw new BorshError(`Expected buffer length ${len} isn't within bounds`);
      }
      const result = this.buf.slice(this.offset, this.offset + len);
      this.offset += len;
      return result;
    }
    readString() {
      const len = this.readU32();
      const buf = this.readBuffer(len);
      try {
        return textDecoder.decode(buf);
      } catch (e) {
        throw new BorshError(`Error decoding UTF-8 string: ${e}`);
      }
    }
    readFixedArray(len) {
      return new Uint8Array(this.readBuffer(len));
    }
    readArray(fn) {
      const len = this.readU32();
      const result = Array();
      for (let i = 0; i < len; ++i) {
        result.push(fn());
      }
      return result;
    }
  }
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU8", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU16", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU32", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU64", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU128", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU256", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readU512", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readString", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readFixedArray", null);
  __decorate($Array.of(handlingRangeError), BinaryReader.prototype, "readArray", null);
  lib.BinaryReader = BinaryReader;
  function capitalizeFirstLetter(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
  }
  $(capitalizeFirstLetter);
  function serializeField(schema, fieldName, value, fieldType, writer) {
    try {
      if (typeof fieldType === "string") {
        writer[`write${capitalizeFirstLetter(fieldType)}`](value);
      } else if (fieldType instanceof Array) {
        if (typeof fieldType[0] === "number") {
          if (value.length !== fieldType[0]) {
            throw new BorshError(`Expecting byte array of length ${fieldType[0]}, but got ${value.length} bytes`);
          }
          writer.writeFixedArray(value);
        } else if (fieldType.length === 2 && typeof fieldType[1] === "number") {
          if (value.length !== fieldType[1]) {
            throw new BorshError(`Expecting byte array of length ${fieldType[1]}, but got ${value.length} bytes`);
          }
          for (let i = 0; i < fieldType[1]; i++) {
            serializeField(schema, null, value[i], fieldType[0], writer);
          }
        } else {
          writer.writeArray(value, $(item => {
            serializeField(schema, fieldName, item, fieldType[0], writer);
          }));
        }
      } else if (fieldType.kind !== undefined) {
        switch (fieldType.kind) {
          case "option":
            {
              if (value === null || value === undefined) {
                writer.writeU8(0);
              } else {
                writer.writeU8(1);
                serializeField(schema, fieldName, value, fieldType.type, writer);
              }
              break;
            }
          case "map":
            {
              writer.writeU32(value.size);
              value.forEach($((val, key) => {
                serializeField(schema, fieldName, key, fieldType.key, writer);
                serializeField(schema, fieldName, val, fieldType.value, writer);
              }));
              break;
            }
          default:
            throw new BorshError(`FieldType ${fieldType} unrecognized`);
        }
      } else {
        serializeStruct(schema, value, writer);
      }
    } catch (error) {
      if (error instanceof BorshError) {
        error.addToFieldPath(fieldName);
      }
      throw error;
    }
  }
  $(serializeField);
  function serializeStruct(schema, obj, writer) {
    if (typeof obj.borshSerialize === "function") {
      obj.borshSerialize(writer);
      return;
    }
    const structSchema = schema.get(obj.constructor);
    if (!structSchema) {
      throw new BorshError(`Class ${obj.constructor.name} is missing in schema`);
    }
    if (structSchema.kind === "struct") {
      structSchema.fields.map($(([fieldName, fieldType]) => {
        serializeField(schema, fieldName, obj[fieldName], fieldType, writer);
      }));
    } else if (structSchema.kind === "enum") {
      const name = obj[structSchema.field];
      for (let idx = 0; idx < structSchema.values.length; ++idx) {
        const [fieldName, fieldType] = structSchema.values[idx];
        if (fieldName === name) {
          writer.writeU8(idx);
          serializeField(schema, fieldName, obj[fieldName], fieldType, writer);
          break;
        }
      }
    } else {
      throw new BorshError(`Unexpected schema kind: ${structSchema.kind} for ${obj.constructor.name}`);
    }
  }
  $(serializeStruct);
  function serialize(schema, obj, Writer = BinaryWriter) {
    const writer = new Writer();
    serializeStruct(schema, obj, writer);
    return writer.toArray();
  }
  $(serialize);
  var serialize_1 = lib.serialize = serialize;
  function deserializeField(schema, fieldName, fieldType, reader) {
    try {
      if (typeof fieldType === "string") {
        return reader[`read${capitalizeFirstLetter(fieldType)}`]();
      }
      if (fieldType instanceof Array) {
        if (typeof fieldType[0] === "number") {
          return reader.readFixedArray(fieldType[0]);
        } else if (typeof fieldType[1] === "number") {
          const arr = $Array.of();
          for (let i = 0; i < fieldType[1]; i++) {
            arr.push(deserializeField(schema, null, fieldType[0], reader));
          }
          return arr;
        } else {
          return reader.readArray($(() => deserializeField(schema, fieldName, fieldType[0], reader)));
        }
      }
      if (fieldType.kind === "option") {
        const option = reader.readU8();
        if (option) {
          return deserializeField(schema, fieldName, fieldType.type, reader);
        }
        return undefined;
      }
      if (fieldType.kind === "map") {
        let map = new Map();
        const length = reader.readU32();
        for (let i = 0; i < length; i++) {
          const key = deserializeField(schema, fieldName, fieldType.key, reader);
          const val = deserializeField(schema, fieldName, fieldType.value, reader);
          map.set(key, val);
        }
        return map;
      }
      return deserializeStruct(schema, fieldType, reader);
    } catch (error) {
      if (error instanceof BorshError) {
        error.addToFieldPath(fieldName);
      }
      throw error;
    }
  }
  $(deserializeField);
  function deserializeStruct(schema, classType, reader) {
    if (typeof classType.borshDeserialize === "function") {
      return classType.borshDeserialize(reader);
    }
    const structSchema = schema.get(classType);
    if (!structSchema) {
      throw new BorshError(`Class ${classType.name} is missing in schema`);
    }
    if (structSchema.kind === "struct") {
      const result = $Object.create(null, undefined);
      for (const [fieldName, fieldType] of schema.get(classType).fields) {
        result[fieldName] = deserializeField(schema, fieldName, fieldType, reader);
      }
      return new classType(result);
    }
    if (structSchema.kind === "enum") {
      const idx = reader.readU8();
      if (idx >= structSchema.values.length) {
        throw new BorshError(`Enum index: ${idx} is out of range`);
      }
      const [fieldName, fieldType] = structSchema.values[idx];
      const fieldValue = deserializeField(schema, fieldName, fieldType, reader);
      return new classType($(function () {
        let result = $Object.create(null, undefined);
        result.fieldName = fieldValue;
        return result;
      })());
    }
    throw new BorshError(`Unexpected schema kind: ${structSchema.kind} for ${classType.constructor.name}`);
  }
  $(deserializeStruct);
  function deserialize(schema, classType, buffer, Reader = BinaryReader) {
    const reader = new Reader(buffer);
    const result = deserializeStruct(schema, classType, reader);
    if (reader.offset < buffer.length) {
      throw new BorshError(`Unexpected ${buffer.length - reader.offset} bytes after deserialized data`);
    }
    return result;
  }
  $(deserialize);
  var deserialize_1 = lib.deserialize = deserialize;
  function deserializeUnchecked(schema, classType, buffer, Reader = BinaryReader) {
    const reader = new Reader(buffer);
    return deserializeStruct(schema, classType, reader);
  }
  $(deserializeUnchecked);
  deserializeUnchecked_1 = lib.deserializeUnchecked = deserializeUnchecked;
  let Struct$1 = class Struct {
    constructor(properties) {
      $Object.assign(this, properties);
    }
    encode() {
      return buffer.Buffer.from(serialize_1(SOLANA_SCHEMA, this));
    }
    static decode(data) {
      return deserialize_1(SOLANA_SCHEMA, this, data);
    }
    static decodeUnchecked(data) {
      return deserializeUnchecked_1(SOLANA_SCHEMA, this, data);
    }
  };
  class Enum extends Struct$1 {
    constructor(properties) {
      super(properties);
      this.enum = "";
      if ($Object.keys(properties).length !== 1) {
        throw new Error("Enum can only take single value");
      }
      $Object.keys(properties).map($(key => {
        this.enum = key;
      }));
    }
  }
  const SOLANA_SCHEMA = new Map();
  var _class;
  let _Symbol$toStringTag;
  const MAX_SEED_LENGTH = 32;
  const PUBLIC_KEY_LENGTH = 32;
  function isPublicKeyData(value) {
    return value._bn !== undefined;
  }
  $(isPublicKeyData);
  let uniquePublicKeyCounter = 1;
  _Symbol$toStringTag = Symbol.toStringTag;
  class PublicKey extends Struct$1 {
    constructor(value) {
      super($Object.create(null, undefined));
      this._bn = void 0;
      if (isPublicKeyData(value)) {
        this._bn = value._bn;
      } else {
        if (typeof value === "string") {
          const decoded = bs58$1.decode(value);
          if (decoded.length != PUBLIC_KEY_LENGTH) {
            throw new Error(`Invalid public key input`);
          }
          this._bn = new BN(decoded);
        } else {
          this._bn = new BN(value);
        }
        if (this._bn.byteLength() > PUBLIC_KEY_LENGTH) {
          throw new Error(`Invalid public key input`);
        }
      }
    }
    static unique() {
      const key = new PublicKey(uniquePublicKeyCounter);
      uniquePublicKeyCounter += 1;
      return new PublicKey(key.toBuffer());
    }
    equals(publicKey) {
      return this._bn.eq(publicKey._bn);
    }
    toBase58() {
      return bs58$1.encode(this.toBytes());
    }
    toJSON() {
      return this.toBase58();
    }
    toBytes() {
      const buf = this.toBuffer();
      return new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
    }
    toBuffer() {
      const b = this._bn.toArrayLike(buffer.Buffer);
      if (b.length === PUBLIC_KEY_LENGTH) {
        return b;
      }
      const zeroPad = buffer.Buffer.alloc(32);
      b.copy(zeroPad, 32 - b.length);
      return zeroPad;
    }
    get [_Symbol$toStringTag]() {
      return `PublicKey(${this.toString()})`;
    }
    toString() {
      return this.toBase58();
    }
    static async createWithSeed(fromPublicKey, seed, programId) {
      const buffer$1 = buffer.Buffer.concat($Array.of(fromPublicKey.toBuffer(), buffer.Buffer.from(seed), programId.toBuffer()));
      const publicKeyBytes = sha256$1(buffer$1);
      return new PublicKey(publicKeyBytes);
    }
    static createProgramAddressSync(seeds, programId) {
      let buffer$1 = buffer.Buffer.alloc(0);
      seeds.forEach($(function (seed) {
        if (seed.length > MAX_SEED_LENGTH) {
          throw new TypeError(`Max seed length exceeded`);
        }
        buffer$1 = buffer.Buffer.concat($Array.of(buffer$1, toBuffer(seed)));
      }));
      buffer$1 = buffer.Buffer.concat($Array.of(buffer$1, programId.toBuffer(), buffer.Buffer.from("ProgramDerivedAddress")));
      const publicKeyBytes = sha256$1(buffer$1);
      if (isOnCurve(publicKeyBytes)) {
        throw new Error(`Invalid seeds, address must fall off the curve`);
      }
      return new PublicKey(publicKeyBytes);
    }
    static async createProgramAddress(seeds, programId) {
      return this.createProgramAddressSync(seeds, programId);
    }
    static findProgramAddressSync(seeds, programId) {
      let nonce = 255;
      let address;
      while (nonce != 0) {
        try {
          const seedsWithNonce = seeds.concat(buffer.Buffer.from($Array.of(nonce)));
          address = this.createProgramAddressSync(seedsWithNonce, programId);
        } catch (err) {
          if (err instanceof TypeError) {
            throw err;
          }
          nonce--;
          continue;
        }
        return $Array.of(address, nonce);
      }
      throw new Error(`Unable to find a viable program address nonce`);
    }
    static async findProgramAddress(seeds, programId) {
      return this.findProgramAddressSync(seeds, programId);
    }
    static isOnCurve(pubkeyData) {
      const pubkey = new PublicKey(pubkeyData);
      return isOnCurve(pubkey.toBytes());
    }
  }
  _class = PublicKey;
  PublicKey.default = new _class("11111111111111111111111111111111");
  SOLANA_SCHEMA.set(PublicKey, $(function () {
    let result = $Object.create(null, undefined);
    result.kind = "struct";
    result.fields = $Array.of($Array.of("_bn", "u256"));
    return result;
  })());
  class Account {
    constructor(secretKey) {
      this._publicKey = void 0;
      this._secretKey = void 0;
      if (secretKey) {
        const secretKeyBuffer = toBuffer(secretKey);
        if (secretKey.length !== 64) {
          throw new Error("bad secret key size");
        }
        this._publicKey = secretKeyBuffer.slice(32, 64);
        this._secretKey = secretKeyBuffer.slice(0, 32);
      } else {
        this._secretKey = toBuffer(generatePrivateKey());
        this._publicKey = toBuffer(getPublicKey(this._secretKey));
      }
    }
    get publicKey() {
      return new PublicKey(this._publicKey);
    }
    get secretKey() {
      return buffer.Buffer.concat($Array.of(this._secretKey, this._publicKey), 64);
    }
  }
  const BPF_LOADER_DEPRECATED_PROGRAM_ID = new PublicKey("BPFLoader1111111111111111111111111111111111");
  var Layout$1 = $Object.create(null, undefined);
  $Object.defineProperty(Layout$1, "__esModule", $(function () {
    let result = $Object.create(null, undefined);
    result.value = true;
    return result;
  })());
  Layout$1.s16 = Layout$1.s8 = Layout$1.nu64be = Layout$1.u48be = Layout$1.u40be = Layout$1.u32be = Layout$1.u24be = Layout$1.u16be = nu64 = Layout$1.nu64 = Layout$1.u48 = Layout$1.u40 = u32 = Layout$1.u32 = Layout$1.u24 = u16 = Layout$1.u16 = u8 = Layout$1.u8 = offset = Layout$1.offset = Layout$1.greedy = Layout$1.Constant = Layout$1.UTF8 = Layout$1.CString = Layout$1.Blob = Layout$1.Boolean = Layout$1.BitField = Layout$1.BitStructure = Layout$1.VariantLayout = Layout$1.Union = Layout$1.UnionLayoutDiscriminator = Layout$1.UnionDiscriminator = Layout$1.Structure = Layout$1.Sequence = Layout$1.DoubleBE = Layout$1.Double = Layout$1.FloatBE = Layout$1.Float = Layout$1.NearInt64BE = Layout$1.NearInt64 = Layout$1.NearUInt64BE = Layout$1.NearUInt64 = Layout$1.IntBE = Layout$1.Int = Layout$1.UIntBE = Layout$1.UInt = Layout$1.OffsetLayout = Layout$1.GreedyCount = Layout$1.ExternalLayout = Layout$1.bindConstructorLayout = Layout$1.nameWithProperty = Layout$1.Layout = Layout$1.uint8ArrayToBuffer = Layout$1.checkUint8Array = void 0;
  Layout$1.constant = Layout$1.utf8 = Layout$1.cstr = blob = Layout$1.blob = Layout$1.unionLayoutDiscriminator = Layout$1.union = seq = Layout$1.seq = Layout$1.bits = struct = Layout$1.struct = Layout$1.f64be = Layout$1.f64 = Layout$1.f32be = Layout$1.f32 = Layout$1.ns64be = Layout$1.s48be = Layout$1.s40be = Layout$1.s32be = Layout$1.s24be = Layout$1.s16be = ns64 = Layout$1.ns64 = Layout$1.s48 = Layout$1.s40 = Layout$1.s32 = Layout$1.s24 = void 0;
  const buffer_1 = buffer;
  function checkUint8Array(b) {
    if (!(b instanceof Uint8Array)) {
      throw new TypeError("b must be a Uint8Array");
    }
  }
  $(checkUint8Array);
  Layout$1.checkUint8Array = checkUint8Array;
  function uint8ArrayToBuffer(b) {
    checkUint8Array(b);
    return buffer_1.Buffer.from(b.buffer, b.byteOffset, b.length);
  }
  $(uint8ArrayToBuffer);
  Layout$1.uint8ArrayToBuffer = uint8ArrayToBuffer;
  class Layout {
    constructor(span, property) {
      if (!Number.isInteger(span)) {
        throw new TypeError("span must be an integer");
      }
      this.span = span;
      this.property = property;
    }
    makeDestinationObject() {
      return $Object.create(null, undefined);
    }
    getSpan(b, offset) {
      if (0 > this.span) {
        throw new RangeError("indeterminate span");
      }
      return this.span;
    }
    replicate(property) {
      const rv = $Object.create(this.constructor.prototype);
      $Object.assign(rv, this);
      rv.property = property;
      return rv;
    }
    fromArray(values) {
      return undefined;
    }
  }
  Layout$1.Layout = Layout;
  function nameWithProperty(name, lo) {
    if (lo.property) {
      return name + "[" + lo.property + "]";
    }
    return name;
  }
  $(nameWithProperty);
  Layout$1.nameWithProperty = nameWithProperty;
  function bindConstructorLayout(Class, layout) {
    if ("function" !== typeof Class) {
      throw new TypeError("Class must be constructor");
    }
    if ($Object.prototype.hasOwnProperty.call(Class, "layout_")) {
      throw new Error("Class is already bound to a layout");
    }
    if (!(layout && layout instanceof Layout)) {
      throw new TypeError("layout must be a Layout");
    }
    if ($Object.prototype.hasOwnProperty.call(layout, "boundConstructor_")) {
      throw new Error("layout is already bound to a constructor");
    }
    Class.layout_ = layout;
    layout.boundConstructor_ = Class;
    layout.makeDestinationObject = $(() => new Class());
    $Object.defineProperty(Class.prototype, "encode", $(function () {
      let result = $Object.create(null, undefined);
      result.value = $(function (b, offset) {
        return layout.encode(this, b, offset);
      });
      result.writable = true;
      return result;
    })());
    $Object.defineProperty(Class, "decode", $(function () {
      let result = $Object.create(null, undefined);
      result.value = $(function (b, offset) {
        return layout.decode(b, offset);
      });
      result.writable = true;
      return result;
    })());
  }
  $(bindConstructorLayout);
  Layout$1.bindConstructorLayout = bindConstructorLayout;
  class ExternalLayout extends Layout {
    isCount() {
      throw new Error("ExternalLayout is abstract");
    }
  }
  Layout$1.ExternalLayout = ExternalLayout;
  class GreedyCount extends ExternalLayout {
    constructor(elementSpan = 1, property) {
      if (!Number.isInteger(elementSpan) || 0 >= elementSpan) {
        throw new TypeError("elementSpan must be a (positive) integer");
      }
      super(-1, property);
      this.elementSpan = elementSpan;
    }
    isCount() {
      return true;
    }
    decode(b, offset = 0) {
      checkUint8Array(b);
      const rem = b.length - offset;
      return Math.floor(rem / this.elementSpan);
    }
    encode(src, b, offset) {
      return 0;
    }
  }
  Layout$1.GreedyCount = GreedyCount;
  class OffsetLayout extends ExternalLayout {
    constructor(layout, offset = 0, property) {
      if (!(layout instanceof Layout)) {
        throw new TypeError("layout must be a Layout");
      }
      if (!Number.isInteger(offset)) {
        throw new TypeError("offset must be integer or undefined");
      }
      super(layout.span, property || layout.property);
      this.layout = layout;
      this.offset = offset;
    }
    isCount() {
      return this.layout instanceof UInt || this.layout instanceof UIntBE;
    }
    decode(b, offset = 0) {
      return this.layout.decode(b, offset + this.offset);
    }
    encode(src, b, offset = 0) {
      return this.layout.encode(src, b, offset + this.offset);
    }
  }
  Layout$1.OffsetLayout = OffsetLayout;
  class UInt extends Layout {
    constructor(span, property) {
      super(span, property);
      if (6 < this.span) {
        throw new RangeError("span must not exceed 6 bytes");
      }
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readUIntLE(offset, this.span);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeUIntLE(src, offset, this.span);
      return this.span;
    }
  }
  Layout$1.UInt = UInt;
  class UIntBE extends Layout {
    constructor(span, property) {
      super(span, property);
      if (6 < this.span) {
        throw new RangeError("span must not exceed 6 bytes");
      }
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readUIntBE(offset, this.span);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeUIntBE(src, offset, this.span);
      return this.span;
    }
  }
  Layout$1.UIntBE = UIntBE;
  class Int extends Layout {
    constructor(span, property) {
      super(span, property);
      if (6 < this.span) {
        throw new RangeError("span must not exceed 6 bytes");
      }
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readIntLE(offset, this.span);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeIntLE(src, offset, this.span);
      return this.span;
    }
  }
  Layout$1.Int = Int;
  class IntBE extends Layout {
    constructor(span, property) {
      super(span, property);
      if (6 < this.span) {
        throw new RangeError("span must not exceed 6 bytes");
      }
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readIntBE(offset, this.span);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeIntBE(src, offset, this.span);
      return this.span;
    }
  }
  Layout$1.IntBE = IntBE;
  const V2E32 = Math.pow(2, 32);
  function divmodInt64(src) {
    const hi32 = Math.floor(src / V2E32);
    const lo32 = src - hi32 * V2E32;
    return $(function () {
      let result = $Object.create(null, undefined);
      result.hi32 = hi32;
      result.lo32 = lo32;
      return result;
    })();
  }
  $(divmodInt64);
  function roundedInt64(hi32, lo32) {
    return hi32 * V2E32 + lo32;
  }
  $(roundedInt64);
  class NearUInt64 extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      const buffer = uint8ArrayToBuffer(b);
      const lo32 = buffer.readUInt32LE(offset);
      const hi32 = buffer.readUInt32LE(offset + 4);
      return roundedInt64(hi32, lo32);
    }
    encode(src, b, offset = 0) {
      const split = divmodInt64(src);
      const buffer = uint8ArrayToBuffer(b);
      buffer.writeUInt32LE(split.lo32, offset);
      buffer.writeUInt32LE(split.hi32, offset + 4);
      return 8;
    }
  }
  Layout$1.NearUInt64 = NearUInt64;
  class NearUInt64BE extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      const buffer = uint8ArrayToBuffer(b);
      const hi32 = buffer.readUInt32BE(offset);
      const lo32 = buffer.readUInt32BE(offset + 4);
      return roundedInt64(hi32, lo32);
    }
    encode(src, b, offset = 0) {
      const split = divmodInt64(src);
      const buffer = uint8ArrayToBuffer(b);
      buffer.writeUInt32BE(split.hi32, offset);
      buffer.writeUInt32BE(split.lo32, offset + 4);
      return 8;
    }
  }
  Layout$1.NearUInt64BE = NearUInt64BE;
  class NearInt64 extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      const buffer = uint8ArrayToBuffer(b);
      const lo32 = buffer.readUInt32LE(offset);
      const hi32 = buffer.readInt32LE(offset + 4);
      return roundedInt64(hi32, lo32);
    }
    encode(src, b, offset = 0) {
      const split = divmodInt64(src);
      const buffer = uint8ArrayToBuffer(b);
      buffer.writeUInt32LE(split.lo32, offset);
      buffer.writeInt32LE(split.hi32, offset + 4);
      return 8;
    }
  }
  Layout$1.NearInt64 = NearInt64;
  class NearInt64BE extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      const buffer = uint8ArrayToBuffer(b);
      const hi32 = buffer.readInt32BE(offset);
      const lo32 = buffer.readUInt32BE(offset + 4);
      return roundedInt64(hi32, lo32);
    }
    encode(src, b, offset = 0) {
      const split = divmodInt64(src);
      const buffer = uint8ArrayToBuffer(b);
      buffer.writeInt32BE(split.hi32, offset);
      buffer.writeUInt32BE(split.lo32, offset + 4);
      return 8;
    }
  }
  Layout$1.NearInt64BE = NearInt64BE;
  class Float extends Layout {
    constructor(property) {
      super(4, property);
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readFloatLE(offset);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeFloatLE(src, offset);
      return 4;
    }
  }
  Layout$1.Float = Float;
  class FloatBE extends Layout {
    constructor(property) {
      super(4, property);
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readFloatBE(offset);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeFloatBE(src, offset);
      return 4;
    }
  }
  Layout$1.FloatBE = FloatBE;
  class Double extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readDoubleLE(offset);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeDoubleLE(src, offset);
      return 8;
    }
  }
  Layout$1.Double = Double;
  class DoubleBE extends Layout {
    constructor(property) {
      super(8, property);
    }
    decode(b, offset = 0) {
      return uint8ArrayToBuffer(b).readDoubleBE(offset);
    }
    encode(src, b, offset = 0) {
      uint8ArrayToBuffer(b).writeDoubleBE(src, offset);
      return 8;
    }
  }
  Layout$1.DoubleBE = DoubleBE;
  class Sequence extends Layout {
    constructor(elementLayout, count, property) {
      if (!(elementLayout instanceof Layout)) {
        throw new TypeError("elementLayout must be a Layout");
      }
      if (!(count instanceof ExternalLayout && count.isCount() || Number.isInteger(count) && 0 <= count)) {
        throw new TypeError("count must be non-negative integer " + "or an unsigned integer ExternalLayout");
      }
      let span = -1;
      if (!(count instanceof ExternalLayout) && 0 < elementLayout.span) {
        span = count * elementLayout.span;
      }
      super(span, property);
      this.elementLayout = elementLayout;
      this.count = count;
    }
    getSpan(b, offset = 0) {
      if (0 <= this.span) {
        return this.span;
      }
      let span = 0;
      let count = this.count;
      if (count instanceof ExternalLayout) {
        count = count.decode(b, offset);
      }
      if (0 < this.elementLayout.span) {
        span = count * this.elementLayout.span;
      } else {
        let idx = 0;
        while (idx < count) {
          span += this.elementLayout.getSpan(b, offset + span);
          ++idx;
        }
      }
      return span;
    }
    decode(b, offset = 0) {
      const rv = $Array.of();
      let i = 0;
      let count = this.count;
      if (count instanceof ExternalLayout) {
        count = count.decode(b, offset);
      }
      while (i < count) {
        rv.push(this.elementLayout.decode(b, offset));
        offset += this.elementLayout.getSpan(b, offset);
        i += 1;
      }
      return rv;
    }
    encode(src, b, offset = 0) {
      const elo = this.elementLayout;
      const span = src.reduce($((span, v) => span + elo.encode(v, b, offset + span)), 0);
      if (this.count instanceof ExternalLayout) {
        this.count.encode(src.length, b, offset);
      }
      return span;
    }
  }
  Layout$1.Sequence = Sequence;
  class Structure extends Layout {
    constructor(fields, property, decodePrefixes) {
      if (!($Array.isArray(fields) && fields.reduce($((acc, v) => acc && v instanceof Layout), true))) {
        throw new TypeError("fields must be array of Layout instances");
      }
      if ("boolean" === typeof property && undefined === decodePrefixes) {
        decodePrefixes = property;
        property = undefined;
      }
      for (const fd of fields) {
        if (0 > fd.span && undefined === fd.property) {
          throw new Error("fields cannot contain unnamed variable-length layout");
        }
      }
      let span = -1;
      try {
        span = fields.reduce($((span, fd) => span + fd.getSpan()), 0);
      } catch (e) {}
      super(span, property);
      this.fields = fields;
      this.decodePrefixes = !!decodePrefixes;
    }
    getSpan(b, offset = 0) {
      if (0 <= this.span) {
        return this.span;
      }
      let span = 0;
      try {
        span = this.fields.reduce($((span, fd) => {
          const fsp = fd.getSpan(b, offset);
          offset += fsp;
          return span + fsp;
        }), 0);
      } catch (e) {
        throw new RangeError("indeterminate span");
      }
      return span;
    }
    decode(b, offset = 0) {
      checkUint8Array(b);
      const dest = this.makeDestinationObject();
      for (const fd of this.fields) {
        if (undefined !== fd.property) {
          dest[fd.property] = fd.decode(b, offset);
        }
        offset += fd.getSpan(b, offset);
        if (this.decodePrefixes && b.length === offset) {
          break;
        }
      }
      return dest;
    }
    encode(src, b, offset = 0) {
      const firstOffset = offset;
      let lastOffset = 0;
      let lastWrote = 0;
      for (const fd of this.fields) {
        let span = fd.span;
        lastWrote = 0 < span ? span : 0;
        if (undefined !== fd.property) {
          const fv = src[fd.property];
          if (undefined !== fv) {
            lastWrote = fd.encode(fv, b, offset);
            if (0 > span) {
              span = fd.getSpan(b, offset);
            }
          }
        }
        lastOffset = offset;
        offset += span;
      }
      return lastOffset + lastWrote - firstOffset;
    }
    fromArray(values) {
      const dest = this.makeDestinationObject();
      for (const fd of this.fields) {
        if (undefined !== fd.property && 0 < values.length) {
          dest[fd.property] = values.shift();
        }
      }
      return dest;
    }
    layoutFor(property) {
      if ("string" !== typeof property) {
        throw new TypeError("property must be string");
      }
      for (const fd of this.fields) {
        if (fd.property === property) {
          return fd;
        }
      }
      return undefined;
    }
    offsetOf(property) {
      if ("string" !== typeof property) {
        throw new TypeError("property must be string");
      }
      let offset = 0;
      for (const fd of this.fields) {
        if (fd.property === property) {
          return offset;
        }
        if (0 > fd.span) {
          offset = -1;
        } else if (0 <= offset) {
          offset += fd.span;
        }
      }
      return undefined;
    }
  }
  Layout$1.Structure = Structure;
  class UnionDiscriminator {
    constructor(property) {
      this.property = property;
    }
    decode(b, offset) {
      throw new Error("UnionDiscriminator is abstract");
    }
    encode(src, b, offset) {
      throw new Error("UnionDiscriminator is abstract");
    }
  }
  Layout$1.UnionDiscriminator = UnionDiscriminator;
  class UnionLayoutDiscriminator extends UnionDiscriminator {
    constructor(layout, property) {
      if (!(layout instanceof ExternalLayout && layout.isCount())) {
        throw new TypeError("layout must be an unsigned integer ExternalLayout");
      }
      super(property || layout.property || "variant");
      this.layout = layout;
    }
    decode(b, offset) {
      return this.layout.decode(b, offset);
    }
    encode(src, b, offset) {
      return this.layout.encode(src, b, offset);
    }
  }
  Layout$1.UnionLayoutDiscriminator = UnionLayoutDiscriminator;
  class Union extends Layout {
    constructor(discr, defaultLayout, property) {
      let discriminator;
      if (discr instanceof UInt || discr instanceof UIntBE) {
        discriminator = new UnionLayoutDiscriminator(new OffsetLayout(discr));
      } else if (discr instanceof ExternalLayout && discr.isCount()) {
        discriminator = new UnionLayoutDiscriminator(discr);
      } else if (!(discr instanceof UnionDiscriminator)) {
        throw new TypeError("discr must be a UnionDiscriminator " + "or an unsigned integer layout");
      } else {
        discriminator = discr;
      }
      if (undefined === defaultLayout) {
        defaultLayout = null;
      }
      if (!(null === defaultLayout || defaultLayout instanceof Layout)) {
        throw new TypeError("defaultLayout must be null or a Layout");
      }
      if (null !== defaultLayout) {
        if (0 > defaultLayout.span) {
          throw new Error("defaultLayout must have constant span");
        }
        if (undefined === defaultLayout.property) {
          defaultLayout = defaultLayout.replicate("content");
        }
      }
      let span = -1;
      if (defaultLayout) {
        span = defaultLayout.span;
        if (0 <= span && (discr instanceof UInt || discr instanceof UIntBE)) {
          span += discriminator.layout.span;
        }
      }
      super(span, property);
      this.discriminator = discriminator;
      this.usesPrefixDiscriminator = discr instanceof UInt || discr instanceof UIntBE;
      this.defaultLayout = defaultLayout;
      this.registry = $Object.create(null, undefined);
      let boundGetSourceVariant = this.defaultGetSourceVariant.bind(this);
      this.getSourceVariant = $(function (src) {
        return boundGetSourceVariant(src);
      });
      this.configGetSourceVariant = $(function (gsv) {
        boundGetSourceVariant = gsv.bind(this);
      });
    }
    getSpan(b, offset = 0) {
      if (0 <= this.span) {
        return this.span;
      }
      const vlo = this.getVariant(b, offset);
      if (!vlo) {
        throw new Error("unable to determine span for unrecognized variant");
      }
      return vlo.getSpan(b, offset);
    }
    defaultGetSourceVariant(src) {
      if ($Object.prototype.hasOwnProperty.call(src, this.discriminator.property)) {
        if (this.defaultLayout && this.defaultLayout.property && $Object.prototype.hasOwnProperty.call(src, this.defaultLayout.property)) {
          return undefined;
        }
        const vlo = this.registry[src[this.discriminator.property]];
        if (vlo && (!vlo.layout || vlo.property && $Object.prototype.hasOwnProperty.call(src, vlo.property))) {
          return vlo;
        }
      } else {
        for (const tag in this.registry) {
          const vlo = this.registry[tag];
          if (vlo.property && $Object.prototype.hasOwnProperty.call(src, vlo.property)) {
            return vlo;
          }
        }
      }
      throw new Error("unable to infer src variant");
    }
    decode(b, offset = 0) {
      let dest;
      const dlo = this.discriminator;
      const discr = dlo.decode(b, offset);
      const clo = this.registry[discr];
      if (undefined === clo) {
        const defaultLayout = this.defaultLayout;
        let contentOffset = 0;
        if (this.usesPrefixDiscriminator) {
          contentOffset = dlo.layout.span;
        }
        dest = this.makeDestinationObject();
        dest[dlo.property] = discr;
        dest[defaultLayout.property] = defaultLayout.decode(b, offset + contentOffset);
      } else {
        dest = clo.decode(b, offset);
      }
      return dest;
    }
    encode(src, b, offset = 0) {
      const vlo = this.getSourceVariant(src);
      if (undefined === vlo) {
        const dlo = this.discriminator;
        const clo = this.defaultLayout;
        let contentOffset = 0;
        if (this.usesPrefixDiscriminator) {
          contentOffset = dlo.layout.span;
        }
        dlo.encode(src[dlo.property], b, offset);
        return contentOffset + clo.encode(src[clo.property], b, offset + contentOffset);
      }
      return vlo.encode(src, b, offset);
    }
    addVariant(variant, layout, property) {
      const rv = new VariantLayout(this, variant, layout, property);
      this.registry[variant] = rv;
      return rv;
    }
    getVariant(vb, offset = 0) {
      let variant;
      if (vb instanceof Uint8Array) {
        variant = this.discriminator.decode(vb, offset);
      } else {
        variant = vb;
      }
      return this.registry[variant];
    }
  }
  Layout$1.Union = Union;
  class VariantLayout extends Layout {
    constructor(union, variant, layout, property) {
      if (!(union instanceof Union)) {
        throw new TypeError("union must be a Union");
      }
      if (!Number.isInteger(variant) || 0 > variant) {
        throw new TypeError("variant must be a (non-negative) integer");
      }
      if ("string" === typeof layout && undefined === property) {
        property = layout;
        layout = null;
      }
      if (layout) {
        if (!(layout instanceof Layout)) {
          throw new TypeError("layout must be a Layout");
        }
        if (null !== union.defaultLayout && 0 <= layout.span && layout.span > union.defaultLayout.span) {
          throw new Error("variant span exceeds span of containing union");
        }
        if ("string" !== typeof property) {
          throw new TypeError("variant must have a String property");
        }
      }
      let span = union.span;
      if (0 > union.span) {
        span = layout ? layout.span : 0;
        if (0 <= span && union.usesPrefixDiscriminator) {
          span += union.discriminator.layout.span;
        }
      }
      super(span, property);
      this.union = union;
      this.variant = variant;
      this.layout = layout || null;
    }
    getSpan(b, offset = 0) {
      if (0 <= this.span) {
        return this.span;
      }
      let contentOffset = 0;
      if (this.union.usesPrefixDiscriminator) {
        contentOffset = this.union.discriminator.layout.span;
      }
      let span = 0;
      if (this.layout) {
        span = this.layout.getSpan(b, offset + contentOffset);
      }
      return contentOffset + span;
    }
    decode(b, offset = 0) {
      const dest = this.makeDestinationObject();
      if (this !== this.union.getVariant(b, offset)) {
        throw new Error("variant mismatch");
      }
      let contentOffset = 0;
      if (this.union.usesPrefixDiscriminator) {
        contentOffset = this.union.discriminator.layout.span;
      }
      if (this.layout) {
        dest[this.property] = this.layout.decode(b, offset + contentOffset);
      } else if (this.property) {
        dest[this.property] = true;
      } else if (this.union.usesPrefixDiscriminator) {
        dest[this.union.discriminator.property] = this.variant;
      }
      return dest;
    }
    encode(src, b, offset = 0) {
      let contentOffset = 0;
      if (this.union.usesPrefixDiscriminator) {
        contentOffset = this.union.discriminator.layout.span;
      }
      if (this.layout && !$Object.prototype.hasOwnProperty.call(src, this.property)) {
        throw new TypeError("variant lacks property " + this.property);
      }
      this.union.discriminator.encode(this.variant, b, offset);
      let span = contentOffset;
      if (this.layout) {
        this.layout.encode(src[this.property], b, offset + contentOffset);
        span += this.layout.getSpan(b, offset + contentOffset);
        if (0 <= this.union.span && span > this.union.span) {
          throw new Error("encoded variant overruns containing union");
        }
      }
      return span;
    }
    fromArray(values) {
      if (this.layout) {
        return this.layout.fromArray(values);
      }
      return undefined;
    }
  }
  Layout$1.VariantLayout = VariantLayout;
  function fixBitwiseResult(v) {
    if (0 > v) {
      v += 4294967296;
    }
    return v;
  }
  $(fixBitwiseResult);
  class BitStructure extends Layout {
    constructor(word, msb, property) {
      if (!(word instanceof UInt || word instanceof UIntBE)) {
        throw new TypeError("word must be a UInt or UIntBE layout");
      }
      if ("string" === typeof msb && undefined === property) {
        property = msb;
        msb = false;
      }
      if (4 < word.span) {
        throw new RangeError("word cannot exceed 32 bits");
      }
      super(word.span, property);
      this.word = word;
      this.msb = !!msb;
      this.fields = $Array.of();
      let value = 0;
      this._packedSetValue = $(function (v) {
        value = fixBitwiseResult(v);
        return this;
      });
      this._packedGetValue = $(function () {
        return value;
      });
    }
    decode(b, offset = 0) {
      const dest = this.makeDestinationObject();
      const value = this.word.decode(b, offset);
      this._packedSetValue(value);
      for (const fd of this.fields) {
        if (undefined !== fd.property) {
          dest[fd.property] = fd.decode(b);
        }
      }
      return dest;
    }
    encode(src, b, offset = 0) {
      const value = this.word.decode(b, offset);
      this._packedSetValue(value);
      for (const fd of this.fields) {
        if (undefined !== fd.property) {
          const fv = src[fd.property];
          if (undefined !== fv) {
            fd.encode(fv);
          }
        }
      }
      return this.word.encode(this._packedGetValue(), b, offset);
    }
    addField(bits, property) {
      const bf = new BitField(this, bits, property);
      this.fields.push(bf);
      return bf;
    }
    addBoolean(property) {
      const bf = new Boolean$1(this, property);
      this.fields.push(bf);
      return bf;
    }
    fieldFor(property) {
      if ("string" !== typeof property) {
        throw new TypeError("property must be string");
      }
      for (const fd of this.fields) {
        if (fd.property === property) {
          return fd;
        }
      }
      return undefined;
    }
  }
  Layout$1.BitStructure = BitStructure;
  class BitField {
    constructor(container, bits, property) {
      if (!(container instanceof BitStructure)) {
        throw new TypeError("container must be a BitStructure");
      }
      if (!Number.isInteger(bits) || 0 >= bits) {
        throw new TypeError("bits must be positive integer");
      }
      const totalBits = 8 * container.span;
      const usedBits = container.fields.reduce($((sum, fd) => sum + fd.bits), 0);
      if (bits + usedBits > totalBits) {
        throw new Error("bits too long for span remainder (" + (totalBits - usedBits) + " of " + totalBits + " remain)");
      }
      this.container = container;
      this.bits = bits;
      this.valueMask = (1 << bits) - 1;
      if (32 === bits) {
        this.valueMask = 4294967295;
      }
      this.start = usedBits;
      if (this.container.msb) {
        this.start = totalBits - usedBits - bits;
      }
      this.wordMask = fixBitwiseResult(this.valueMask << this.start);
      this.property = property;
    }
    decode(b, offset) {
      const word = this.container._packedGetValue();
      const wordValue = fixBitwiseResult(word & this.wordMask);
      const value = wordValue >>> this.start;
      return value;
    }
    encode(value) {
      if ("number" !== typeof value || !Number.isInteger(value) || value !== fixBitwiseResult(value & this.valueMask)) {
        throw new TypeError(nameWithProperty("BitField.encode", this) + " value must be integer not exceeding " + this.valueMask);
      }
      const word = this.container._packedGetValue();
      const wordValue = fixBitwiseResult(value << this.start);
      this.container._packedSetValue(fixBitwiseResult(word & ~this.wordMask) | wordValue);
    }
  }
  Layout$1.BitField = BitField;
  let Boolean$1 = class Boolean extends BitField {
    constructor(container, property) {
      super(container, 1, property);
    }
    decode(b, offset) {
      return !!super.decode(b, offset);
    }
    encode(value) {
      if ("boolean" === typeof value) {
        value = +value;
      }
      super.encode(value);
    }
  };
  Layout$1.Boolean = Boolean$1;
  class Blob extends Layout {
    constructor(length, property) {
      if (!(length instanceof ExternalLayout && length.isCount() || Number.isInteger(length) && 0 <= length)) {
        throw new TypeError("length must be positive integer " + "or an unsigned integer ExternalLayout");
      }
      let span = -1;
      if (!(length instanceof ExternalLayout)) {
        span = length;
      }
      super(span, property);
      this.length = length;
    }
    getSpan(b, offset) {
      let span = this.span;
      if (0 > span) {
        span = this.length.decode(b, offset);
      }
      return span;
    }
    decode(b, offset = 0) {
      let span = this.span;
      if (0 > span) {
        span = this.length.decode(b, offset);
      }
      return uint8ArrayToBuffer(b).slice(offset, offset + span);
    }
    encode(src, b, offset) {
      let span = this.length;
      if (this.length instanceof ExternalLayout) {
        span = src.length;
      }
      if (!(src instanceof Uint8Array && span === src.length)) {
        throw new TypeError(nameWithProperty("Blob.encode", this) + " requires (length " + span + ") Uint8Array as src");
      }
      if (offset + span > b.length) {
        throw new RangeError("encoding overruns Uint8Array");
      }
      const srcBuffer = uint8ArrayToBuffer(src);
      uint8ArrayToBuffer(b).write(srcBuffer.toString("hex"), offset, span, "hex");
      if (this.length instanceof ExternalLayout) {
        this.length.encode(span, b, offset);
      }
      return span;
    }
  }
  Layout$1.Blob = Blob;
  class CString extends Layout {
    constructor(property) {
      super(-1, property);
    }
    getSpan(b, offset = 0) {
      checkUint8Array(b);
      let idx = offset;
      while (idx < b.length && 0 !== b[idx]) {
        idx += 1;
      }
      return 1 + idx - offset;
    }
    decode(b, offset = 0) {
      const span = this.getSpan(b, offset);
      return uint8ArrayToBuffer(b).slice(offset, offset + span - 1).toString("utf-8");
    }
    encode(src, b, offset = 0) {
      if ("string" !== typeof src) {
        src = String(src);
      }
      const srcb = buffer_1.Buffer.from(src, "utf8");
      const span = srcb.length;
      if (offset + span > b.length) {
        throw new RangeError("encoding overruns Buffer");
      }
      const buffer = uint8ArrayToBuffer(b);
      srcb.copy(buffer, offset);
      buffer[offset + span] = 0;
      return span + 1;
    }
  }
  Layout$1.CString = CString;
  class UTF8 extends Layout {
    constructor(maxSpan, property) {
      if ("string" === typeof maxSpan && undefined === property) {
        property = maxSpan;
        maxSpan = undefined;
      }
      if (undefined === maxSpan) {
        maxSpan = -1;
      } else if (!Number.isInteger(maxSpan)) {
        throw new TypeError("maxSpan must be an integer");
      }
      super(-1, property);
      this.maxSpan = maxSpan;
    }
    getSpan(b, offset = 0) {
      checkUint8Array(b);
      return b.length - offset;
    }
    decode(b, offset = 0) {
      const span = this.getSpan(b, offset);
      if (0 <= this.maxSpan && this.maxSpan < span) {
        throw new RangeError("text length exceeds maxSpan");
      }
      return uint8ArrayToBuffer(b).slice(offset, offset + span).toString("utf-8");
    }
    encode(src, b, offset = 0) {
      if ("string" !== typeof src) {
        src = String(src);
      }
      const srcb = buffer_1.Buffer.from(src, "utf8");
      const span = srcb.length;
      if (0 <= this.maxSpan && this.maxSpan < span) {
        throw new RangeError("text length exceeds maxSpan");
      }
      if (offset + span > b.length) {
        throw new RangeError("encoding overruns Buffer");
      }
      srcb.copy(uint8ArrayToBuffer(b), offset);
      return span;
    }
  }
  Layout$1.UTF8 = UTF8;
  class Constant extends Layout {
    constructor(value, property) {
      super(0, property);
      this.value = value;
    }
    decode(b, offset) {
      return this.value;
    }
    encode(src, b, offset) {
      return 0;
    }
  }
  Layout$1.Constant = Constant;
  Layout$1.greedy = $((elementSpan, property) => new GreedyCount(elementSpan, property));
  var offset = Layout$1.offset = $((layout, offset, property) => new OffsetLayout(layout, offset, property));
  var u8 = Layout$1.u8 = $(property => new UInt(1, property));
  var u16 = Layout$1.u16 = $(property => new UInt(2, property));
  Layout$1.u24 = $(property => new UInt(3, property));
  var u32 = Layout$1.u32 = $(property => new UInt(4, property));
  Layout$1.u40 = $(property => new UInt(5, property));
  Layout$1.u48 = $(property => new UInt(6, property));
  var nu64 = Layout$1.nu64 = $(property => new NearUInt64(property));
  Layout$1.u16be = $(property => new UIntBE(2, property));
  Layout$1.u24be = $(property => new UIntBE(3, property));
  Layout$1.u32be = $(property => new UIntBE(4, property));
  Layout$1.u40be = $(property => new UIntBE(5, property));
  Layout$1.u48be = $(property => new UIntBE(6, property));
  Layout$1.nu64be = $(property => new NearUInt64BE(property));
  Layout$1.s8 = $(property => new Int(1, property));
  Layout$1.s16 = $(property => new Int(2, property));
  Layout$1.s24 = $(property => new Int(3, property));
  Layout$1.s32 = $(property => new Int(4, property));
  Layout$1.s40 = $(property => new Int(5, property));
  Layout$1.s48 = $(property => new Int(6, property));
  var ns64 = Layout$1.ns64 = $(property => new NearInt64(property));
  Layout$1.s16be = $(property => new IntBE(2, property));
  Layout$1.s24be = $(property => new IntBE(3, property));
  Layout$1.s32be = $(property => new IntBE(4, property));
  Layout$1.s40be = $(property => new IntBE(5, property));
  Layout$1.s48be = $(property => new IntBE(6, property));
  Layout$1.ns64be = $(property => new NearInt64BE(property));
  Layout$1.f32 = $(property => new Float(property));
  Layout$1.f32be = $(property => new FloatBE(property));
  Layout$1.f64 = $(property => new Double(property));
  Layout$1.f64be = $(property => new DoubleBE(property));
  var struct = Layout$1.struct = $((fields, property, decodePrefixes) => new Structure(fields, property, decodePrefixes));
  Layout$1.bits = $((word, msb, property) => new BitStructure(word, msb, property));
  var seq = Layout$1.seq = $((elementLayout, count, property) => new Sequence(elementLayout, count, property));
  Layout$1.union = $((discr, defaultLayout, property) => new Union(discr, defaultLayout, property));
  Layout$1.unionLayoutDiscriminator = $((layout, property) => new UnionLayoutDiscriminator(layout, property));
  var blob = Layout$1.blob = $((length, property) => new Blob(length, property));
  Layout$1.cstr = $(property => new CString(property));
  Layout$1.utf8 = $((maxSpan, property) => new UTF8(maxSpan, property));
  Layout$1.constant = $((value, property) => new Constant(value, property));
  const PACKET_DATA_SIZE = 1280 - 40 - 8;
  const VERSION_PREFIX_MASK = 127;
  const SIGNATURE_LENGTH_IN_BYTES = 64;
  class TransactionExpiredBlockheightExceededError extends Error {
    constructor(signature) {
      super(`Signature ${signature} has expired: block height exceeded.`);
      this.signature = void 0;
      this.signature = signature;
    }
  }
  $Object.defineProperty(TransactionExpiredBlockheightExceededError.prototype, "name", $(function () {
    let result = $Object.create(null, undefined);
    result.value = "TransactionExpiredBlockheightExceededError";
    return result;
  })());
  class TransactionExpiredTimeoutError extends Error {
    constructor(signature, timeoutSeconds) {
      super(`Transaction was not confirmed in ${timeoutSeconds.toFixed(2)} seconds. It is ` + "unknown if it succeeded or failed. Check signature " + `${signature} using the Solana Explorer or CLI tools.`);
      this.signature = void 0;
      this.signature = signature;
    }
  }
  $Object.defineProperty(TransactionExpiredTimeoutError.prototype, "name", $(function () {
    let result = $Object.create(null, undefined);
    result.value = "TransactionExpiredTimeoutError";
    return result;
  })());
  class TransactionExpiredNonceInvalidError extends Error {
    constructor(signature) {
      super(`Signature ${signature} has expired: the nonce is no longer valid.`);
      this.signature = void 0;
      this.signature = signature;
    }
  }
  $Object.defineProperty(TransactionExpiredNonceInvalidError.prototype, "name", $(function () {
    let result = $Object.create(null, undefined);
    result.value = "TransactionExpiredNonceInvalidError";
    return result;
  })());
  class MessageAccountKeys {
    constructor(staticAccountKeys, accountKeysFromLookups) {
      this.staticAccountKeys = void 0;
      this.accountKeysFromLookups = void 0;
      this.staticAccountKeys = staticAccountKeys;
      this.accountKeysFromLookups = accountKeysFromLookups;
    }
    keySegments() {
      const keySegments = $Array.of(this.staticAccountKeys);
      if (this.accountKeysFromLookups) {
        keySegments.push(this.accountKeysFromLookups.writable);
        keySegments.push(this.accountKeysFromLookups.readonly);
      }
      return keySegments;
    }
    get(index) {
      for (const keySegment of this.keySegments()) {
        if (index < keySegment.length) {
          return keySegment[index];
        } else {
          index -= keySegment.length;
        }
      }
      return;
    }
    get length() {
      return this.keySegments().flat().length;
    }
    compileInstructions(instructions) {
      const U8_MAX = 255;
      if (this.length > U8_MAX + 1) {
        throw new Error("Account index overflow encountered during compilation");
      }
      const keyIndexMap = new Map();
      this.keySegments().flat().forEach($((key, index) => {
        keyIndexMap.set(key.toBase58(), index);
      }));
      const findKeyIndex = key => {
        const keyIndex = keyIndexMap.get(key.toBase58());
        if (keyIndex === undefined) throw new Error("Encountered an unknown instruction account key during compilation");
        return keyIndex;
      };
      $(findKeyIndex);
      return instructions.map($(instruction => $(function () {
        let result = $Object.create(null, undefined);
        result.programIdIndex = findKeyIndex(instruction.programId);
        result.accountKeyIndexes = instruction.keys.map($(meta => findKeyIndex(meta.pubkey)));
        result.data = instruction.data;
        return result;
      })()));
    }
  }
  const publicKey = (property = "publicKey") => blob(32, property);
  $(publicKey);
  const signature = (property = "signature") => blob(64, property);
  $(signature);
  const rustString = (property = "string") => {
    const rsl = struct($Array.of(u32("length"), u32("lengthPadding"), blob(offset(u32(), -8), "chars")), property);
    const _decode = rsl.decode.bind(rsl);
    const _encode = rsl.encode.bind(rsl);
    const rslShim = rsl;
    rslShim.decode = $((b, offset) => {
      const data = _decode(b, offset);
      return data["chars"].toString();
    });
    rslShim.encode = $((str, b, offset) => {
      const data = $(function () {
        let result = $Object.create(null, undefined);
        result.chars = buffer.Buffer.from(str, "utf8");
        return result;
      })();
      return _encode(data, b, offset);
    });
    rslShim.alloc = $(str => u32().span + u32().span + buffer.Buffer.from(str, "utf8").length);
    return rslShim;
  };
  $(rustString);
  const authorized = (property = "authorized") => struct($Array.of(publicKey("staker"), publicKey("withdrawer")), property);
  $(authorized);
  const lockup = (property = "lockup") => struct($Array.of(ns64("unixTimestamp"), ns64("epoch"), publicKey("custodian")), property);
  $(lockup);
  const voteInit = (property = "voteInit") => struct($Array.of(publicKey("nodePubkey"), publicKey("authorizedVoter"), publicKey("authorizedWithdrawer"), u8("commission")), property);
  $(voteInit);
  const voteAuthorizeWithSeedArgs = (property = "voteAuthorizeWithSeedArgs") => struct($Array.of(u32("voteAuthorizationType"), publicKey("currentAuthorityDerivedKeyOwnerPubkey"), rustString("currentAuthorityDerivedKeySeed"), publicKey("newAuthorized")), property);
  $(voteAuthorizeWithSeedArgs);
  function getAlloc(type, fields) {
    const getItemAlloc = item => {
      if (item.span >= 0) {
        return item.span;
      } else if (typeof item.alloc === "function") {
        return item.alloc(fields[item.property]);
      } else if ("count" in item && "elementLayout" in item) {
        const field = fields[item.property];
        if ($Array.isArray(field)) {
          return field.length * getItemAlloc(item.elementLayout);
        }
      } else if ("fields" in item) {
        return getAlloc($(function () {
          let result = $Object.create(null, undefined);
          result.layout = item;
          return result;
        })(), fields[item.property]);
      }
      return 0;
    };
    $(getItemAlloc);
    let alloc = 0;
    type.layout.fields.forEach($(item => {
      alloc += getItemAlloc(item);
    }));
    return alloc;
  }
  $(getAlloc);
  function decodeLength(bytes) {
    let len = 0;
    let size = 0;
    for (;;) {
      let elem = bytes.shift();
      len |= (elem & 127) << size * 7;
      size += 1;
      if ((elem & 128) === 0) {
        break;
      }
    }
    return len;
  }
  $(decodeLength);
  function encodeLength(bytes, len) {
    let rem_len = len;
    for (;;) {
      let elem = rem_len & 127;
      rem_len >>= 7;
      if (rem_len == 0) {
        bytes.push(elem);
        break;
      } else {
        elem |= 128;
        bytes.push(elem);
      }
    }
  }
  $(encodeLength);
  function assert$1(condition, message) {
    if (!condition) {
      throw new Error(message || "Assertion failed");
    }
  }
  $(assert$1);
  class CompiledKeys {
    constructor(payer, keyMetaMap) {
      this.payer = void 0;
      this.keyMetaMap = void 0;
      this.payer = payer;
      this.keyMetaMap = keyMetaMap;
    }
    static compile(instructions, payer) {
      const keyMetaMap = new Map();
      const getOrInsertDefault = pubkey => {
        const address = pubkey.toBase58();
        let keyMeta = keyMetaMap.get(address);
        if (keyMeta === undefined) {
          keyMeta = $(function () {
            let result = $Object.create(null, undefined);
            result.isSigner = false;
            result.isWritable = false;
            result.isInvoked = false;
            return result;
          })();
          keyMetaMap.set(address, keyMeta);
        }
        return keyMeta;
      };
      $(getOrInsertDefault);
      const payerKeyMeta = getOrInsertDefault(payer);
      payerKeyMeta.isSigner = true;
      payerKeyMeta.isWritable = true;
      for (const ix of instructions) {
        getOrInsertDefault(ix.programId).isInvoked = true;
        for (const accountMeta of ix.keys) {
          const keyMeta = getOrInsertDefault(accountMeta.pubkey);
          keyMeta.isSigner ||= accountMeta.isSigner;
          keyMeta.isWritable ||= accountMeta.isWritable;
        }
      }
      return new CompiledKeys(payer, keyMetaMap);
    }
    getMessageComponents() {
      const mapEntries = $Array.of(...this.keyMetaMap.entries());
      assert$1(mapEntries.length <= 256, "Max static account keys length exceeded");
      const writableSigners = mapEntries.filter($(([, meta]) => meta.isSigner && meta.isWritable));
      const readonlySigners = mapEntries.filter($(([, meta]) => meta.isSigner && !meta.isWritable));
      const writableNonSigners = mapEntries.filter($(([, meta]) => !meta.isSigner && meta.isWritable));
      const readonlyNonSigners = mapEntries.filter($(([, meta]) => !meta.isSigner && !meta.isWritable));
      const header = $(function () {
        let result = $Object.create(null, undefined);
        result.numRequiredSignatures = writableSigners.length + readonlySigners.length;
        result.numReadonlySignedAccounts = readonlySigners.length;
        result.numReadonlyUnsignedAccounts = readonlyNonSigners.length;
        return result;
      })();
      {
        assert$1(writableSigners.length > 0, "Expected at least one writable signer key");
        const [payerAddress] = writableSigners[0];
        assert$1(payerAddress === this.payer.toBase58(), "Expected first writable signer key to be the fee payer");
      }
      const staticAccountKeys = $Array.of(...writableSigners.map($(([address]) => new PublicKey(address))), ...readonlySigners.map($(([address]) => new PublicKey(address))), ...writableNonSigners.map($(([address]) => new PublicKey(address))), ...readonlyNonSigners.map($(([address]) => new PublicKey(address))));
      return $Array.of(header, staticAccountKeys);
    }
    extractTableLookup(lookupTable) {
      const [writableIndexes, drainedWritableKeys] = this.drainKeysFoundInLookupTable(lookupTable.state.addresses, $(keyMeta => !keyMeta.isSigner && !keyMeta.isInvoked && keyMeta.isWritable));
      const [readonlyIndexes, drainedReadonlyKeys] = this.drainKeysFoundInLookupTable(lookupTable.state.addresses, $(keyMeta => !keyMeta.isSigner && !keyMeta.isInvoked && !keyMeta.isWritable));
      if (writableIndexes.length === 0 && readonlyIndexes.length === 0) {
        return;
      }
      return $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.accountKey = lookupTable.key;
        result.writableIndexes = writableIndexes;
        result.readonlyIndexes = readonlyIndexes;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.writable = drainedWritableKeys;
        result.readonly = drainedReadonlyKeys;
        return result;
      })());
    }
    drainKeysFoundInLookupTable(lookupTableEntries, keyMetaFilter) {
      const lookupTableIndexes = new Array();
      const drainedKeys = new Array();
      for (const [address, keyMeta] of this.keyMetaMap.entries()) {
        if (keyMetaFilter(keyMeta)) {
          const key = new PublicKey(address);
          const lookupTableIndex = lookupTableEntries.findIndex($(entry => entry.equals(key)));
          if (lookupTableIndex >= 0) {
            assert$1(lookupTableIndex < 256, "Max lookup table index exceeded");
            lookupTableIndexes.push(lookupTableIndex);
            drainedKeys.push(key);
            this.keyMetaMap.delete(address);
          }
        }
      }
      return $Array.of(lookupTableIndexes, drainedKeys);
    }
  }
  const END_OF_BUFFER_ERROR_MESSAGE = "Reached end of buffer unexpectedly";
  function guardedShift(byteArray) {
    if (byteArray.length === 0) {
      throw new Error(END_OF_BUFFER_ERROR_MESSAGE);
    }
    return byteArray.shift();
  }
  $(guardedShift);
  function guardedSplice(byteArray, ...args) {
    const [start] = args;
    if (args.length === 2 ? start + (args[1] ?? 0) > byteArray.length : start >= byteArray.length) {
      throw new Error(END_OF_BUFFER_ERROR_MESSAGE);
    }
    return byteArray.splice(...args);
  }
  $(guardedSplice);
  class Message {
    constructor(args) {
      this.header = void 0;
      this.accountKeys = void 0;
      this.recentBlockhash = void 0;
      this.instructions = void 0;
      this.indexToProgramIds = new Map();
      this.header = args.header;
      this.accountKeys = args.accountKeys.map($(account => new PublicKey(account)));
      this.recentBlockhash = args.recentBlockhash;
      this.instructions = args.instructions;
      this.instructions.forEach($(ix => this.indexToProgramIds.set(ix.programIdIndex, this.accountKeys[ix.programIdIndex])));
    }
    get version() {
      return "legacy";
    }
    get staticAccountKeys() {
      return this.accountKeys;
    }
    get compiledInstructions() {
      return this.instructions.map($(ix => $(function () {
        let result = $Object.create(null, undefined);
        result.programIdIndex = ix.programIdIndex;
        result.accountKeyIndexes = ix.accounts;
        result.data = bs58$1.decode(ix.data);
        return result;
      })()));
    }
    get addressTableLookups() {
      return $Array.of();
    }
    getAccountKeys() {
      return new MessageAccountKeys(this.staticAccountKeys);
    }
    static compile(args) {
      const compiledKeys = CompiledKeys.compile(args.instructions, args.payerKey);
      const [header, staticAccountKeys] = compiledKeys.getMessageComponents();
      const accountKeys = new MessageAccountKeys(staticAccountKeys);
      const instructions = accountKeys.compileInstructions(args.instructions).map($(ix => $(function () {
        let result = $Object.create(null, undefined);
        result.programIdIndex = ix.programIdIndex;
        result.accounts = ix.accountKeyIndexes;
        result.data = bs58$1.encode(ix.data);
        return result;
      })()));
      return new Message($(function () {
        let result = $Object.create(null, undefined);
        result.header = header;
        result.accountKeys = staticAccountKeys;
        result.recentBlockhash = args.recentBlockhash;
        result.instructions = instructions;
        return result;
      })());
    }
    isAccountSigner(index) {
      return index < this.header.numRequiredSignatures;
    }
    isAccountWritable(index) {
      const numSignedAccounts = this.header.numRequiredSignatures;
      if (index >= this.header.numRequiredSignatures) {
        const unsignedAccountIndex = index - numSignedAccounts;
        const numUnsignedAccounts = this.accountKeys.length - numSignedAccounts;
        const numWritableUnsignedAccounts = numUnsignedAccounts - this.header.numReadonlyUnsignedAccounts;
        return unsignedAccountIndex < numWritableUnsignedAccounts;
      } else {
        const numWritableSignedAccounts = numSignedAccounts - this.header.numReadonlySignedAccounts;
        return index < numWritableSignedAccounts;
      }
    }
    isProgramId(index) {
      return this.indexToProgramIds.has(index);
    }
    programIds() {
      return $Array.of(...this.indexToProgramIds.values());
    }
    nonProgramIds() {
      return this.accountKeys.filter($((_, index) => !this.isProgramId(index)));
    }
    serialize() {
      const numKeys = this.accountKeys.length;
      let keyCount = $Array.of();
      encodeLength(keyCount, numKeys);
      const instructions = this.instructions.map($(instruction => {
        const {
          accounts: accounts,
          programIdIndex: programIdIndex
        } = instruction;
        const data = $Array.from(bs58$1.decode(instruction.data));
        let keyIndicesCount = $Array.of();
        encodeLength(keyIndicesCount, accounts.length);
        let dataCount = $Array.of();
        encodeLength(dataCount, data.length);
        return $(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = programIdIndex;
          result.keyIndicesCount = buffer.Buffer.from(keyIndicesCount);
          result.keyIndices = accounts;
          result.dataLength = buffer.Buffer.from(dataCount);
          result.data = data;
          return result;
        })();
      }));
      let instructionCount = $Array.of();
      encodeLength(instructionCount, instructions.length);
      let instructionBuffer = buffer.Buffer.alloc(PACKET_DATA_SIZE);
      buffer.Buffer.from(instructionCount).copy(instructionBuffer);
      let instructionBufferLength = instructionCount.length;
      instructions.forEach($(instruction => {
        const instructionLayout = struct($Array.of(u8("programIdIndex"), blob(instruction.keyIndicesCount.length, "keyIndicesCount"), seq(u8("keyIndex"), instruction.keyIndices.length, "keyIndices"), blob(instruction.dataLength.length, "dataLength"), seq(u8("userdatum"), instruction.data.length, "data")));
        const length = instructionLayout.encode(instruction, instructionBuffer, instructionBufferLength);
        instructionBufferLength += length;
      }));
      instructionBuffer = instructionBuffer.slice(0, instructionBufferLength);
      const signDataLayout = struct($Array.of(blob(1, "numRequiredSignatures"), blob(1, "numReadonlySignedAccounts"), blob(1, "numReadonlyUnsignedAccounts"), blob(keyCount.length, "keyCount"), seq(publicKey("key"), numKeys, "keys"), publicKey("recentBlockhash")));
      const transaction = $(function () {
        let result = $Object.create(null, undefined);
        result.numRequiredSignatures = buffer.Buffer.from($Array.of(this.header.numRequiredSignatures));
        result.numReadonlySignedAccounts = buffer.Buffer.from($Array.of(this.header.numReadonlySignedAccounts));
        result.numReadonlyUnsignedAccounts = buffer.Buffer.from($Array.of(this.header.numReadonlyUnsignedAccounts));
        result.keyCount = buffer.Buffer.from(keyCount);
        result.keys = this.accountKeys.map($(key => toBuffer(key.toBytes())));
        result.recentBlockhash = bs58$1.decode(this.recentBlockhash);
        return result;
      }).bind(this)();
      let signData = buffer.Buffer.alloc(2048);
      const length = signDataLayout.encode(transaction, signData);
      instructionBuffer.copy(signData, length);
      return signData.slice(0, length + instructionBuffer.length);
    }
    static from(buffer$1) {
      let byteArray = $Array.of(...buffer$1);
      const numRequiredSignatures = guardedShift(byteArray);
      if (numRequiredSignatures !== (numRequiredSignatures & VERSION_PREFIX_MASK)) {
        throw new Error("Versioned messages must be deserialized with VersionedMessage.deserialize()");
      }
      const numReadonlySignedAccounts = guardedShift(byteArray);
      const numReadonlyUnsignedAccounts = guardedShift(byteArray);
      const accountCount = decodeLength(byteArray);
      let accountKeys = $Array.of();
      for (let i = 0; i < accountCount; i++) {
        const account = guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH);
        accountKeys.push(new PublicKey(buffer.Buffer.from(account)));
      }
      const recentBlockhash = guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH);
      const instructionCount = decodeLength(byteArray);
      let instructions = $Array.of();
      for (let i = 0; i < instructionCount; i++) {
        const programIdIndex = guardedShift(byteArray);
        const accountCount = decodeLength(byteArray);
        const accounts = guardedSplice(byteArray, 0, accountCount);
        const dataLength = decodeLength(byteArray);
        const dataSlice = guardedSplice(byteArray, 0, dataLength);
        const data = bs58$1.encode(buffer.Buffer.from(dataSlice));
        instructions.push($(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = programIdIndex;
          result.accounts = accounts;
          result.data = data;
          return result;
        })());
      }
      const messageArgs = $(function () {
        let result = $Object.create(null, undefined);
        result.header = $(function () {
          let result = $Object.create(null, undefined);
          result.numRequiredSignatures = numRequiredSignatures;
          result.numReadonlySignedAccounts = numReadonlySignedAccounts;
          result.numReadonlyUnsignedAccounts = numReadonlyUnsignedAccounts;
          return result;
        })();
        result.recentBlockhash = bs58$1.encode(buffer.Buffer.from(recentBlockhash));
        result.accountKeys = accountKeys;
        result.instructions = instructions;
        return result;
      })();
      return new Message(messageArgs);
    }
  }
  class MessageV0 {
    constructor(args) {
      this.header = void 0;
      this.staticAccountKeys = void 0;
      this.recentBlockhash = void 0;
      this.compiledInstructions = void 0;
      this.addressTableLookups = void 0;
      this.header = args.header;
      this.staticAccountKeys = args.staticAccountKeys;
      this.recentBlockhash = args.recentBlockhash;
      this.compiledInstructions = args.compiledInstructions;
      this.addressTableLookups = args.addressTableLookups;
    }
    get version() {
      return 0;
    }
    get numAccountKeysFromLookups() {
      let count = 0;
      for (const lookup of this.addressTableLookups) {
        count += lookup.readonlyIndexes.length + lookup.writableIndexes.length;
      }
      return count;
    }
    getAccountKeys(args) {
      let accountKeysFromLookups;
      if (args && "accountKeysFromLookups" in args && args.accountKeysFromLookups) {
        if (this.numAccountKeysFromLookups != args.accountKeysFromLookups.writable.length + args.accountKeysFromLookups.readonly.length) {
          throw new Error("Failed to get account keys because of a mismatch in the number of account keys from lookups");
        }
        accountKeysFromLookups = args.accountKeysFromLookups;
      } else if (args && "addressLookupTableAccounts" in args && args.addressLookupTableAccounts) {
        accountKeysFromLookups = this.resolveAddressTableLookups(args.addressLookupTableAccounts);
      } else if (this.addressTableLookups.length > 0) {
        throw new Error("Failed to get account keys because address table lookups were not resolved");
      }
      return new MessageAccountKeys(this.staticAccountKeys, accountKeysFromLookups);
    }
    isAccountSigner(index) {
      return index < this.header.numRequiredSignatures;
    }
    isAccountWritable(index) {
      const numSignedAccounts = this.header.numRequiredSignatures;
      const numStaticAccountKeys = this.staticAccountKeys.length;
      if (index >= numStaticAccountKeys) {
        const lookupAccountKeysIndex = index - numStaticAccountKeys;
        const numWritableLookupAccountKeys = this.addressTableLookups.reduce($((count, lookup) => count + lookup.writableIndexes.length), 0);
        return lookupAccountKeysIndex < numWritableLookupAccountKeys;
      } else if (index >= this.header.numRequiredSignatures) {
        const unsignedAccountIndex = index - numSignedAccounts;
        const numUnsignedAccounts = numStaticAccountKeys - numSignedAccounts;
        const numWritableUnsignedAccounts = numUnsignedAccounts - this.header.numReadonlyUnsignedAccounts;
        return unsignedAccountIndex < numWritableUnsignedAccounts;
      } else {
        const numWritableSignedAccounts = numSignedAccounts - this.header.numReadonlySignedAccounts;
        return index < numWritableSignedAccounts;
      }
    }
    resolveAddressTableLookups(addressLookupTableAccounts) {
      const accountKeysFromLookups = $(function () {
        let result = $Object.create(null, undefined);
        result.writable = $Array.of();
        result.readonly = $Array.of();
        return result;
      })();
      for (const tableLookup of this.addressTableLookups) {
        const tableAccount = addressLookupTableAccounts.find($(account => account.key.equals(tableLookup.accountKey)));
        if (!tableAccount) {
          throw new Error(`Failed to find address lookup table account for table key ${tableLookup.accountKey.toBase58()}`);
        }
        for (const index of tableLookup.writableIndexes) {
          if (index < tableAccount.state.addresses.length) {
            accountKeysFromLookups.writable.push(tableAccount.state.addresses[index]);
          } else {
            throw new Error(`Failed to find address for index ${index} in address lookup table ${tableLookup.accountKey.toBase58()}`);
          }
        }
        for (const index of tableLookup.readonlyIndexes) {
          if (index < tableAccount.state.addresses.length) {
            accountKeysFromLookups.readonly.push(tableAccount.state.addresses[index]);
          } else {
            throw new Error(`Failed to find address for index ${index} in address lookup table ${tableLookup.accountKey.toBase58()}`);
          }
        }
      }
      return accountKeysFromLookups;
    }
    static compile(args) {
      const compiledKeys = CompiledKeys.compile(args.instructions, args.payerKey);
      const addressTableLookups = new Array();
      const accountKeysFromLookups = $(function () {
        let result = $Object.create(null, undefined);
        result.writable = new Array();
        result.readonly = new Array();
        return result;
      })();
      const lookupTableAccounts = args.addressLookupTableAccounts || $Array.of();
      for (const lookupTable of lookupTableAccounts) {
        const extractResult = compiledKeys.extractTableLookup(lookupTable);
        if (extractResult !== undefined) {
          const [addressTableLookup, {
            writable: writable,
            readonly: readonly
          }] = extractResult;
          addressTableLookups.push(addressTableLookup);
          accountKeysFromLookups.writable.push(...writable);
          accountKeysFromLookups.readonly.push(...readonly);
        }
      }
      const [header, staticAccountKeys] = compiledKeys.getMessageComponents();
      const accountKeys = new MessageAccountKeys(staticAccountKeys, accountKeysFromLookups);
      const compiledInstructions = accountKeys.compileInstructions(args.instructions);
      return new MessageV0($(function () {
        let result = $Object.create(null, undefined);
        result.header = header;
        result.staticAccountKeys = staticAccountKeys;
        result.recentBlockhash = args.recentBlockhash;
        result.compiledInstructions = compiledInstructions;
        result.addressTableLookups = addressTableLookups;
        return result;
      })());
    }
    serialize() {
      const encodedStaticAccountKeysLength = Array();
      encodeLength(encodedStaticAccountKeysLength, this.staticAccountKeys.length);
      const serializedInstructions = this.serializeInstructions();
      const encodedInstructionsLength = Array();
      encodeLength(encodedInstructionsLength, this.compiledInstructions.length);
      const serializedAddressTableLookups = this.serializeAddressTableLookups();
      const encodedAddressTableLookupsLength = Array();
      encodeLength(encodedAddressTableLookupsLength, this.addressTableLookups.length);
      const messageLayout = struct($Array.of(u8("prefix"), struct($Array.of(u8("numRequiredSignatures"), u8("numReadonlySignedAccounts"), u8("numReadonlyUnsignedAccounts")), "header"), blob(encodedStaticAccountKeysLength.length, "staticAccountKeysLength"), seq(publicKey(), this.staticAccountKeys.length, "staticAccountKeys"), publicKey("recentBlockhash"), blob(encodedInstructionsLength.length, "instructionsLength"), blob(serializedInstructions.length, "serializedInstructions"), blob(encodedAddressTableLookupsLength.length, "addressTableLookupsLength"), blob(serializedAddressTableLookups.length, "serializedAddressTableLookups")));
      const serializedMessage = new Uint8Array(PACKET_DATA_SIZE);
      const MESSAGE_VERSION_0_PREFIX = 1 << 7;
      const serializedMessageLength = messageLayout.encode($(function () {
        let result = $Object.create(null, undefined);
        result.prefix = MESSAGE_VERSION_0_PREFIX;
        result.header = this.header;
        result.staticAccountKeysLength = new Uint8Array(encodedStaticAccountKeysLength);
        result.staticAccountKeys = this.staticAccountKeys.map($(key => key.toBytes()));
        result.recentBlockhash = bs58$1.decode(this.recentBlockhash);
        result.instructionsLength = new Uint8Array(encodedInstructionsLength);
        result.serializedInstructions = serializedInstructions;
        result.addressTableLookupsLength = new Uint8Array(encodedAddressTableLookupsLength);
        result.serializedAddressTableLookups = serializedAddressTableLookups;
        return result;
      }).bind(this)(), serializedMessage);
      return serializedMessage.slice(0, serializedMessageLength);
    }
    serializeInstructions() {
      let serializedLength = 0;
      const serializedInstructions = new Uint8Array(PACKET_DATA_SIZE);
      for (const instruction of this.compiledInstructions) {
        const encodedAccountKeyIndexesLength = Array();
        encodeLength(encodedAccountKeyIndexesLength, instruction.accountKeyIndexes.length);
        const encodedDataLength = Array();
        encodeLength(encodedDataLength, instruction.data.length);
        const instructionLayout = struct($Array.of(u8("programIdIndex"), blob(encodedAccountKeyIndexesLength.length, "encodedAccountKeyIndexesLength"), seq(u8(), instruction.accountKeyIndexes.length, "accountKeyIndexes"), blob(encodedDataLength.length, "encodedDataLength"), blob(instruction.data.length, "data")));
        serializedLength += instructionLayout.encode($(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = instruction.programIdIndex;
          result.encodedAccountKeyIndexesLength = new Uint8Array(encodedAccountKeyIndexesLength);
          result.accountKeyIndexes = instruction.accountKeyIndexes;
          result.encodedDataLength = new Uint8Array(encodedDataLength);
          result.data = instruction.data;
          return result;
        })(), serializedInstructions, serializedLength);
      }
      return serializedInstructions.slice(0, serializedLength);
    }
    serializeAddressTableLookups() {
      let serializedLength = 0;
      const serializedAddressTableLookups = new Uint8Array(PACKET_DATA_SIZE);
      for (const lookup of this.addressTableLookups) {
        const encodedWritableIndexesLength = Array();
        encodeLength(encodedWritableIndexesLength, lookup.writableIndexes.length);
        const encodedReadonlyIndexesLength = Array();
        encodeLength(encodedReadonlyIndexesLength, lookup.readonlyIndexes.length);
        const addressTableLookupLayout = struct($Array.of(publicKey("accountKey"), blob(encodedWritableIndexesLength.length, "encodedWritableIndexesLength"), seq(u8(), lookup.writableIndexes.length, "writableIndexes"), blob(encodedReadonlyIndexesLength.length, "encodedReadonlyIndexesLength"), seq(u8(), lookup.readonlyIndexes.length, "readonlyIndexes")));
        serializedLength += addressTableLookupLayout.encode($(function () {
          let result = $Object.create(null, undefined);
          result.accountKey = lookup.accountKey.toBytes();
          result.encodedWritableIndexesLength = new Uint8Array(encodedWritableIndexesLength);
          result.writableIndexes = lookup.writableIndexes;
          result.encodedReadonlyIndexesLength = new Uint8Array(encodedReadonlyIndexesLength);
          result.readonlyIndexes = lookup.readonlyIndexes;
          return result;
        })(), serializedAddressTableLookups, serializedLength);
      }
      return serializedAddressTableLookups.slice(0, serializedLength);
    }
    static deserialize(serializedMessage) {
      let byteArray = $Array.of(...serializedMessage);
      const prefix = guardedShift(byteArray);
      const maskedPrefix = prefix & VERSION_PREFIX_MASK;
      assert$1(prefix !== maskedPrefix, `Expected versioned message but received legacy message`);
      const version = maskedPrefix;
      assert$1(version === 0, `Expected versioned message with version 0 but found version ${version}`);
      const header = $(function () {
        let result = $Object.create(null, undefined);
        result.numRequiredSignatures = guardedShift(byteArray);
        result.numReadonlySignedAccounts = guardedShift(byteArray);
        result.numReadonlyUnsignedAccounts = guardedShift(byteArray);
        return result;
      })();
      const staticAccountKeys = $Array.of();
      const staticAccountKeysLength = decodeLength(byteArray);
      for (let i = 0; i < staticAccountKeysLength; i++) {
        staticAccountKeys.push(new PublicKey(guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH)));
      }
      const recentBlockhash = bs58$1.encode(guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH));
      const instructionCount = decodeLength(byteArray);
      const compiledInstructions = $Array.of();
      for (let i = 0; i < instructionCount; i++) {
        const programIdIndex = guardedShift(byteArray);
        const accountKeyIndexesLength = decodeLength(byteArray);
        const accountKeyIndexes = guardedSplice(byteArray, 0, accountKeyIndexesLength);
        const dataLength = decodeLength(byteArray);
        const data = new Uint8Array(guardedSplice(byteArray, 0, dataLength));
        compiledInstructions.push($(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = programIdIndex;
          result.accountKeyIndexes = accountKeyIndexes;
          result.data = data;
          return result;
        })());
      }
      const addressTableLookupsCount = decodeLength(byteArray);
      const addressTableLookups = $Array.of();
      for (let i = 0; i < addressTableLookupsCount; i++) {
        const accountKey = new PublicKey(guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH));
        const writableIndexesLength = decodeLength(byteArray);
        const writableIndexes = guardedSplice(byteArray, 0, writableIndexesLength);
        const readonlyIndexesLength = decodeLength(byteArray);
        const readonlyIndexes = guardedSplice(byteArray, 0, readonlyIndexesLength);
        addressTableLookups.push($(function () {
          let result = $Object.create(null, undefined);
          result.accountKey = accountKey;
          result.writableIndexes = writableIndexes;
          result.readonlyIndexes = readonlyIndexes;
          return result;
        })());
      }
      return new MessageV0($(function () {
        let result = $Object.create(null, undefined);
        result.header = header;
        result.staticAccountKeys = staticAccountKeys;
        result.recentBlockhash = recentBlockhash;
        result.compiledInstructions = compiledInstructions;
        result.addressTableLookups = addressTableLookups;
        return result;
      })());
    }
  }
  const VersionedMessage = $(function () {
    let result = $Object.create(null, undefined);
    result.deserializeMessageVersion = $(function (serializedMessage) {
      const prefix = serializedMessage[0];
      const maskedPrefix = prefix & VERSION_PREFIX_MASK;
      if (maskedPrefix === prefix) {
        return "legacy";
      }
      return maskedPrefix;
    });
    result.deserialize = $(serializedMessage => {
      const version = VersionedMessage.deserializeMessageVersion(serializedMessage);
      if (version === "legacy") {
        return Message.from(serializedMessage);
      }
      if (version === 0) {
        return MessageV0.deserialize(serializedMessage);
      } else {
        throw new Error(`Transaction message version ${version} deserialization is not supported`);
      }
    });
    return result;
  })();
  let TransactionStatus = $(function (TransactionStatus) {
    TransactionStatus[TransactionStatus["BLOCKHEIGHT_EXCEEDED"] = 0] = "BLOCKHEIGHT_EXCEEDED";
    TransactionStatus[TransactionStatus["PROCESSED"] = 1] = "PROCESSED";
    TransactionStatus[TransactionStatus["TIMED_OUT"] = 2] = "TIMED_OUT";
    TransactionStatus[TransactionStatus["NONCE_INVALID"] = 3] = "NONCE_INVALID";
    return TransactionStatus;
  })($Object.create(null, undefined));
  const DEFAULT_SIGNATURE = buffer.Buffer.alloc(SIGNATURE_LENGTH_IN_BYTES).fill(0);
  class TransactionInstruction {
    constructor(opts) {
      this.keys = void 0;
      this.programId = void 0;
      this.data = buffer.Buffer.alloc(0);
      this.programId = opts.programId;
      this.keys = opts.keys;
      if (opts.data) {
        this.data = opts.data;
      }
    }
    toJSON() {
      return $(function () {
        let result = $Object.create(null, undefined);
        result.keys = this.keys.map($(({
          pubkey: pubkey,
          isSigner: isSigner,
          isWritable: isWritable
        }) => $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = pubkey.toJSON();
          result.isSigner = isSigner;
          result.isWritable = isWritable;
          return result;
        })()));
        result.programId = this.programId.toJSON();
        result.data = $Array.of(...this.data);
        return result;
      }).bind(this)();
    }
  }
  class Transaction {
    get signature() {
      if (this.signatures.length > 0) {
        return this.signatures[0].signature;
      }
      return null;
    }
    constructor(opts) {
      this.signatures = $Array.of();
      this.feePayer = void 0;
      this.instructions = $Array.of();
      this.recentBlockhash = void 0;
      this.lastValidBlockHeight = void 0;
      this.nonceInfo = void 0;
      this.minNonceContextSlot = void 0;
      this._message = void 0;
      this._json = void 0;
      if (!opts) {
        return;
      }
      if (opts.feePayer) {
        this.feePayer = opts.feePayer;
      }
      if (opts.signatures) {
        this.signatures = opts.signatures;
      }
      if ($Object.prototype.hasOwnProperty.call(opts, "nonceInfo")) {
        const {
          minContextSlot: minContextSlot,
          nonceInfo: nonceInfo
        } = opts;
        this.minNonceContextSlot = minContextSlot;
        this.nonceInfo = nonceInfo;
      } else if ($Object.prototype.hasOwnProperty.call(opts, "lastValidBlockHeight")) {
        const {
          blockhash: blockhash,
          lastValidBlockHeight: lastValidBlockHeight
        } = opts;
        this.recentBlockhash = blockhash;
        this.lastValidBlockHeight = lastValidBlockHeight;
      } else {
        const {
          recentBlockhash: recentBlockhash,
          nonceInfo: nonceInfo
        } = opts;
        if (nonceInfo) {
          this.nonceInfo = nonceInfo;
        }
        this.recentBlockhash = recentBlockhash;
      }
    }
    toJSON() {
      return $(function () {
        let result = $Object.create(null, undefined);
        result.recentBlockhash = this.recentBlockhash || null;
        result.feePayer = this.feePayer ? this.feePayer.toJSON() : null;
        result.nonceInfo = this.nonceInfo ? $(function () {
          let result = $Object.create(null, undefined);
          result.nonce = this.nonceInfo.nonce;
          result.nonceInstruction = this.nonceInfo.nonceInstruction.toJSON();
          return result;
        })() : null;
        result.instructions = this.instructions.map($(instruction => instruction.toJSON()));
        result.signers = this.signatures.map($(({
          publicKey: publicKey
        }) => publicKey.toJSON()));
        return result;
      }).bind(this)();
    }
    add(...items) {
      if (items.length === 0) {
        throw new Error("No instructions");
      }
      items.forEach($(item => {
        if ("instructions" in item) {
          this.instructions = this.instructions.concat(item.instructions);
        } else if ("data" in item && "programId" in item && "keys" in item) {
          this.instructions.push(item);
        } else {
          this.instructions.push(new TransactionInstruction(item));
        }
      }));
      return this;
    }
    compileMessage() {
      if (this._message && JSON.stringify(this.toJSON()) === JSON.stringify(this._json)) {
        return this._message;
      }
      let recentBlockhash;
      let instructions;
      if (this.nonceInfo) {
        recentBlockhash = this.nonceInfo.nonce;
        if (this.instructions[0] != this.nonceInfo.nonceInstruction) {
          instructions = $Array.of(this.nonceInfo.nonceInstruction, ...this.instructions);
        } else {
          instructions = this.instructions;
        }
      } else {
        recentBlockhash = this.recentBlockhash;
        instructions = this.instructions;
      }
      if (!recentBlockhash) {
        throw new Error("Transaction recentBlockhash required");
      }
      if (instructions.length < 1) {
        console.warn("No instructions provided");
      }
      let feePayer;
      if (this.feePayer) {
        feePayer = this.feePayer;
      } else if (this.signatures.length > 0 && this.signatures[0].publicKey) {
        feePayer = this.signatures[0].publicKey;
      } else {
        throw new Error("Transaction fee payer required");
      }
      for (let i = 0; i < instructions.length; i++) {
        if (instructions[i].programId === undefined) {
          throw new Error(`Transaction instruction index ${i} has undefined program id`);
        }
      }
      const programIds = $Array.of();
      const accountMetas = $Array.of();
      instructions.forEach($(instruction => {
        instruction.keys.forEach($(accountMeta => {
          accountMetas.push($(function () {
            let result = $Object.create(null, undefined);
            $Object.assign(result, accountMeta);
            return result;
          })());
        }));
        const programId = instruction.programId.toString();
        if (!programIds.includes(programId)) {
          programIds.push(programId);
        }
      }));
      programIds.forEach($(programId => {
        accountMetas.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = new PublicKey(programId);
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })());
      }));
      const uniqueMetas = $Array.of();
      accountMetas.forEach($(accountMeta => {
        const pubkeyString = accountMeta.pubkey.toString();
        const uniqueIndex = uniqueMetas.findIndex($(x => x.pubkey.toString() === pubkeyString));
        if (uniqueIndex > -1) {
          uniqueMetas[uniqueIndex].isWritable = uniqueMetas[uniqueIndex].isWritable || accountMeta.isWritable;
          uniqueMetas[uniqueIndex].isSigner = uniqueMetas[uniqueIndex].isSigner || accountMeta.isSigner;
        } else {
          uniqueMetas.push(accountMeta);
        }
      }));
      uniqueMetas.sort($(function (x, y) {
        if (x.isSigner !== y.isSigner) {
          return x.isSigner ? -1 : 1;
        }
        if (x.isWritable !== y.isWritable) {
          return x.isWritable ? -1 : 1;
        }
        const options = $(function () {
          let result = $Object.create(null, undefined);
          result.localeMatcher = "best fit";
          result.usage = "sort";
          result.sensitivity = "variant";
          result.ignorePunctuation = false;
          result.numeric = false;
          result.caseFirst = "lower";
          return result;
        })();
        return x.pubkey.toBase58().localeCompare(y.pubkey.toBase58(), "en", options);
      }));
      const feePayerIndex = uniqueMetas.findIndex($(x => x.pubkey.equals(feePayer)));
      if (feePayerIndex > -1) {
        const [payerMeta] = uniqueMetas.splice(feePayerIndex, 1);
        payerMeta.isSigner = true;
        payerMeta.isWritable = true;
        uniqueMetas.unshift(payerMeta);
      } else {
        uniqueMetas.unshift($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = feePayer;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })());
      }
      for (const signature of this.signatures) {
        const uniqueIndex = uniqueMetas.findIndex($(x => x.pubkey.equals(signature.publicKey)));
        if (uniqueIndex > -1) {
          if (!uniqueMetas[uniqueIndex].isSigner) {
            uniqueMetas[uniqueIndex].isSigner = true;
            console.warn("Transaction references a signature that is unnecessary, " + "only the fee payer and instruction signer accounts should sign a transaction. " + "This behavior is deprecated and will throw an error in the next major version release.");
          }
        } else {
          throw new Error(`unknown signer: ${signature.publicKey.toString()}`);
        }
      }
      let numRequiredSignatures = 0;
      let numReadonlySignedAccounts = 0;
      let numReadonlyUnsignedAccounts = 0;
      const signedKeys = $Array.of();
      const unsignedKeys = $Array.of();
      uniqueMetas.forEach($(({
        pubkey: pubkey,
        isSigner: isSigner,
        isWritable: isWritable
      }) => {
        if (isSigner) {
          signedKeys.push(pubkey.toString());
          numRequiredSignatures += 1;
          if (!isWritable) {
            numReadonlySignedAccounts += 1;
          }
        } else {
          unsignedKeys.push(pubkey.toString());
          if (!isWritable) {
            numReadonlyUnsignedAccounts += 1;
          }
        }
      }));
      const accountKeys = signedKeys.concat(unsignedKeys);
      const compiledInstructions = instructions.map($(instruction => {
        const {
          data: data,
          programId: programId
        } = instruction;
        return $(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = accountKeys.indexOf(programId.toString());
          result.accounts = instruction.keys.map($(meta => accountKeys.indexOf(meta.pubkey.toString())));
          result.data = bs58$1.encode(data);
          return result;
        })();
      }));
      compiledInstructions.forEach($(instruction => {
        assert$1(instruction.programIdIndex >= 0);
        instruction.accounts.forEach($(keyIndex => assert$1(keyIndex >= 0)));
      }));
      return new Message($(function () {
        let result = $Object.create(null, undefined);
        result.header = $(function () {
          let result = $Object.create(null, undefined);
          result.numRequiredSignatures = numRequiredSignatures;
          result.numReadonlySignedAccounts = numReadonlySignedAccounts;
          result.numReadonlyUnsignedAccounts = numReadonlyUnsignedAccounts;
          return result;
        })();
        result.accountKeys = accountKeys;
        result.recentBlockhash = recentBlockhash;
        result.instructions = compiledInstructions;
        return result;
      })());
    }
    _compile() {
      const message = this.compileMessage();
      const signedKeys = message.accountKeys.slice(0, message.header.numRequiredSignatures);
      if (this.signatures.length === signedKeys.length) {
        const valid = this.signatures.every($((pair, index) => signedKeys[index].equals(pair.publicKey)));
        if (valid) return message;
      }
      this.signatures = signedKeys.map($(publicKey => $(function () {
        let result = $Object.create(null, undefined);
        result.signature = null;
        result.publicKey = publicKey;
        return result;
      })()));
      return message;
    }
    serializeMessage() {
      return this._compile().serialize();
    }
    async getEstimatedFee(connection) {
      return (await connection.getFeeForMessage(this.compileMessage())).value;
    }
    setSigners(...signers) {
      if (signers.length === 0) {
        throw new Error("No signers");
      }
      const seen = new Set();
      this.signatures = signers.filter($(publicKey => {
        const key = publicKey.toString();
        if (seen.has(key)) {
          return false;
        } else {
          seen.add(key);
          return true;
        }
      })).map($(publicKey => $(function () {
        let result = $Object.create(null, undefined);
        result.signature = null;
        result.publicKey = publicKey;
        return result;
      })()));
    }
    sign(...signers) {
      if (signers.length === 0) {
        throw new Error("No signers");
      }
      const seen = new Set();
      const uniqueSigners = $Array.of();
      for (const signer of signers) {
        const key = signer.publicKey.toString();
        if (seen.has(key)) {
          continue;
        } else {
          seen.add(key);
          uniqueSigners.push(signer);
        }
      }
      this.signatures = uniqueSigners.map($(signer => $(function () {
        let result = $Object.create(null, undefined);
        result.signature = null;
        result.publicKey = signer.publicKey;
        return result;
      })()));
      const message = this._compile();
      this._partialSign(message, ...uniqueSigners);
    }
    partialSign(...signers) {
      if (signers.length === 0) {
        throw new Error("No signers");
      }
      const seen = new Set();
      const uniqueSigners = $Array.of();
      for (const signer of signers) {
        const key = signer.publicKey.toString();
        if (seen.has(key)) {
          continue;
        } else {
          seen.add(key);
          uniqueSigners.push(signer);
        }
      }
      const message = this._compile();
      this._partialSign(message, ...uniqueSigners);
    }
    _partialSign(message, ...signers) {
      const signData = message.serialize();
      signers.forEach($(signer => {
        const signature = sign(signData, signer.secretKey);
        this._addSignature(signer.publicKey, toBuffer(signature));
      }));
    }
    addSignature(pubkey, signature) {
      this._compile();
      this._addSignature(pubkey, signature);
    }
    _addSignature(pubkey, signature) {
      assert$1(signature.length === 64);
      const index = this.signatures.findIndex($(sigpair => pubkey.equals(sigpair.publicKey)));
      if (index < 0) {
        throw new Error(`unknown signer: ${pubkey.toString()}`);
      }
      this.signatures[index].signature = buffer.Buffer.from(signature);
    }
    verifySignatures(requireAllSignatures = true) {
      const signatureErrors = this._getMessageSignednessErrors(this.serializeMessage(), requireAllSignatures);
      return !signatureErrors;
    }
    _getMessageSignednessErrors(message, requireAllSignatures) {
      const errors = $Object.create(null, undefined);
      for (const {
        signature: signature,
        publicKey: publicKey
      } of this.signatures) {
        if (signature === null) {
          if (requireAllSignatures) {
            (errors.missing ||= $Array.of()).push(publicKey);
          }
        } else {
          if (!verify(signature, message, publicKey.toBytes())) {
            (errors.invalid ||= $Array.of()).push(publicKey);
          }
        }
      }
      return errors.invalid || errors.missing ? errors : undefined;
    }
    serialize(config) {
      const {
        requireAllSignatures: requireAllSignatures,
        verifySignatures: verifySignatures
      } = $Object.assign($(function () {
        let result = $Object.create(null, undefined);
        result.requireAllSignatures = true;
        result.verifySignatures = true;
        return result;
      })(), config);
      const signData = this.serializeMessage();
      if (verifySignatures) {
        const sigErrors = this._getMessageSignednessErrors(signData, requireAllSignatures);
        if (sigErrors) {
          let errorMessage = "Signature verification failed.";
          if (sigErrors.invalid) {
            errorMessage += `\nInvalid signature for public key${sigErrors.invalid.length === 1 ? "" : "(s)"} [\`${sigErrors.invalid.map($(p => p.toBase58())).join("`, `")}\`].`;
          }
          if (sigErrors.missing) {
            errorMessage += `\nMissing signature for public key${sigErrors.missing.length === 1 ? "" : "(s)"} [\`${sigErrors.missing.map($(p => p.toBase58())).join("`, `")}\`].`;
          }
          throw new Error(errorMessage);
        }
      }
      return this._serialize(signData);
    }
    _serialize(signData) {
      const {
        signatures: signatures
      } = this;
      const signatureCount = $Array.of();
      encodeLength(signatureCount, signatures.length);
      const transactionLength = signatureCount.length + signatures.length * 64 + signData.length;
      const wireTransaction = buffer.Buffer.alloc(transactionLength);
      assert$1(signatures.length < 256);
      buffer.Buffer.from(signatureCount).copy(wireTransaction, 0);
      signatures.forEach($(({
        signature: signature
      }, index) => {
        if (signature !== null) {
          assert$1(signature.length === 64, `signature has invalid length`);
          buffer.Buffer.from(signature).copy(wireTransaction, signatureCount.length + index * 64);
        }
      }));
      signData.copy(wireTransaction, signatureCount.length + signatures.length * 64);
      assert$1(wireTransaction.length <= PACKET_DATA_SIZE, `Transaction too large: ${wireTransaction.length} > ${PACKET_DATA_SIZE}`);
      return wireTransaction;
    }
    get keys() {
      assert$1(this.instructions.length === 1);
      return this.instructions[0].keys.map($(keyObj => keyObj.pubkey));
    }
    get programId() {
      assert$1(this.instructions.length === 1);
      return this.instructions[0].programId;
    }
    get data() {
      assert$1(this.instructions.length === 1);
      return this.instructions[0].data;
    }
    static from(buffer$1) {
      let byteArray = $Array.of(...buffer$1);
      const signatureCount = decodeLength(byteArray);
      let signatures = $Array.of();
      for (let i = 0; i < signatureCount; i++) {
        const signature = guardedSplice(byteArray, 0, SIGNATURE_LENGTH_IN_BYTES);
        signatures.push(bs58$1.encode(buffer.Buffer.from(signature)));
      }
      return Transaction.populate(Message.from(byteArray), signatures);
    }
    static populate(message, signatures = $Array.of()) {
      const transaction = new Transaction();
      transaction.recentBlockhash = message.recentBlockhash;
      if (message.header.numRequiredSignatures > 0) {
        transaction.feePayer = message.accountKeys[0];
      }
      signatures.forEach($((signature, index) => {
        const sigPubkeyPair = $(function () {
          let result = $Object.create(null, undefined);
          result.signature = signature == bs58$1.encode(DEFAULT_SIGNATURE) ? null : bs58$1.decode(signature);
          result.publicKey = message.accountKeys[index];
          return result;
        })();
        transaction.signatures.push(sigPubkeyPair);
      }));
      message.instructions.forEach($(instruction => {
        const keys = instruction.accounts.map($(account => {
          const pubkey = message.accountKeys[account];
          return $(function () {
            let result = $Object.create(null, undefined);
            result.pubkey = pubkey;
            result.isSigner = transaction.signatures.some($(keyObj => keyObj.publicKey.toString() === pubkey.toString())) || message.isAccountSigner(account);
            result.isWritable = message.isAccountWritable(account);
            return result;
          })();
        }));
        transaction.instructions.push(new TransactionInstruction($(function () {
          let result = $Object.create(null, undefined);
          result.keys = keys;
          result.programId = message.accountKeys[instruction.programIdIndex];
          result.data = bs58$1.decode(instruction.data);
          return result;
        })()));
      }));
      transaction._message = message;
      transaction._json = transaction.toJSON();
      return transaction;
    }
  }
  class TransactionMessage {
    constructor(args) {
      this.payerKey = void 0;
      this.instructions = void 0;
      this.recentBlockhash = void 0;
      this.payerKey = args.payerKey;
      this.instructions = args.instructions;
      this.recentBlockhash = args.recentBlockhash;
    }
    static decompile(message, args) {
      const {
        header: header,
        compiledInstructions: compiledInstructions,
        recentBlockhash: recentBlockhash
      } = message;
      const {
        numRequiredSignatures: numRequiredSignatures,
        numReadonlySignedAccounts: numReadonlySignedAccounts,
        numReadonlyUnsignedAccounts: numReadonlyUnsignedAccounts
      } = header;
      const numWritableSignedAccounts = numRequiredSignatures - numReadonlySignedAccounts;
      assert$1(numWritableSignedAccounts > 0, "Message header is invalid");
      const numWritableUnsignedAccounts = message.staticAccountKeys.length - numRequiredSignatures - numReadonlyUnsignedAccounts;
      assert$1(numWritableUnsignedAccounts >= 0, "Message header is invalid");
      const accountKeys = message.getAccountKeys(args);
      const payerKey = accountKeys.get(0);
      if (payerKey === undefined) {
        throw new Error("Failed to decompile message because no account keys were found");
      }
      const instructions = $Array.of();
      for (const compiledIx of compiledInstructions) {
        const keys = $Array.of();
        for (const keyIndex of compiledIx.accountKeyIndexes) {
          const pubkey = accountKeys.get(keyIndex);
          if (pubkey === undefined) {
            throw new Error(`Failed to find key for account key index ${keyIndex}`);
          }
          const isSigner = keyIndex < numRequiredSignatures;
          let isWritable;
          if (isSigner) {
            isWritable = keyIndex < numWritableSignedAccounts;
          } else if (keyIndex < accountKeys.staticAccountKeys.length) {
            isWritable = keyIndex - numRequiredSignatures < numWritableUnsignedAccounts;
          } else {
            isWritable = keyIndex - accountKeys.staticAccountKeys.length < accountKeys.accountKeysFromLookups.writable.length;
          }
          keys.push($(function () {
            let result = $Object.create(null, undefined);
            result.pubkey = pubkey;
            result.isSigner = keyIndex < header.numRequiredSignatures;
            result.isWritable = isWritable;
            return result;
          })());
        }
        const programId = accountKeys.get(compiledIx.programIdIndex);
        if (programId === undefined) {
          throw new Error(`Failed to find program id for program id index ${compiledIx.programIdIndex}`);
        }
        instructions.push(new TransactionInstruction($(function () {
          let result = $Object.create(null, undefined);
          result.programId = programId;
          result.data = toBuffer(compiledIx.data);
          result.keys = keys;
          return result;
        })()));
      }
      return new TransactionMessage($(function () {
        let result = $Object.create(null, undefined);
        result.payerKey = payerKey;
        result.instructions = instructions;
        result.recentBlockhash = recentBlockhash;
        return result;
      })());
    }
    compileToLegacyMessage() {
      return Message.compile($(function () {
        let result = $Object.create(null, undefined);
        result.payerKey = this.payerKey;
        result.recentBlockhash = this.recentBlockhash;
        result.instructions = this.instructions;
        return result;
      }).bind(this)());
    }
    compileToV0Message(addressLookupTableAccounts) {
      return MessageV0.compile($(function () {
        let result = $Object.create(null, undefined);
        result.payerKey = this.payerKey;
        result.recentBlockhash = this.recentBlockhash;
        result.instructions = this.instructions;
        result.addressLookupTableAccounts = addressLookupTableAccounts;
        return result;
      }).bind(this)());
    }
  }
  class VersionedTransaction {
    get version() {
      return this.message.version;
    }
    constructor(message, signatures) {
      this.signatures = void 0;
      this.message = void 0;
      if (signatures !== undefined) {
        assert$1(signatures.length === message.header.numRequiredSignatures, "Expected signatures length to be equal to the number of required signatures");
        this.signatures = signatures;
      } else {
        const defaultSignatures = $Array.of();
        for (let i = 0; i < message.header.numRequiredSignatures; i++) {
          defaultSignatures.push(new Uint8Array(SIGNATURE_LENGTH_IN_BYTES));
        }
        this.signatures = defaultSignatures;
      }
      this.message = message;
    }
    serialize() {
      const serializedMessage = this.message.serialize();
      const encodedSignaturesLength = Array();
      encodeLength(encodedSignaturesLength, this.signatures.length);
      const transactionLayout = struct($Array.of(blob(encodedSignaturesLength.length, "encodedSignaturesLength"), seq(signature(), this.signatures.length, "signatures"), blob(serializedMessage.length, "serializedMessage")));
      const serializedTransaction = new Uint8Array(2048);
      const serializedTransactionLength = transactionLayout.encode($(function () {
        let result = $Object.create(null, undefined);
        result.encodedSignaturesLength = new Uint8Array(encodedSignaturesLength);
        result.signatures = this.signatures;
        result.serializedMessage = serializedMessage;
        return result;
      }).bind(this)(), serializedTransaction);
      return serializedTransaction.slice(0, serializedTransactionLength);
    }
    static deserialize(serializedTransaction) {
      let byteArray = $Array.of(...serializedTransaction);
      const signatures = $Array.of();
      const signaturesLength = decodeLength(byteArray);
      for (let i = 0; i < signaturesLength; i++) {
        signatures.push(new Uint8Array(guardedSplice(byteArray, 0, SIGNATURE_LENGTH_IN_BYTES)));
      }
      const message = VersionedMessage.deserialize(new Uint8Array(byteArray));
      return new VersionedTransaction(message, signatures);
    }
    sign(signers) {
      const messageData = this.message.serialize();
      const signerPubkeys = this.message.staticAccountKeys.slice(0, this.message.header.numRequiredSignatures);
      for (const signer of signers) {
        const signerIndex = signerPubkeys.findIndex($(pubkey => pubkey.equals(signer.publicKey)));
        assert$1(signerIndex >= 0, `Cannot sign with non signer key ${signer.publicKey.toBase58()}`);
        this.signatures[signerIndex] = sign(messageData, signer.secretKey);
      }
    }
    addSignature(publicKey, signature) {
      assert$1(signature.byteLength === 64, "Signature must be 64 bytes long");
      const signerPubkeys = this.message.staticAccountKeys.slice(0, this.message.header.numRequiredSignatures);
      const signerIndex = signerPubkeys.findIndex($(pubkey => pubkey.equals(publicKey)));
      assert$1(signerIndex >= 0, `Can not add signature; \`${publicKey.toBase58()}\` is not required to sign this transaction`);
      this.signatures[signerIndex] = signature;
    }
  }
  const NUM_TICKS_PER_SECOND = 160;
  const DEFAULT_TICKS_PER_SLOT = 64;
  const NUM_SLOTS_PER_SECOND = NUM_TICKS_PER_SECOND / DEFAULT_TICKS_PER_SLOT;
  const MS_PER_SLOT = 1e3 / NUM_SLOTS_PER_SECOND;
  const SYSVAR_CLOCK_PUBKEY = new PublicKey("SysvarC1ock11111111111111111111111111111111");
  const SYSVAR_EPOCH_SCHEDULE_PUBKEY = new PublicKey("SysvarEpochSchedu1e111111111111111111111111");
  const SYSVAR_INSTRUCTIONS_PUBKEY = new PublicKey("Sysvar1nstructions1111111111111111111111111");
  const SYSVAR_RECENT_BLOCKHASHES_PUBKEY = new PublicKey("SysvarRecentB1ockHashes11111111111111111111");
  const SYSVAR_RENT_PUBKEY = new PublicKey("SysvarRent111111111111111111111111111111111");
  const SYSVAR_REWARDS_PUBKEY = new PublicKey("SysvarRewards111111111111111111111111111111");
  const SYSVAR_SLOT_HASHES_PUBKEY = new PublicKey("SysvarS1otHashes111111111111111111111111111");
  const SYSVAR_SLOT_HISTORY_PUBKEY = new PublicKey("SysvarS1otHistory11111111111111111111111111");
  const SYSVAR_STAKE_HISTORY_PUBKEY = new PublicKey("SysvarStakeHistory1111111111111111111111111");
  async function sendAndConfirmTransaction(connection, transaction, signers, options) {
    const sendOptions = options && $(function () {
      let result = $Object.create(null, undefined);
      result.skipPreflight = options.skipPreflight;
      result.preflightCommitment = options.preflightCommitment || options.commitment;
      result.maxRetries = options.maxRetries;
      result.minContextSlot = options.minContextSlot;
      return result;
    })();
    const signature = await connection.sendTransaction(transaction, signers, sendOptions);
    let status;
    if (transaction.recentBlockhash != null && transaction.lastValidBlockHeight != null) {
      status = (await connection.confirmTransaction($(function () {
        let result = $Object.create(null, undefined);
        result.abortSignal = options?.abortSignal;
        result.signature = signature;
        result.blockhash = transaction.recentBlockhash;
        result.lastValidBlockHeight = transaction.lastValidBlockHeight;
        return result;
      })(), options && options.commitment)).value;
    } else if (transaction.minNonceContextSlot != null && transaction.nonceInfo != null) {
      const {
        nonceInstruction: nonceInstruction
      } = transaction.nonceInfo;
      const nonceAccountPubkey = nonceInstruction.keys[0].pubkey;
      status = (await connection.confirmTransaction($(function () {
        let result = $Object.create(null, undefined);
        result.abortSignal = options?.abortSignal;
        result.minContextSlot = transaction.minNonceContextSlot;
        result.nonceAccountPubkey = nonceAccountPubkey;
        result.nonceValue = transaction.nonceInfo.nonce;
        result.signature = signature;
        return result;
      })(), options && options.commitment)).value;
    } else {
      if (options?.abortSignal != null) {
        console.warn("sendAndConfirmTransaction(): A transaction with a deprecated confirmation strategy was " + "supplied along with an `abortSignal`. Only transactions having `lastValidBlockHeight` " + "or a combination of `nonceInfo` and `minNonceContextSlot` are abortable.");
      }
      status = (await connection.confirmTransaction(signature, options && options.commitment)).value;
    }
    if (status.err) {
      throw new Error(`Transaction ${signature} failed (${JSON.stringify(status)})`);
    }
    return signature;
  }
  $(sendAndConfirmTransaction);
  function sleep(ms) {
    return new Promise($(resolve => setTimeout(resolve, ms)));
  }
  $(sleep);
  function encodeData(type, fields) {
    const allocLength = type.layout.span >= 0 ? type.layout.span : getAlloc(type, fields);
    const data = buffer.Buffer.alloc(allocLength);
    const layoutFields = $Object.assign($(function () {
      let result = $Object.create(null, undefined);
      result.instruction = type.index;
      return result;
    })(), fields);
    type.layout.encode(layoutFields, data);
    return data;
  }
  $(encodeData);
  function decodeData$1(type, buffer) {
    let data;
    try {
      data = type.layout.decode(buffer);
    } catch (err) {
      throw new Error("invalid instruction; " + err);
    }
    if (data.instruction !== type.index) {
      throw new Error(`invalid instruction; instruction index mismatch ${data.instruction} != ${type.index}`);
    }
    return data;
  }
  $(decodeData$1);
  const FeeCalculatorLayout = nu64("lamportsPerSignature");
  const NonceAccountLayout = struct($Array.of(u32("version"), u32("state"), publicKey("authorizedPubkey"), publicKey("nonce"), struct($Array.of(FeeCalculatorLayout), "feeCalculator")));
  const NONCE_ACCOUNT_LENGTH = NonceAccountLayout.span;
  class NonceAccount {
    constructor(args) {
      this.authorizedPubkey = void 0;
      this.nonce = void 0;
      this.feeCalculator = void 0;
      this.authorizedPubkey = args.authorizedPubkey;
      this.nonce = args.nonce;
      this.feeCalculator = args.feeCalculator;
    }
    static fromAccountData(buffer) {
      const nonceAccount = NonceAccountLayout.decode(toBuffer(buffer), 0);
      return new NonceAccount($(function () {
        let result = $Object.create(null, undefined);
        result.authorizedPubkey = new PublicKey(nonceAccount.authorizedPubkey);
        result.nonce = new PublicKey(nonceAccount.nonce).toString();
        result.feeCalculator = nonceAccount.feeCalculator;
        return result;
      })());
    }
  }
  var browser$1 = $Object.create(null, undefined);
  $Object.defineProperty(browser$1, "__esModule", $(function () {
    let result = $Object.create(null, undefined);
    result.value = true;
    return result;
  })());
  function toBigIntLE(buf) {
    {
      const reversed = Buffer.from(buf);
      reversed.reverse();
      const hex = reversed.toString("hex");
      if (hex.length === 0) {
        return BigInt(0);
      }
      return BigInt(`0x${hex}`);
    }
  }
  $(toBigIntLE);
  var toBigIntLE_1 = browser$1.toBigIntLE = toBigIntLE;
  function toBigIntBE(buf) {
    {
      const hex = buf.toString("hex");
      if (hex.length === 0) {
        return BigInt(0);
      }
      return BigInt(`0x${hex}`);
    }
  }
  $(toBigIntBE);
  browser$1.toBigIntBE = toBigIntBE;
  function toBufferLE(num, width) {
    {
      const hex = num.toString(16);
      const buffer = Buffer.from(hex.padStart(width * 2, "0").slice(0, width * 2), "hex");
      buffer.reverse();
      return buffer;
    }
  }
  $(toBufferLE);
  var toBufferLE_1 = browser$1.toBufferLE = toBufferLE;
  function toBufferBE(num, width) {
    {
      const hex = num.toString(16);
      return Buffer.from(hex.padStart(width * 2, "0").slice(0, width * 2), "hex");
    }
  }
  $(toBufferBE);
  browser$1.toBufferBE = toBufferBE;
  const encodeDecode = layout => {
    const decode = layout.decode.bind(layout);
    const encode = layout.encode.bind(layout);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.decode = decode;
      result.encode = encode;
      return result;
    })();
  };
  $(encodeDecode);
  const bigInt = length => $(property => {
    const layout = blob(length, property);
    const {
      encode: encode,
      decode: decode
    } = encodeDecode(layout);
    const bigIntLayout = layout;
    bigIntLayout.decode = $((buffer$1, offset) => {
      const src = decode(buffer$1, offset);
      return toBigIntLE_1(buffer.Buffer.from(src));
    });
    bigIntLayout.encode = $((bigInt, buffer, offset) => {
      const src = toBufferLE_1(bigInt, length);
      return encode(src, buffer, offset);
    });
    return bigIntLayout;
  });
  $(bigInt);
  const u64 = bigInt(8);
  class SystemInstruction {
    constructor() {}
    static decodeInstructionType(instruction) {
      this.checkProgramId(instruction.programId);
      const instructionTypeLayout = u32("instruction");
      const typeIndex = instructionTypeLayout.decode(instruction.data);
      let type;
      for (const [ixType, layout] of $Object.entries(SYSTEM_INSTRUCTION_LAYOUTS)) {
        if (layout.index == typeIndex) {
          type = ixType;
          break;
        }
      }
      if (!type) {
        throw new Error("Instruction type incorrect; not a SystemInstruction");
      }
      return type;
    }
    static decodeCreateAccount(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        lamports: lamports,
        space: space,
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.Create, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = instruction.keys[0].pubkey;
        result.newAccountPubkey = instruction.keys[1].pubkey;
        result.lamports = lamports;
        result.space = space;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeTransfer(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        lamports: lamports
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.Transfer, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = instruction.keys[0].pubkey;
        result.toPubkey = instruction.keys[1].pubkey;
        result.lamports = lamports;
        return result;
      })();
    }
    static decodeTransferWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        lamports: lamports,
        seed: seed,
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.TransferWithSeed, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = instruction.keys[0].pubkey;
        result.basePubkey = instruction.keys[1].pubkey;
        result.toPubkey = instruction.keys[2].pubkey;
        result.lamports = lamports;
        result.seed = seed;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeAllocate(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 1);
      const {
        space: space
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.Allocate, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.accountPubkey = instruction.keys[0].pubkey;
        result.space = space;
        return result;
      })();
    }
    static decodeAllocateWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 1);
      const {
        base: base,
        seed: seed,
        space: space,
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.AllocateWithSeed, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.accountPubkey = instruction.keys[0].pubkey;
        result.basePubkey = new PublicKey(base);
        result.seed = seed;
        result.space = space;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeAssign(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 1);
      const {
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.Assign, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.accountPubkey = instruction.keys[0].pubkey;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeAssignWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 1);
      const {
        base: base,
        seed: seed,
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.AssignWithSeed, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.accountPubkey = instruction.keys[0].pubkey;
        result.basePubkey = new PublicKey(base);
        result.seed = seed;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeCreateWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        base: base,
        seed: seed,
        lamports: lamports,
        space: space,
        programId: programId
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.CreateWithSeed, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = instruction.keys[0].pubkey;
        result.newAccountPubkey = instruction.keys[1].pubkey;
        result.basePubkey = new PublicKey(base);
        result.seed = seed;
        result.lamports = lamports;
        result.space = space;
        result.programId = new PublicKey(programId);
        return result;
      })();
    }
    static decodeNonceInitialize(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        authorized: authorized
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.InitializeNonceAccount, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.noncePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = new PublicKey(authorized);
        return result;
      })();
    }
    static decodeNonceAdvance(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.AdvanceNonceAccount, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.noncePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = instruction.keys[2].pubkey;
        return result;
      })();
    }
    static decodeNonceWithdraw(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 5);
      const {
        lamports: lamports
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.WithdrawNonceAccount, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.noncePubkey = instruction.keys[0].pubkey;
        result.toPubkey = instruction.keys[1].pubkey;
        result.authorizedPubkey = instruction.keys[4].pubkey;
        result.lamports = lamports;
        return result;
      })();
    }
    static decodeNonceAuthorize(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        authorized: authorized
      } = decodeData$1(SYSTEM_INSTRUCTION_LAYOUTS.AuthorizeNonceAccount, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.noncePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = instruction.keys[1].pubkey;
        result.newAuthorizedPubkey = new PublicKey(authorized);
        return result;
      })();
    }
    static checkProgramId(programId) {
      if (!programId.equals(SystemProgram.programId)) {
        throw new Error("invalid instruction; programId is not SystemProgram");
      }
    }
    static checkKeyLength(keys, expectedLength) {
      if (keys.length < expectedLength) {
        throw new Error(`invalid instruction; found ${keys.length} keys, expected at least ${expectedLength}`);
      }
    }
  }
  const SYSTEM_INSTRUCTION_LAYOUTS = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.Create = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      result.layout = struct($Array.of(u32("instruction"), ns64("lamports"), ns64("space"), publicKey("programId")));
      return result;
    })();
    result.Assign = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      result.layout = struct($Array.of(u32("instruction"), publicKey("programId")));
      return result;
    })();
    result.Transfer = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 2;
      result.layout = struct($Array.of(u32("instruction"), u64("lamports")));
      return result;
    })();
    result.CreateWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 3;
      result.layout = struct($Array.of(u32("instruction"), publicKey("base"), rustString("seed"), ns64("lamports"), ns64("space"), publicKey("programId")));
      return result;
    })();
    result.AdvanceNonceAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 4;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.WithdrawNonceAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 5;
      result.layout = struct($Array.of(u32("instruction"), ns64("lamports")));
      return result;
    })();
    result.InitializeNonceAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 6;
      result.layout = struct($Array.of(u32("instruction"), publicKey("authorized")));
      return result;
    })();
    result.AuthorizeNonceAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 7;
      result.layout = struct($Array.of(u32("instruction"), publicKey("authorized")));
      return result;
    })();
    result.Allocate = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 8;
      result.layout = struct($Array.of(u32("instruction"), ns64("space")));
      return result;
    })();
    result.AllocateWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 9;
      result.layout = struct($Array.of(u32("instruction"), publicKey("base"), rustString("seed"), ns64("space"), publicKey("programId")));
      return result;
    })();
    result.AssignWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 10;
      result.layout = struct($Array.of(u32("instruction"), publicKey("base"), rustString("seed"), publicKey("programId")));
      return result;
    })();
    result.TransferWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 11;
      result.layout = struct($Array.of(u32("instruction"), u64("lamports"), rustString("seed"), publicKey("programId")));
      return result;
    })();
    result.UpgradeNonceAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 12;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    return result;
  })());
  class SystemProgram {
    constructor() {}
    static createAccount(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.Create;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.lamports = params.lamports;
        result.space = params.space;
        result.programId = toBuffer(params.programId.toBuffer());
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.fromPubkey;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.newAccountPubkey;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static transfer(params) {
      let data;
      let keys;
      if ("basePubkey" in params) {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.TransferWithSeed;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.lamports = BigInt(params.lamports);
          result.seed = params.seed;
          result.programId = toBuffer(params.programId.toBuffer());
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.fromPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.basePubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.toPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })());
      } else {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.Transfer;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.lamports = BigInt(params.lamports);
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.fromPubkey;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.toPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })());
      }
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static assign(params) {
      let data;
      let keys;
      if ("basePubkey" in params) {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.AssignWithSeed;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.base = toBuffer(params.basePubkey.toBuffer());
          result.seed = params.seed;
          result.programId = toBuffer(params.programId.toBuffer());
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.accountPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.basePubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      } else {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.Assign;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.programId = toBuffer(params.programId.toBuffer());
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.accountPubkey;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })());
      }
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static createAccountWithSeed(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.CreateWithSeed;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.base = toBuffer(params.basePubkey.toBuffer());
        result.seed = params.seed;
        result.lamports = params.lamports;
        result.space = params.space;
        result.programId = toBuffer(params.programId.toBuffer());
        return result;
      })());
      let keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.fromPubkey;
        result.isSigner = true;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.newAccountPubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })());
      if (params.basePubkey != params.fromPubkey) {
        keys.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.basePubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      }
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static createNonceAccount(params) {
      const transaction = new Transaction();
      if ("basePubkey" in params && "seed" in params) {
        transaction.add(SystemProgram.createAccountWithSeed($(function () {
          let result = $Object.create(null, undefined);
          result.fromPubkey = params.fromPubkey;
          result.newAccountPubkey = params.noncePubkey;
          result.basePubkey = params.basePubkey;
          result.seed = params.seed;
          result.lamports = params.lamports;
          result.space = NONCE_ACCOUNT_LENGTH;
          result.programId = this.programId;
          return result;
        }).bind(this)()));
      } else {
        transaction.add(SystemProgram.createAccount($(function () {
          let result = $Object.create(null, undefined);
          result.fromPubkey = params.fromPubkey;
          result.newAccountPubkey = params.noncePubkey;
          result.lamports = params.lamports;
          result.space = NONCE_ACCOUNT_LENGTH;
          result.programId = this.programId;
          return result;
        }).bind(this)()));
      }
      const initParams = $(function () {
        let result = $Object.create(null, undefined);
        result.noncePubkey = params.noncePubkey;
        result.authorizedPubkey = params.authorizedPubkey;
        return result;
      })();
      transaction.add(this.nonceInitialize(initParams));
      return transaction;
    }
    static nonceInitialize(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.InitializeNonceAccount;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.authorized = toBuffer(params.authorizedPubkey.toBuffer());
        return result;
      })());
      const instructionData = $(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.noncePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RECENT_BLOCKHASHES_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RENT_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)();
      return new TransactionInstruction(instructionData);
    }
    static nonceAdvance(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.AdvanceNonceAccount;
      const data = encodeData(type);
      const instructionData = $(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.noncePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RECENT_BLOCKHASHES_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)();
      return new TransactionInstruction(instructionData);
    }
    static nonceWithdraw(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.WithdrawNonceAccount;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.lamports = params.lamports;
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.noncePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.toPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RECENT_BLOCKHASHES_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RENT_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static nonceAuthorize(params) {
      const type = SYSTEM_INSTRUCTION_LAYOUTS.AuthorizeNonceAccount;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.authorized = toBuffer(params.newAuthorizedPubkey.toBuffer());
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.noncePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static allocate(params) {
      let data;
      let keys;
      if ("basePubkey" in params) {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.AllocateWithSeed;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.base = toBuffer(params.basePubkey.toBuffer());
          result.seed = params.seed;
          result.space = params.space;
          result.programId = toBuffer(params.programId.toBuffer());
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.accountPubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.basePubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      } else {
        const type = SYSTEM_INSTRUCTION_LAYOUTS.Allocate;
        data = encodeData(type, $(function () {
          let result = $Object.create(null, undefined);
          result.space = params.space;
          return result;
        })());
        keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.accountPubkey;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })());
      }
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
  }
  SystemProgram.programId = new PublicKey("11111111111111111111111111111111");
  const CHUNK_SIZE = PACKET_DATA_SIZE - 300;
  class Loader {
    constructor() {}
    static getMinNumSignatures(dataLength) {
      return 2 * (Math.ceil(dataLength / Loader.chunkSize) + 1 + 1);
    }
    static async load(connection, payer, program, programId, data) {
      {
        const balanceNeeded = await connection.getMinimumBalanceForRentExemption(data.length);
        const programInfo = await connection.getAccountInfo(program.publicKey, "confirmed");
        let transaction = null;
        if (programInfo !== null) {
          if (programInfo.executable) {
            console.error("Program load failed, account is already executable");
            return false;
          }
          if (programInfo.data.length !== data.length) {
            transaction = transaction || new Transaction();
            transaction.add(SystemProgram.allocate($(function () {
              let result = $Object.create(null, undefined);
              result.accountPubkey = program.publicKey;
              result.space = data.length;
              return result;
            })()));
          }
          if (!programInfo.owner.equals(programId)) {
            transaction = transaction || new Transaction();
            transaction.add(SystemProgram.assign($(function () {
              let result = $Object.create(null, undefined);
              result.accountPubkey = program.publicKey;
              result.programId = programId;
              return result;
            })()));
          }
          if (programInfo.lamports < balanceNeeded) {
            transaction = transaction || new Transaction();
            transaction.add(SystemProgram.transfer($(function () {
              let result = $Object.create(null, undefined);
              result.fromPubkey = payer.publicKey;
              result.toPubkey = program.publicKey;
              result.lamports = balanceNeeded - programInfo.lamports;
              return result;
            })()));
          }
        } else {
          transaction = new Transaction().add(SystemProgram.createAccount($(function () {
            let result = $Object.create(null, undefined);
            result.fromPubkey = payer.publicKey;
            result.newAccountPubkey = program.publicKey;
            result.lamports = balanceNeeded > 0 ? balanceNeeded : 1;
            result.space = data.length;
            result.programId = programId;
            return result;
          })()));
        }
        if (transaction !== null) {
          await sendAndConfirmTransaction(connection, transaction, $Array.of(payer, program), $(function () {
            let result = $Object.create(null, undefined);
            result.commitment = "confirmed";
            return result;
          })());
        }
      }
      const dataLayout = struct($Array.of(u32("instruction"), u32("offset"), u32("bytesLength"), u32("bytesLengthPadding"), seq(u8("byte"), offset(u32(), -8), "bytes")));
      const chunkSize = Loader.chunkSize;
      let offset$1 = 0;
      let array = data;
      let transactions = $Array.of();
      while (array.length > 0) {
        const bytes = array.slice(0, chunkSize);
        const data = buffer.Buffer.alloc(chunkSize + 16);
        dataLayout.encode($(function () {
          let result = $Object.create(null, undefined);
          result.instruction = 0;
          result.offset = offset$1;
          result.bytes = bytes;
          result.bytesLength = 0;
          result.bytesLengthPadding = 0;
          return result;
        })(), data);
        const transaction = new Transaction().add($(function () {
          let result = $Object.create(null, undefined);
          result.keys = $Array.of($(function () {
            let result = $Object.create(null, undefined);
            result.pubkey = program.publicKey;
            result.isSigner = true;
            result.isWritable = true;
            return result;
          })());
          result.programId = programId;
          result.data = data;
          return result;
        })());
        transactions.push(sendAndConfirmTransaction(connection, transaction, $Array.of(payer, program), $(function () {
          let result = $Object.create(null, undefined);
          result.commitment = "confirmed";
          return result;
        })()));
        if (connection._rpcEndpoint.includes("solana.com")) {
          const REQUESTS_PER_SECOND = 4;
          await sleep(1e3 / REQUESTS_PER_SECOND);
        }
        offset$1 += chunkSize;
        array = array.slice(chunkSize);
      }
      await Promise.all(transactions);
      {
        const dataLayout = struct($Array.of(u32("instruction")));
        const data = buffer.Buffer.alloc(dataLayout.span);
        dataLayout.encode($(function () {
          let result = $Object.create(null, undefined);
          result.instruction = 1;
          return result;
        })(), data);
        const transaction = new Transaction().add($(function () {
          let result = $Object.create(null, undefined);
          result.keys = $Array.of($(function () {
            let result = $Object.create(null, undefined);
            result.pubkey = program.publicKey;
            result.isSigner = true;
            result.isWritable = true;
            return result;
          })(), $(function () {
            let result = $Object.create(null, undefined);
            result.pubkey = SYSVAR_RENT_PUBKEY;
            result.isSigner = false;
            result.isWritable = false;
            return result;
          })());
          result.programId = programId;
          result.data = data;
          return result;
        })());
        const deployCommitment = "processed";
        const finalizeSignature = await connection.sendTransaction(transaction, $Array.of(payer, program), $(function () {
          let result = $Object.create(null, undefined);
          result.preflightCommitment = deployCommitment;
          return result;
        })());
        const {
          context: context,
          value: value
        } = await connection.confirmTransaction($(function () {
          let result = $Object.create(null, undefined);
          result.signature = finalizeSignature;
          result.lastValidBlockHeight = transaction.lastValidBlockHeight;
          result.blockhash = transaction.recentBlockhash;
          return result;
        })(), deployCommitment);
        if (value.err) {
          throw new Error(`Transaction ${finalizeSignature} failed (${JSON.stringify(value)})`);
        }
        while (true) {
          try {
            const currentSlot = await connection.getSlot($(function () {
              let result = $Object.create(null, undefined);
              result.commitment = deployCommitment;
              return result;
            })());
            if (currentSlot > context.slot) {
              break;
            }
          } catch {}
          await new Promise($(resolve => setTimeout(resolve, Math.round(MS_PER_SLOT / 2))));
        }
      }
      return true;
    }
  }
  Loader.chunkSize = CHUNK_SIZE;
  const BPF_LOADER_PROGRAM_ID = new PublicKey("BPFLoader2111111111111111111111111111111111");
  class BpfLoader {
    static getMinNumSignatures(dataLength) {
      return Loader.getMinNumSignatures(dataLength);
    }
    static load(connection, payer, program, elf, loaderProgramId) {
      return Loader.load(connection, payer, program, loaderProgramId, elf);
    }
  }
  var objToString = $Object.prototype.toString;
  var objKeys = $Object.keys || $(function (obj) {
    var keys = $Array.of();
    for (var name in obj) {
      keys.push(name);
    }
    return keys;
  });
  function stringify$1(val, isArrayProp) {
    var i, max, str, keys, key, propVal, toStr;
    if (val === true) {
      return "true";
    }
    if (val === false) {
      return "false";
    }
    switch (typeof val) {
      case "object":
        if (val === null) {
          return null;
        } else if (val.toJSON && typeof val.toJSON === "function") {
          return stringify$1(val.toJSON(), isArrayProp);
        } else {
          toStr = objToString.call(val);
          if (toStr === "[object Array]") {
            str = "[";
            max = val.length - 1;
            for (i = 0; i < max; i++) {
              str += stringify$1(val[i], true) + ",";
            }
            if (max > -1) {
              str += stringify$1(val[i], true);
            }
            return str + "]";
          } else if (toStr === "[object Object]") {
            keys = objKeys(val).sort();
            max = keys.length;
            str = "";
            i = 0;
            while (i < max) {
              key = keys[i];
              propVal = stringify$1(val[key], false);
              if (propVal !== undefined) {
                if (str) {
                  str += ",";
                }
                str += JSON.stringify(key) + ":" + propVal;
              }
              i++;
            }
            return "{" + str + "}";
          } else {
            return JSON.stringify(val);
          }
        }
      case "function":
      case "undefined":
        return isArrayProp ? null : undefined;
      case "string":
        return JSON.stringify(val);
      default:
        return isFinite(val) ? val : null;
    }
  }
  $(stringify$1);
  var fastStableStringify = function (val) {
    var returnVal = stringify$1(val, false);
    if (returnVal !== undefined) {
      return "" + returnVal;
    }
  };
  $(fastStableStringify);
  var fastStableStringify$1 = getDefaultExportFromCjs(fastStableStringify);
  class StructError extends TypeError {
    constructor(failure, failures) {
      let cached;
      const {
        message: message,
        ...rest
      } = failure;
      const {
        path: path
      } = failure;
      const msg = path.length === 0 ? message : "At path: " + path.join(".") + " -- " + message;
      super(msg);
      $Object.assign(this, rest);
      this.name = this.constructor.name;
      this.failures = $(() => {
        var _cached;
        return (_cached = cached) != null ? _cached : cached = $Array.of(failure, ...failures());
      });
    }
  }
  function isIterable(x) {
    return isObject(x) && typeof x[Symbol.iterator] === "function";
  }
  $(isIterable);
  function isObject(x) {
    return typeof x === "object" && x != null;
  }
  $(isObject);
  function print(value) {
    return typeof value === "string" ? JSON.stringify(value) : "" + value;
  }
  $(print);
  function shiftIterator(input) {
    const {
      done: done,
      value: value
    } = input.next();
    return done ? undefined : value;
  }
  $(shiftIterator);
  function toFailure(result, context, struct, value) {
    if (result === true) {
      return;
    } else if (result === false) {
      result = $Object.create(null, undefined);
    } else if (typeof result === "string") {
      result = $(function () {
        let result = $Object.create(null, undefined);
        result.message = result;
        return result;
      })();
    }
    const {
      path: path,
      branch: branch
    } = context;
    const {
      type: type
    } = struct;
    const {
      refinement: refinement,
      message = "Expected a value of type `" + type + "`" + (refinement ? " with refinement `" + refinement + "`" : "") + ", but received: `" + print(value) + "`"
    } = result;
    return $(function () {
      let result = $Object.create(null, undefined);
      result.value = value;
      result.type = type;
      result.refinement = refinement;
      result.key = path[path.length - 1];
      result.path = path;
      result.branch = branch;
      $Object.assign(result, result);
      result.message = message;
      return result;
    })();
  }
  $(toFailure);
  function* toFailures(result, context, struct, value) {
    if (!isIterable(result)) {
      result = $Array.of(result);
    }
    for (const r of result) {
      const failure = toFailure(r, context, struct, value);
      if (failure) {
        yield failure;
      }
    }
  }
  $(toFailures);
  function* run(value, struct, options = $Object.create(null, undefined)) {
    const {
      path = $Array.of(),
      branch = $Array.of(value),
      coerce = false,
      mask = false
    } = options;
    const ctx = $(function () {
      let result = $Object.create(null, undefined);
      result.path = path;
      result.branch = branch;
      return result;
    })();
    if (coerce) {
      value = struct.coercer(value, ctx);
      if (mask && struct.type !== "type" && isObject(struct.schema) && isObject(value) && !$Array.isArray(value)) {
        for (const key in value) {
          if (struct.schema[key] === undefined) {
            delete value[key];
          }
        }
      }
    }
    let valid = true;
    for (const failure of struct.validator(value, ctx)) {
      valid = false;
      yield $Array.of(failure, undefined);
    }
    for (let [k, v, s] of struct.entries(value, ctx)) {
      const ts = run(v, s, $(function () {
        let result = $Object.create(null, undefined);
        result.path = k === undefined ? path : $Array.of(...path, k);
        result.branch = k === undefined ? branch : $Array.of(...branch, v);
        result.coerce = coerce;
        result.mask = mask;
        return result;
      })());
      for (const t of ts) {
        if (t[0]) {
          valid = false;
          yield $Array.of(t[0], undefined);
        } else if (coerce) {
          v = t[1];
          if (k === undefined) {
            value = v;
          } else if (value instanceof Map) {
            value.set(k, v);
          } else if (value instanceof Set) {
            value.add(v);
          } else if (isObject(value)) {
            value[k] = v;
          }
        }
      }
    }
    if (valid) {
      for (const failure of struct.refiner(value, ctx)) {
        valid = false;
        yield $Array.of(failure, undefined);
      }
    }
    if (valid) {
      yield $Array.of(undefined, value);
    }
  }
  $(run);
  class Struct {
    constructor(props) {
      const {
        type: type,
        schema: schema,
        validator: validator,
        refiner: refiner,
        coercer = $(value => value),
        entries = $(function* () {})
      } = props;
      this.type = type;
      this.schema = schema;
      this.entries = entries;
      this.coercer = coercer;
      if (validator) {
        this.validator = $((value, context) => {
          const result = validator(value, context);
          return toFailures(result, context, this, value);
        });
      } else {
        this.validator = $(() => $Array.of());
      }
      if (refiner) {
        this.refiner = $((value, context) => {
          const result = refiner(value, context);
          return toFailures(result, context, this, value);
        });
      } else {
        this.refiner = $(() => $Array.of());
      }
    }
    assert(value) {
      return assert(value, this);
    }
    create(value) {
      return create(value, this);
    }
    is(value) {
      return is(value, this);
    }
    mask(value) {
      return mask(value, this);
    }
    validate(value, options = $Object.create(null, undefined)) {
      return validate$1(value, this, options);
    }
  }
  function assert(value, struct) {
    const result = validate$1(value, struct);
    if (result[0]) {
      throw result[0];
    }
  }
  $(assert);
  function create(value, struct) {
    const result = validate$1(value, struct, $(function () {
      let result = $Object.create(null, undefined);
      result.coerce = true;
      return result;
    })());
    if (result[0]) {
      throw result[0];
    } else {
      return result[1];
    }
  }
  $(create);
  function mask(value, struct) {
    const result = validate$1(value, struct, $(function () {
      let result = $Object.create(null, undefined);
      result.coerce = true;
      result.mask = true;
      return result;
    })());
    if (result[0]) {
      throw result[0];
    } else {
      return result[1];
    }
  }
  $(mask);
  function is(value, struct) {
    const result = validate$1(value, struct);
    return !result[0];
  }
  $(is);
  function validate$1(value, struct, options = $Object.create(null, undefined)) {
    const tuples = run(value, struct, options);
    const tuple = shiftIterator(tuples);
    if (tuple[0]) {
      const error = new StructError(tuple[0], $(function* () {
        for (const t of tuples) {
          if (t[0]) {
            yield t[0];
          }
        }
      }));
      return $Array.of(error, undefined);
    } else {
      const v = tuple[1];
      return $Array.of(undefined, v);
    }
  }
  $(validate$1);
  function define(name, validator) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = name;
      result.schema = null;
      result.validator = validator;
      return result;
    })());
  }
  $(define);
  function any() {
    return define("any", $(() => true));
  }
  $(any);
  function array(Element) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "array";
      result.schema = Element;
      result.entries = $(function* (value) {
        if (Element && $Array.isArray(value)) {
          for (const [i, v] of value.entries()) {
            yield $Array.of(i, v, Element);
          }
        }
      });
      result.coercer = $(function (value) {
        return $Array.isArray(value) ? value.slice() : value;
      });
      result.validator = $(function (value) {
        return $Array.isArray(value) || "Expected an array value, but received: " + print(value);
      });
      return result;
    })());
  }
  $(array);
  function boolean() {
    return define("boolean", $(value => typeof value === "boolean"));
  }
  $(boolean);
  function instance(Class) {
    return define("instance", $(value => value instanceof Class || "Expected a `" + Class.name + "` instance, but received: " + print(value)));
  }
  $(instance);
  function literal(constant) {
    const description = print(constant);
    const t = typeof constant;
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "literal";
      result.schema = t === "string" || t === "number" || t === "boolean" ? constant : null;
      result.validator = $(function (value) {
        return value === constant || "Expected the literal `" + description + "`, but received: " + print(value);
      });
      return result;
    })());
  }
  $(literal);
  function never() {
    return define("never", $(() => false));
  }
  $(never);
  function nullable(struct) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, struct);
      result.validator = $((value, ctx) => value === null || struct.validator(value, ctx));
      result.refiner = $((value, ctx) => value === null || struct.refiner(value, ctx));
      return result;
    })());
  }
  $(nullable);
  function number() {
    return define("number", $(value => typeof value === "number" && !isNaN(value) || "Expected a number, but received: " + print(value)));
  }
  $(number);
  function optional(struct) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, struct);
      result.validator = $((value, ctx) => value === undefined || struct.validator(value, ctx));
      result.refiner = $((value, ctx) => value === undefined || struct.refiner(value, ctx));
      return result;
    })());
  }
  $(optional);
  function record(Key, Value) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "record";
      result.schema = null;
      result.entries = $(function* (value) {
        if (isObject(value)) {
          for (const k in value) {
            const v = value[k];
            yield $Array.of(k, k, Key);
            yield $Array.of(k, v, Value);
          }
        }
      });
      result.validator = $(function (value) {
        return isObject(value) || "Expected an object, but received: " + print(value);
      });
      return result;
    })());
  }
  $(record);
  function string() {
    return define("string", $(value => typeof value === "string" || "Expected a string, but received: " + print(value)));
  }
  $(string);
  function tuple(Elements) {
    const Never = never();
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "tuple";
      result.schema = null;
      result.entries = $(function* (value) {
        if ($Array.isArray(value)) {
          const length = Math.max(Elements.length, value.length);
          for (let i = 0; i < length; i++) {
            yield $Array.of(i, value[i], Elements[i] || Never);
          }
        }
      });
      result.validator = $(function (value) {
        return $Array.isArray(value) || "Expected an array, but received: " + print(value);
      });
      return result;
    })());
  }
  $(tuple);
  function type(schema) {
    const keys = $Object.keys(schema);
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "type";
      result.schema = schema;
      result.entries = $(function* (value) {
        if (isObject(value)) {
          for (const k of keys) {
            yield $Array.of(k, value[k], schema[k]);
          }
        }
      });
      result.validator = $(function (value) {
        return isObject(value) || "Expected an object, but received: " + print(value);
      });
      return result;
    })());
  }
  $(type);
  function union(Structs) {
    const description = Structs.map($(s => s.type)).join(" | ");
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      result.type = "union";
      result.schema = null;
      result.validator = $(function (value, ctx) {
        const failures = $Array.of();
        for (const S of Structs) {
          const [...tuples] = run(value, S, ctx);
          const [first] = tuples;
          if (!first[0]) {
            return $Array.of();
          } else {
            for (const [failure] of tuples) {
              if (failure) {
                failures.push(failure);
              }
            }
          }
        }
        return $Array.of("Expected the value to satisfy a union of `" + description + "`, but received: " + print(value), ...failures);
      });
      return result;
    })());
  }
  $(union);
  function unknown() {
    return define("unknown", $(() => true));
  }
  $(unknown);
  function coerce(struct, condition, coercer) {
    return new Struct($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, struct);
      result.coercer = $((value, ctx) => is(value, condition) ? struct.coercer(coercer(value, ctx), ctx) : struct.coercer(value, ctx));
      return result;
    })());
  }
  $(coerce);
  var getRandomValues;
  var rnds8 = new Uint8Array(16);
  function rng() {
    if (!getRandomValues) {
      getRandomValues = typeof crypto !== "undefined" && crypto.getRandomValues && crypto.getRandomValues.bind(crypto) || typeof msCrypto !== "undefined" && typeof msCrypto.getRandomValues === "function" && msCrypto.getRandomValues.bind(msCrypto);
      if (!getRandomValues) {
        throw new Error("crypto.getRandomValues() not supported. See https://github.com/uuidjs/uuid#getrandomvalues-not-supported");
      }
    }
    return getRandomValues(rnds8);
  }
  $(rng);
  var REGEX = /^(?:[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}|00000000-0000-0000-0000-000000000000)$/i;
  function validate(uuid) {
    return typeof uuid === "string" && REGEX.test(uuid);
  }
  $(validate);
  var byteToHex = $Array.of();
  for (var i = 0; i < 256; ++i) {
    byteToHex.push((i + 256).toString(16).substr(1));
  }
  function stringify(arr) {
    var offset = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : 0;
    var uuid = (byteToHex[arr[offset + 0]] + byteToHex[arr[offset + 1]] + byteToHex[arr[offset + 2]] + byteToHex[arr[offset + 3]] + "-" + byteToHex[arr[offset + 4]] + byteToHex[arr[offset + 5]] + "-" + byteToHex[arr[offset + 6]] + byteToHex[arr[offset + 7]] + "-" + byteToHex[arr[offset + 8]] + byteToHex[arr[offset + 9]] + "-" + byteToHex[arr[offset + 10]] + byteToHex[arr[offset + 11]] + byteToHex[arr[offset + 12]] + byteToHex[arr[offset + 13]] + byteToHex[arr[offset + 14]] + byteToHex[arr[offset + 15]]).toLowerCase();
    if (!validate(uuid)) {
      throw TypeError("Stringified UUID is invalid");
    }
    return uuid;
  }
  $(stringify);
  var _nodeId;
  var _clockseq;
  var _lastMSecs = 0;
  var _lastNSecs = 0;
  function v1(options, buf, offset) {
    var i = buf && offset || 0;
    var b = buf || new Array(16);
    options = options || $Object.create(null, undefined);
    var node = options.node || _nodeId;
    var clockseq = options.clockseq !== undefined ? options.clockseq : _clockseq;
    if (node == null || clockseq == null) {
      var seedBytes = options.random || (options.rng || rng)();
      if (node == null) {
        node = _nodeId = $Array.of(seedBytes[0] | 1, seedBytes[1], seedBytes[2], seedBytes[3], seedBytes[4], seedBytes[5]);
      }
      if (clockseq == null) {
        clockseq = _clockseq = (seedBytes[6] << 8 | seedBytes[7]) & 16383;
      }
    }
    var msecs = options.msecs !== undefined ? options.msecs : Date.now();
    var nsecs = options.nsecs !== undefined ? options.nsecs : _lastNSecs + 1;
    var dt = msecs - _lastMSecs + (nsecs - _lastNSecs) / 1e4;
    if (dt < 0 && options.clockseq === undefined) {
      clockseq = clockseq + 1 & 16383;
    }
    if ((dt < 0 || msecs > _lastMSecs) && options.nsecs === undefined) {
      nsecs = 0;
    }
    if (nsecs >= 1e4) {
      throw new Error("uuid.v1(): Can't create more than 10M uuids/sec");
    }
    _lastMSecs = msecs;
    _lastNSecs = nsecs;
    _clockseq = clockseq;
    msecs += 122192928e5;
    var tl = ((msecs & 268435455) * 1e4 + nsecs) % 4294967296;
    b[i++] = tl >>> 24 & 255;
    b[i++] = tl >>> 16 & 255;
    b[i++] = tl >>> 8 & 255;
    b[i++] = tl & 255;
    var tmh = msecs / 4294967296 * 1e4 & 268435455;
    b[i++] = tmh >>> 8 & 255;
    b[i++] = tmh & 255;
    b[i++] = tmh >>> 24 & 15 | 16;
    b[i++] = tmh >>> 16 & 255;
    b[i++] = clockseq >>> 8 | 128;
    b[i++] = clockseq & 255;
    for (var n = 0; n < 6; ++n) {
      b[i + n] = node[n];
    }
    return buf || stringify(b);
  }
  $(v1);
  function parse(uuid) {
    if (!validate(uuid)) {
      throw TypeError("Invalid UUID");
    }
    var v;
    var arr = new Uint8Array(16);
    arr[0] = (v = parseInt(uuid.slice(0, 8), 16)) >>> 24;
    arr[1] = v >>> 16 & 255;
    arr[2] = v >>> 8 & 255;
    arr[3] = v & 255;
    arr[4] = (v = parseInt(uuid.slice(9, 13), 16)) >>> 8;
    arr[5] = v & 255;
    arr[6] = (v = parseInt(uuid.slice(14, 18), 16)) >>> 8;
    arr[7] = v & 255;
    arr[8] = (v = parseInt(uuid.slice(19, 23), 16)) >>> 8;
    arr[9] = v & 255;
    arr[10] = (v = parseInt(uuid.slice(24, 36), 16)) / 1099511627776 & 255;
    arr[11] = v / 4294967296 & 255;
    arr[12] = v >>> 24 & 255;
    arr[13] = v >>> 16 & 255;
    arr[14] = v >>> 8 & 255;
    arr[15] = v & 255;
    return arr;
  }
  $(parse);
  function stringToBytes(str) {
    str = unescape(encodeURIComponent(str));
    var bytes = $Array.of();
    for (var i = 0; i < str.length; ++i) {
      bytes.push(str.charCodeAt(i));
    }
    return bytes;
  }
  $(stringToBytes);
  var DNS = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
  var URL = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";
  function v35(name, version, hashfunc) {
    function generateUUID(value, namespace, buf, offset) {
      if (typeof value === "string") {
        value = stringToBytes(value);
      }
      if (typeof namespace === "string") {
        namespace = parse(namespace);
      }
      if (namespace.length !== 16) {
        throw TypeError("Namespace must be array-like (16 iterable integer values, 0-255)");
      }
      var bytes = new Uint8Array(16 + value.length);
      bytes.set(namespace);
      bytes.set(value, namespace.length);
      bytes = hashfunc(bytes);
      bytes[6] = bytes[6] & 15 | version;
      bytes[8] = bytes[8] & 63 | 128;
      if (buf) {
        offset = offset || 0;
        for (var i = 0; i < 16; ++i) {
          buf[offset + i] = bytes[i];
        }
        return buf;
      }
      return stringify(bytes);
    }
    $(generateUUID);
    try {
      generateUUID.name = name;
    } catch (err) {}
    generateUUID.DNS = DNS;
    generateUUID.URL = URL;
    return generateUUID;
  }
  $(v35);
  function md5(bytes) {
    if (typeof bytes === "string") {
      var msg = unescape(encodeURIComponent(bytes));
      bytes = new Uint8Array(msg.length);
      for (var i = 0; i < msg.length; ++i) {
        bytes[i] = msg.charCodeAt(i);
      }
    }
    return md5ToHexEncodedArray(wordsToMd5(bytesToWords(bytes), bytes.length * 8));
  }
  $(md5);
  function md5ToHexEncodedArray(input) {
    var output = $Array.of();
    var length32 = input.length * 32;
    var hexTab = "0123456789abcdef";
    for (var i = 0; i < length32; i += 8) {
      var x = input[i >> 5] >>> i % 32 & 255;
      var hex = parseInt(hexTab.charAt(x >>> 4 & 15) + hexTab.charAt(x & 15), 16);
      output.push(hex);
    }
    return output;
  }
  $(md5ToHexEncodedArray);
  function getOutputLength(inputLength8) {
    return (inputLength8 + 64 >>> 9 << 4) + 14 + 1;
  }
  $(getOutputLength);
  function wordsToMd5(x, len) {
    x[len >> 5] |= 128 << len % 32;
    x[getOutputLength(len) - 1] = len;
    var a = 1732584193;
    var b = -271733879;
    var c = -1732584194;
    var d = 271733878;
    for (var i = 0; i < x.length; i += 16) {
      var olda = a;
      var oldb = b;
      var oldc = c;
      var oldd = d;
      a = md5ff(a, b, c, d, x[i], 7, -680876936);
      d = md5ff(d, a, b, c, x[i + 1], 12, -389564586);
      c = md5ff(c, d, a, b, x[i + 2], 17, 606105819);
      b = md5ff(b, c, d, a, x[i + 3], 22, -1044525330);
      a = md5ff(a, b, c, d, x[i + 4], 7, -176418897);
      d = md5ff(d, a, b, c, x[i + 5], 12, 1200080426);
      c = md5ff(c, d, a, b, x[i + 6], 17, -1473231341);
      b = md5ff(b, c, d, a, x[i + 7], 22, -45705983);
      a = md5ff(a, b, c, d, x[i + 8], 7, 1770035416);
      d = md5ff(d, a, b, c, x[i + 9], 12, -1958414417);
      c = md5ff(c, d, a, b, x[i + 10], 17, -42063);
      b = md5ff(b, c, d, a, x[i + 11], 22, -1990404162);
      a = md5ff(a, b, c, d, x[i + 12], 7, 1804603682);
      d = md5ff(d, a, b, c, x[i + 13], 12, -40341101);
      c = md5ff(c, d, a, b, x[i + 14], 17, -1502002290);
      b = md5ff(b, c, d, a, x[i + 15], 22, 1236535329);
      a = md5gg(a, b, c, d, x[i + 1], 5, -165796510);
      d = md5gg(d, a, b, c, x[i + 6], 9, -1069501632);
      c = md5gg(c, d, a, b, x[i + 11], 14, 643717713);
      b = md5gg(b, c, d, a, x[i], 20, -373897302);
      a = md5gg(a, b, c, d, x[i + 5], 5, -701558691);
      d = md5gg(d, a, b, c, x[i + 10], 9, 38016083);
      c = md5gg(c, d, a, b, x[i + 15], 14, -660478335);
      b = md5gg(b, c, d, a, x[i + 4], 20, -405537848);
      a = md5gg(a, b, c, d, x[i + 9], 5, 568446438);
      d = md5gg(d, a, b, c, x[i + 14], 9, -1019803690);
      c = md5gg(c, d, a, b, x[i + 3], 14, -187363961);
      b = md5gg(b, c, d, a, x[i + 8], 20, 1163531501);
      a = md5gg(a, b, c, d, x[i + 13], 5, -1444681467);
      d = md5gg(d, a, b, c, x[i + 2], 9, -51403784);
      c = md5gg(c, d, a, b, x[i + 7], 14, 1735328473);
      b = md5gg(b, c, d, a, x[i + 12], 20, -1926607734);
      a = md5hh(a, b, c, d, x[i + 5], 4, -378558);
      d = md5hh(d, a, b, c, x[i + 8], 11, -2022574463);
      c = md5hh(c, d, a, b, x[i + 11], 16, 1839030562);
      b = md5hh(b, c, d, a, x[i + 14], 23, -35309556);
      a = md5hh(a, b, c, d, x[i + 1], 4, -1530992060);
      d = md5hh(d, a, b, c, x[i + 4], 11, 1272893353);
      c = md5hh(c, d, a, b, x[i + 7], 16, -155497632);
      b = md5hh(b, c, d, a, x[i + 10], 23, -1094730640);
      a = md5hh(a, b, c, d, x[i + 13], 4, 681279174);
      d = md5hh(d, a, b, c, x[i], 11, -358537222);
      c = md5hh(c, d, a, b, x[i + 3], 16, -722521979);
      b = md5hh(b, c, d, a, x[i + 6], 23, 76029189);
      a = md5hh(a, b, c, d, x[i + 9], 4, -640364487);
      d = md5hh(d, a, b, c, x[i + 12], 11, -421815835);
      c = md5hh(c, d, a, b, x[i + 15], 16, 530742520);
      b = md5hh(b, c, d, a, x[i + 2], 23, -995338651);
      a = md5ii(a, b, c, d, x[i], 6, -198630844);
      d = md5ii(d, a, b, c, x[i + 7], 10, 1126891415);
      c = md5ii(c, d, a, b, x[i + 14], 15, -1416354905);
      b = md5ii(b, c, d, a, x[i + 5], 21, -57434055);
      a = md5ii(a, b, c, d, x[i + 12], 6, 1700485571);
      d = md5ii(d, a, b, c, x[i + 3], 10, -1894986606);
      c = md5ii(c, d, a, b, x[i + 10], 15, -1051523);
      b = md5ii(b, c, d, a, x[i + 1], 21, -2054922799);
      a = md5ii(a, b, c, d, x[i + 8], 6, 1873313359);
      d = md5ii(d, a, b, c, x[i + 15], 10, -30611744);
      c = md5ii(c, d, a, b, x[i + 6], 15, -1560198380);
      b = md5ii(b, c, d, a, x[i + 13], 21, 1309151649);
      a = md5ii(a, b, c, d, x[i + 4], 6, -145523070);
      d = md5ii(d, a, b, c, x[i + 11], 10, -1120210379);
      c = md5ii(c, d, a, b, x[i + 2], 15, 718787259);
      b = md5ii(b, c, d, a, x[i + 9], 21, -343485551);
      a = safeAdd(a, olda);
      b = safeAdd(b, oldb);
      c = safeAdd(c, oldc);
      d = safeAdd(d, oldd);
    }
    return $Array.of(a, b, c, d);
  }
  $(wordsToMd5);
  function bytesToWords(input) {
    if (input.length === 0) {
      return $Array.of();
    }
    var length8 = input.length * 8;
    var output = new Uint32Array(getOutputLength(length8));
    for (var i = 0; i < length8; i += 8) {
      output[i >> 5] |= (input[i / 8] & 255) << i % 32;
    }
    return output;
  }
  $(bytesToWords);
  function safeAdd(x, y) {
    var lsw = (x & 65535) + (y & 65535);
    var msw = (x >> 16) + (y >> 16) + (lsw >> 16);
    return msw << 16 | lsw & 65535;
  }
  $(safeAdd);
  function bitRotateLeft(num, cnt) {
    return num << cnt | num >>> 32 - cnt;
  }
  $(bitRotateLeft);
  function md5cmn(q, a, b, x, s, t) {
    return safeAdd(bitRotateLeft(safeAdd(safeAdd(a, q), safeAdd(x, t)), s), b);
  }
  $(md5cmn);
  function md5ff(a, b, c, d, x, s, t) {
    return md5cmn(b & c | ~b & d, a, b, x, s, t);
  }
  $(md5ff);
  function md5gg(a, b, c, d, x, s, t) {
    return md5cmn(b & d | c & ~d, a, b, x, s, t);
  }
  $(md5gg);
  function md5hh(a, b, c, d, x, s, t) {
    return md5cmn(b ^ c ^ d, a, b, x, s, t);
  }
  $(md5hh);
  function md5ii(a, b, c, d, x, s, t) {
    return md5cmn(c ^ (b | ~d), a, b, x, s, t);
  }
  $(md5ii);
  var v3 = v35("v3", 48, md5);
  var v3$1 = v3;
  function v4(options, buf, offset) {
    options = options || $Object.create(null, undefined);
    var rnds = options.random || (options.rng || rng)();
    rnds[6] = rnds[6] & 15 | 64;
    rnds[8] = rnds[8] & 63 | 128;
    if (buf) {
      offset = offset || 0;
      for (var i = 0; i < 16; ++i) {
        buf[offset + i] = rnds[i];
      }
      return buf;
    }
    return stringify(rnds);
  }
  $(v4);
  function f(s, x, y, z) {
    switch (s) {
      case 0:
        return x & y ^ ~x & z;
      case 1:
        return x ^ y ^ z;
      case 2:
        return x & y ^ x & z ^ y & z;
      case 3:
        return x ^ y ^ z;
    }
  }
  $(f);
  function ROTL(x, n) {
    return x << n | x >>> 32 - n;
  }
  $(ROTL);
  function sha1(bytes) {
    var K = $Array.of(1518500249, 1859775393, 2400959708, 3395469782);
    var H = $Array.of(1732584193, 4023233417, 2562383102, 271733878, 3285377520);
    if (typeof bytes === "string") {
      var msg = unescape(encodeURIComponent(bytes));
      bytes = $Array.of();
      for (var i = 0; i < msg.length; ++i) {
        bytes.push(msg.charCodeAt(i));
      }
    } else if (!$Array.isArray(bytes)) {
      bytes = $Array.prototype.slice.call(bytes);
    }
    bytes.push(128);
    var l = bytes.length / 4 + 2;
    var N = Math.ceil(l / 16);
    var M = new Array(N);
    for (var _i = 0; _i < N; ++_i) {
      var arr = new Uint32Array(16);
      for (var j = 0; j < 16; ++j) {
        arr[j] = bytes[_i * 64 + j * 4] << 24 | bytes[_i * 64 + j * 4 + 1] << 16 | bytes[_i * 64 + j * 4 + 2] << 8 | bytes[_i * 64 + j * 4 + 3];
      }
      M[_i] = arr;
    }
    M[N - 1][14] = (bytes.length - 1) * 8 / Math.pow(2, 32);
    M[N - 1][14] = Math.floor(M[N - 1][14]);
    M[N - 1][15] = (bytes.length - 1) * 8 & 4294967295;
    for (var _i2 = 0; _i2 < N; ++_i2) {
      var W = new Uint32Array(80);
      for (var t = 0; t < 16; ++t) {
        W[t] = M[_i2][t];
      }
      for (var _t = 16; _t < 80; ++_t) {
        W[_t] = ROTL(W[_t - 3] ^ W[_t - 8] ^ W[_t - 14] ^ W[_t - 16], 1);
      }
      var a = H[0];
      var b = H[1];
      var c = H[2];
      var d = H[3];
      var e = H[4];
      for (var _t2 = 0; _t2 < 80; ++_t2) {
        var s = Math.floor(_t2 / 20);
        var T = ROTL(a, 5) + f(s, b, c, d) + e + K[s] + W[_t2] >>> 0;
        e = d;
        d = c;
        c = ROTL(b, 30) >>> 0;
        b = a;
        a = T;
      }
      H[0] = H[0] + a >>> 0;
      H[1] = H[1] + b >>> 0;
      H[2] = H[2] + c >>> 0;
      H[3] = H[3] + d >>> 0;
      H[4] = H[4] + e >>> 0;
    }
    return $Array.of(H[0] >> 24 & 255, H[0] >> 16 & 255, H[0] >> 8 & 255, H[0] & 255, H[1] >> 24 & 255, H[1] >> 16 & 255, H[1] >> 8 & 255, H[1] & 255, H[2] >> 24 & 255, H[2] >> 16 & 255, H[2] >> 8 & 255, H[2] & 255, H[3] >> 24 & 255, H[3] >> 16 & 255, H[3] >> 8 & 255, H[3] & 255, H[4] >> 24 & 255, H[4] >> 16 & 255, H[4] >> 8 & 255, H[4] & 255);
  }
  $(sha1);
  var v5 = v35("v5", 80, sha1);
  var v5$1 = v5;
  var nil = "00000000-0000-0000-0000-000000000000";
  function version(uuid) {
    if (!validate(uuid)) {
      throw TypeError("Invalid UUID");
    }
    return parseInt(uuid.substr(14, 1), 16);
  }
  $(version);
  var esmBrowser = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.__proto__ = null;
    result.NIL = nil;
    result.parse = parse;
    result.stringify = stringify;
    result.v1 = v1;
    result.v3 = v3$1;
    result.v4 = v4;
    result.v5 = v5$1;
    result.validate = validate;
    result.version = version;
    return result;
  })());
  var require$$0 = getAugmentedNamespace(esmBrowser);
  const uuid$1 = require$$0.v4;
  const generateRequest$1 = function (method, params, id, options) {
    if (typeof method !== "string") {
      throw new TypeError(method + " must be a string");
    }
    options = options || $Object.create(null, undefined);
    const version = typeof options.version === "number" ? options.version : 2;
    if (version !== 1 && version !== 2) {
      throw new TypeError(version + " must be 1 or 2");
    }
    const request = $(function () {
      let result = $Object.create(null, undefined);
      result.method = method;
      return result;
    })();
    if (version === 2) {
      request.jsonrpc = "2.0";
    }
    if (params) {
      if (typeof params !== "object" && !$Array.isArray(params)) {
        throw new TypeError(params + " must be an object, array or omitted");
      }
      request.params = params;
    }
    if (typeof id === "undefined") {
      const generator = typeof options.generator === "function" ? options.generator : $(function () {
        return uuid$1();
      });
      request.id = generator(request, options);
    } else if (version === 2 && id === null) {
      if (options.notificationIdNull) {
        request.id = null;
      }
    } else {
      request.id = id;
    }
    return request;
  };
  $(generateRequest$1);
  var generateRequest_1 = generateRequest$1;
  const uuid = require$$0.v4;
  const generateRequest = generateRequest_1;
  const ClientBrowser = function (callServer, options) {
    if (!(this instanceof ClientBrowser)) {
      return new ClientBrowser(callServer, options);
    }
    if (!options) {
      options = $Object.create(null, undefined);
    }
    this.options = $(function () {
      let result = $Object.create(null, undefined);
      result.reviver = typeof options.reviver !== "undefined" ? options.reviver : null;
      result.replacer = typeof options.replacer !== "undefined" ? options.replacer : null;
      result.generator = typeof options.generator !== "undefined" ? options.generator : $(function () {
        return uuid();
      });
      result.version = typeof options.version !== "undefined" ? options.version : 2;
      result.notificationIdNull = typeof options.notificationIdNull === "boolean" ? options.notificationIdNull : false;
      return result;
    })();
    this.callServer = callServer;
  };
  $(ClientBrowser);
  var browser = ClientBrowser;
  ClientBrowser.prototype.request = $(function (method, params, id, callback) {
    const self = this;
    let request = null;
    const isBatch = $Array.isArray(method) && typeof params === "function";
    if (this.options.version === 1 && isBatch) {
      throw new TypeError("JSON-RPC 1.0 does not support batching");
    }
    const isRaw = !isBatch && method && typeof method === "object" && typeof params === "function";
    if (isBatch || isRaw) {
      callback = params;
      request = method;
    } else {
      if (typeof id === "function") {
        callback = id;
        id = undefined;
      }
      const hasCallback = typeof callback === "function";
      try {
        request = generateRequest(method, params, id, $(function () {
          let result = $Object.create(null, undefined);
          result.generator = this.options.generator;
          result.version = this.options.version;
          result.notificationIdNull = this.options.notificationIdNull;
          return result;
        })());
      } catch (err) {
        if (hasCallback) {
          return callback(err);
        }
        throw err;
      }
      if (!hasCallback) {
        return request;
      }
    }
    let message;
    try {
      message = JSON.stringify(request, this.options.replacer);
    } catch (err) {
      return callback(err);
    }
    this.callServer(message, $(function (err, response) {
      self._parseResponse(err, response, callback);
    }));
    return request;
  });
  ClientBrowser.prototype._parseResponse = $(function (err, responseText, callback) {
    if (err) {
      callback(err);
      return;
    }
    if (!responseText) {
      return callback();
    }
    let response;
    try {
      response = JSON.parse(responseText, this.options.reviver);
    } catch (err) {
      return callback(err);
    }
    if (callback.length === 3) {
      if ($Array.isArray(response)) {
        const isError = function (res) {
          return typeof res.error !== "undefined";
        };
        $(isError);
        const isNotError = function (res) {
          return !isError(res);
        };
        $(isNotError);
        return callback(null, response.filter(isError), response.filter(isNotError));
      } else {
        return callback(null, response.error, response.result);
      }
    }
    callback(null, response);
  });
  var RpcClient = getDefaultExportFromCjs(browser);
  const MINIMUM_SLOT_PER_EPOCH = 32;
  function trailingZeros(n) {
    let trailingZeros = 0;
    while (n > 1) {
      n /= 2;
      trailingZeros++;
    }
    return trailingZeros;
  }
  $(trailingZeros);
  function nextPowerOfTwo(n) {
    if (n === 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }
  $(nextPowerOfTwo);
  class EpochSchedule {
    constructor(slotsPerEpoch, leaderScheduleSlotOffset, warmup, firstNormalEpoch, firstNormalSlot) {
      this.slotsPerEpoch = void 0;
      this.leaderScheduleSlotOffset = void 0;
      this.warmup = void 0;
      this.firstNormalEpoch = void 0;
      this.firstNormalSlot = void 0;
      this.slotsPerEpoch = slotsPerEpoch;
      this.leaderScheduleSlotOffset = leaderScheduleSlotOffset;
      this.warmup = warmup;
      this.firstNormalEpoch = firstNormalEpoch;
      this.firstNormalSlot = firstNormalSlot;
    }
    getEpoch(slot) {
      return this.getEpochAndSlotIndex(slot)[0];
    }
    getEpochAndSlotIndex(slot) {
      if (slot < this.firstNormalSlot) {
        const epoch = trailingZeros(nextPowerOfTwo(slot + MINIMUM_SLOT_PER_EPOCH + 1)) - trailingZeros(MINIMUM_SLOT_PER_EPOCH) - 1;
        const epochLen = this.getSlotsInEpoch(epoch);
        const slotIndex = slot - (epochLen - MINIMUM_SLOT_PER_EPOCH);
        return $Array.of(epoch, slotIndex);
      } else {
        const normalSlotIndex = slot - this.firstNormalSlot;
        const normalEpochIndex = Math.floor(normalSlotIndex / this.slotsPerEpoch);
        const epoch = this.firstNormalEpoch + normalEpochIndex;
        const slotIndex = normalSlotIndex % this.slotsPerEpoch;
        return $Array.of(epoch, slotIndex);
      }
    }
    getFirstSlotInEpoch(epoch) {
      if (epoch <= this.firstNormalEpoch) {
        return (Math.pow(2, epoch) - 1) * MINIMUM_SLOT_PER_EPOCH;
      } else {
        return (epoch - this.firstNormalEpoch) * this.slotsPerEpoch + this.firstNormalSlot;
      }
    }
    getLastSlotInEpoch(epoch) {
      return this.getFirstSlotInEpoch(epoch) + this.getSlotsInEpoch(epoch) - 1;
    }
    getSlotsInEpoch(epoch) {
      if (epoch < this.firstNormalEpoch) {
        return Math.pow(2, epoch + trailingZeros(MINIMUM_SLOT_PER_EPOCH));
      } else {
        return this.slotsPerEpoch;
      }
    }
  }
  class SendTransactionError extends Error {
    constructor(message, logs) {
      super(message);
      this.logs = void 0;
      this.logs = logs;
    }
  }
  const SolanaJSONRPCErrorCode = $(function () {
    let result = $Object.create(null, undefined);
    result.JSON_RPC_SERVER_ERROR_BLOCK_CLEANED_UP = -32001;
    result.JSON_RPC_SERVER_ERROR_SEND_TRANSACTION_PREFLIGHT_FAILURE = -32002;
    result.JSON_RPC_SERVER_ERROR_TRANSACTION_SIGNATURE_VERIFICATION_FAILURE = -32003;
    result.JSON_RPC_SERVER_ERROR_BLOCK_NOT_AVAILABLE = -32004;
    result.JSON_RPC_SERVER_ERROR_NODE_UNHEALTHY = -32005;
    result.JSON_RPC_SERVER_ERROR_TRANSACTION_PRECOMPILE_VERIFICATION_FAILURE = -32006;
    result.JSON_RPC_SERVER_ERROR_SLOT_SKIPPED = -32007;
    result.JSON_RPC_SERVER_ERROR_NO_SNAPSHOT = -32008;
    result.JSON_RPC_SERVER_ERROR_LONG_TERM_STORAGE_SLOT_SKIPPED = -32009;
    result.JSON_RPC_SERVER_ERROR_KEY_EXCLUDED_FROM_SECONDARY_INDEX = -32010;
    result.JSON_RPC_SERVER_ERROR_TRANSACTION_HISTORY_NOT_AVAILABLE = -32011;
    result.JSON_RPC_SCAN_ERROR = -32012;
    result.JSON_RPC_SERVER_ERROR_TRANSACTION_SIGNATURE_LEN_MISMATCH = -32013;
    result.JSON_RPC_SERVER_ERROR_BLOCK_STATUS_NOT_AVAILABLE_YET = -32014;
    result.JSON_RPC_SERVER_ERROR_UNSUPPORTED_TRANSACTION_VERSION = -32015;
    result.JSON_RPC_SERVER_ERROR_MIN_CONTEXT_SLOT_NOT_REACHED = -32016;
    return result;
  })();
  class SolanaJSONRPCError extends Error {
    constructor({
      code: code,
      message: message,
      data: data
    }, customMessage) {
      super(customMessage != null ? `${customMessage}: ${message}` : message);
      this.code = void 0;
      this.data = void 0;
      this.code = code;
      this.data = data;
      this.name = "SolanaJSONRPCError";
    }
  }
  var fetchImpl = globalThis.fetch;
  var client = $Object.create(null, undefined);
  var interopRequireDefault = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  $(function (module) {
    function _interopRequireDefault(obj) {
      return obj && obj.__esModule ? obj : $(function () {
        let result = $Object.create(null, undefined);
        result.default = obj;
        return result;
      })();
    }
    $(_interopRequireDefault);
    module.exports = _interopRequireDefault, module.exports.__esModule = true, module.exports["default"] = module.exports;
  })(interopRequireDefault);
  var interopRequireDefaultExports = interopRequireDefault.exports;
  var regeneratorRuntime$1 = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var _typeof = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequired_typeof;
  function require_typeof() {
    if (hasRequired_typeof) return _typeof.exports;
    hasRequired_typeof = 1;
    $(function (module) {
      function _typeof(o) {
        "@babel/helpers - typeof";

        return module.exports = _typeof = "function" == typeof Symbol && "symbol" == typeof Symbol.iterator ? $(function (o) {
          return typeof o;
        }) : $(function (o) {
          return o && "function" == typeof Symbol && o.constructor === Symbol && o !== Symbol.prototype ? "symbol" : typeof o;
        }), module.exports.__esModule = true, module.exports["default"] = module.exports, _typeof(o);
      }
      $(_typeof);
      module.exports = _typeof, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(_typeof);
    return _typeof.exports;
  }
  $(require_typeof);
  var hasRequiredRegeneratorRuntime;
  function requireRegeneratorRuntime() {
    if (hasRequiredRegeneratorRuntime) return regeneratorRuntime$1.exports;
    hasRequiredRegeneratorRuntime = 1;
    $(function (module) {
      var _typeof = require_typeof()["default"];
      function _regeneratorRuntime() {
        module.exports = _regeneratorRuntime = $(function _regeneratorRuntime() {
          return e;
        }), module.exports.__esModule = true, module.exports["default"] = module.exports;
        var t,
          e = $Object.create(null, undefined),
          r = $Object.prototype,
          n = r.hasOwnProperty,
          o = $Object.defineProperty || $(function (t, e, r) {
            t[e] = r.value;
          }),
          i = "function" == typeof Symbol ? Symbol : $Object.create(null, undefined),
          a = i.iterator || "@@iterator",
          c = i.asyncIterator || "@@asyncIterator",
          u = i.toStringTag || "@@toStringTag";
        function define(t, e, r) {
          return $Object.defineProperty(t, e, $(function () {
            let result = $Object.create(null, undefined);
            result.value = r;
            result.enumerable = !0;
            result.configurable = !0;
            result.writable = !0;
            return result;
          })()), t[e];
        }
        $(define);
        try {
          define($Object.create(null, undefined), "");
        } catch (t) {
          define = $(function define(t, e, r) {
            return t[e] = r;
          });
        }
        function wrap(t, e, r, n) {
          var i = e && e.prototype instanceof Generator ? e : Generator,
            a = $Object.create(i.prototype),
            c = new Context(n || $Array.of());
          return o(a, "_invoke", $(function () {
            let result = $Object.create(null, undefined);
            result.value = makeInvokeMethod(t, r, c);
            return result;
          })()), a;
        }
        $(wrap);
        function tryCatch(t, e, r) {
          try {
            return $(function () {
              let result = $Object.create(null, undefined);
              result.type = "normal";
              result.arg = t.call(e, r);
              return result;
            })();
          } catch (t) {
            return $(function () {
              let result = $Object.create(null, undefined);
              result.type = "throw";
              result.arg = t;
              return result;
            })();
          }
        }
        $(tryCatch);
        e.wrap = wrap;
        var h = "suspendedStart",
          l = "suspendedYield",
          f = "executing",
          s = "completed",
          y = $Object.create(null, undefined);
        function Generator() {}
        $(Generator);
        function GeneratorFunction() {}
        $(GeneratorFunction);
        function GeneratorFunctionPrototype() {}
        $(GeneratorFunctionPrototype);
        var p = $Object.create(null, undefined);
        define(p, a, $(function () {
          return this;
        }));
        var d = $Object.getPrototypeOf,
          v = d && d(d(values($Array.of())));
        v && v !== r && n.call(v, a) && (p = v);
        var g = GeneratorFunctionPrototype.prototype = Generator.prototype = $Object.create(p);
        function defineIteratorMethods(t) {
          $Array.of("next", "throw", "return").forEach($(function (e) {
            define(t, e, $(function (t) {
              return this._invoke(e, t);
            }));
          }));
        }
        $(defineIteratorMethods);
        function AsyncIterator(t, e) {
          function invoke(r, o, i, a) {
            var c = tryCatch(t[r], t, o);
            if ("throw" !== c.type) {
              var u = c.arg,
                h = u.value;
              return h && "object" == _typeof(h) && n.call(h, "__await") ? e.resolve(h.__await).then($(function (t) {
                invoke("next", t, i, a);
              }), $(function (t) {
                invoke("throw", t, i, a);
              })) : e.resolve(h).then($(function (t) {
                u.value = t, i(u);
              }), $(function (t) {
                return invoke("throw", t, i, a);
              }));
            }
            a(c.arg);
          }
          $(invoke);
          var r;
          o(this, "_invoke", $(function () {
            let result = $Object.create(null, undefined);
            result.value = $(function value(t, n) {
              function callInvokeWithMethodAndArg() {
                return new e($(function (e, r) {
                  invoke(t, n, e, r);
                }));
              }
              $(callInvokeWithMethodAndArg);
              return r = r ? r.then(callInvokeWithMethodAndArg, callInvokeWithMethodAndArg) : callInvokeWithMethodAndArg();
            });
            return result;
          })());
        }
        $(AsyncIterator);
        function makeInvokeMethod(e, r, n) {
          var o = h;
          return $(function (i, a) {
            if (o === f) throw new Error("Generator is already running");
            if (o === s) {
              if ("throw" === i) throw a;
              return $(function () {
                let result = $Object.create(null, undefined);
                result.value = t;
                result.done = !0;
                return result;
              })();
            }
            for (n.method = i, n.arg = a;;) {
              var c = n.delegate;
              if (c) {
                var u = maybeInvokeDelegate(c, n);
                if (u) {
                  if (u === y) continue;
                  return u;
                }
              }
              if ("next" === n.method) n.sent = n._sent = n.arg;else if ("throw" === n.method) {
                if (o === h) throw o = s, n.arg;
                n.dispatchException(n.arg);
              } else "return" === n.method && n.abrupt("return", n.arg);
              o = f;
              var p = tryCatch(e, r, n);
              if ("normal" === p.type) {
                if (o = n.done ? s : l, p.arg === y) continue;
                return $(function () {
                  let result = $Object.create(null, undefined);
                  result.value = p.arg;
                  result.done = n.done;
                  return result;
                })();
              }
              "throw" === p.type && (o = s, n.method = "throw", n.arg = p.arg);
            }
          });
        }
        $(makeInvokeMethod);
        function maybeInvokeDelegate(e, r) {
          var n = r.method,
            o = e.iterator[n];
          if (o === t) return r.delegate = null, "throw" === n && e.iterator["return"] && (r.method = "return", r.arg = t, maybeInvokeDelegate(e, r), "throw" === r.method) || "return" !== n && (r.method = "throw", r.arg = new TypeError("The iterator does not provide a '" + n + "' method")), y;
          var i = tryCatch(o, e.iterator, r.arg);
          if ("throw" === i.type) return r.method = "throw", r.arg = i.arg, r.delegate = null, y;
          var a = i.arg;
          return a ? a.done ? (r[e.resultName] = a.value, r.next = e.nextLoc, "return" !== r.method && (r.method = "next", r.arg = t), r.delegate = null, y) : a : (r.method = "throw", r.arg = new TypeError("iterator result is not an object"), r.delegate = null, y);
        }
        $(maybeInvokeDelegate);
        function pushTryEntry(t) {
          var e = $(function () {
            let result = $Object.create(null, undefined);
            result.tryLoc = t[0];
            return result;
          })();
          1 in t && (e.catchLoc = t[1]), 2 in t && (e.finallyLoc = t[2], e.afterLoc = t[3]), this.tryEntries.push(e);
        }
        $(pushTryEntry);
        function resetTryEntry(t) {
          var e = t.completion || $Object.create(null, undefined);
          e.type = "normal", delete e.arg, t.completion = e;
        }
        $(resetTryEntry);
        function Context(t) {
          this.tryEntries = $Array.of($(function () {
            let result = $Object.create(null, undefined);
            result.tryLoc = "root";
            return result;
          })()), t.forEach(pushTryEntry, this), this.reset(!0);
        }
        $(Context);
        function values(e) {
          if (e || "" === e) {
            var r = e[a];
            if (r) return r.call(e);
            if ("function" == typeof e.next) return e;
            if (!isNaN(e.length)) {
              var o = -1,
                i = function next() {
                  for (; ++o < e.length;) if (n.call(e, o)) return next.value = e[o], next.done = !1, next;
                  return next.value = t, next.done = !0, next;
                };
              $(i);
              return i.next = i;
            }
          }
          throw new TypeError(_typeof(e) + " is not iterable");
        }
        $(values);
        return GeneratorFunction.prototype = GeneratorFunctionPrototype, o(g, "constructor", $(function () {
          let result = $Object.create(null, undefined);
          result.value = GeneratorFunctionPrototype;
          result.configurable = !0;
          return result;
        })()), o(GeneratorFunctionPrototype, "constructor", $(function () {
          let result = $Object.create(null, undefined);
          result.value = GeneratorFunction;
          result.configurable = !0;
          return result;
        })()), GeneratorFunction.displayName = define(GeneratorFunctionPrototype, u, "GeneratorFunction"), e.isGeneratorFunction = $(function (t) {
          var e = "function" == typeof t && t.constructor;
          return !!e && (e === GeneratorFunction || "GeneratorFunction" === (e.displayName || e.name));
        }), e.mark = $(function (t) {
          return $Object.setPrototypeOf ? $Object.setPrototypeOf(t, GeneratorFunctionPrototype) : (t.__proto__ = GeneratorFunctionPrototype, define(t, u, "GeneratorFunction")), t.prototype = $Object.create(g), t;
        }), e.awrap = $(function (t) {
          return $(function () {
            let result = $Object.create(null, undefined);
            result.__await = t;
            return result;
          })();
        }), defineIteratorMethods(AsyncIterator.prototype), define(AsyncIterator.prototype, c, $(function () {
          return this;
        })), e.AsyncIterator = AsyncIterator, e.async = $(function (t, r, n, o, i) {
          void 0 === i && (i = Promise);
          var a = new AsyncIterator(wrap(t, r, n, o), i);
          return e.isGeneratorFunction(r) ? a : a.next().then($(function (t) {
            return t.done ? t.value : a.next();
          }));
        }), defineIteratorMethods(g), define(g, u, "Generator"), define(g, a, $(function () {
          return this;
        })), define(g, "toString", $(function () {
          return "[object Generator]";
        })), e.keys = $(function (t) {
          var e = Object(t),
            r = $Array.of();
          for (var n in e) r.push(n);
          return r.reverse(), $(function next() {
            for (; r.length;) {
              var t = r.pop();
              if (t in e) return next.value = t, next.done = !1, next;
            }
            return next.done = !0, next;
          });
        }), e.values = values, Context.prototype = $(function () {
          let result = $Object.create(null, undefined);
          result.constructor = Context;
          result.reset = $(function reset(e) {
            if (this.prev = 0, this.next = 0, this.sent = this._sent = t, this.done = !1, this.delegate = null, this.method = "next", this.arg = t, this.tryEntries.forEach(resetTryEntry), !e) for (var r in this) "t" === r.charAt(0) && n.call(this, r) && !isNaN(+r.slice(1)) && (this[r] = t);
          });
          result.stop = $(function stop() {
            this.done = !0;
            var t = this.tryEntries[0].completion;
            if ("throw" === t.type) throw t.arg;
            return this.rval;
          });
          result.dispatchException = $(function dispatchException(e) {
            if (this.done) throw e;
            var r = this;
            function handle(n, o) {
              return a.type = "throw", a.arg = e, r.next = n, o && (r.method = "next", r.arg = t), !!o;
            }
            $(handle);
            for (var o = this.tryEntries.length - 1; o >= 0; --o) {
              var i = this.tryEntries[o],
                a = i.completion;
              if ("root" === i.tryLoc) return handle("end");
              if (i.tryLoc <= this.prev) {
                var c = n.call(i, "catchLoc"),
                  u = n.call(i, "finallyLoc");
                if (c && u) {
                  if (this.prev < i.catchLoc) return handle(i.catchLoc, !0);
                  if (this.prev < i.finallyLoc) return handle(i.finallyLoc);
                } else if (c) {
                  if (this.prev < i.catchLoc) return handle(i.catchLoc, !0);
                } else {
                  if (!u) throw new Error("try statement without catch or finally");
                  if (this.prev < i.finallyLoc) return handle(i.finallyLoc);
                }
              }
            }
          });
          result.abrupt = $(function abrupt(t, e) {
            for (var r = this.tryEntries.length - 1; r >= 0; --r) {
              var o = this.tryEntries[r];
              if (o.tryLoc <= this.prev && n.call(o, "finallyLoc") && this.prev < o.finallyLoc) {
                var i = o;
                break;
              }
            }
            i && ("break" === t || "continue" === t) && i.tryLoc <= e && e <= i.finallyLoc && (i = null);
            var a = i ? i.completion : $Object.create(null, undefined);
            return a.type = t, a.arg = e, i ? (this.method = "next", this.next = i.finallyLoc, y) : this.complete(a);
          });
          result.complete = $(function complete(t, e) {
            if ("throw" === t.type) throw t.arg;
            return "break" === t.type || "continue" === t.type ? this.next = t.arg : "return" === t.type ? (this.rval = this.arg = t.arg, this.method = "return", this.next = "end") : "normal" === t.type && e && (this.next = e), y;
          });
          result.finish = $(function finish(t) {
            for (var e = this.tryEntries.length - 1; e >= 0; --e) {
              var r = this.tryEntries[e];
              if (r.finallyLoc === t) return this.complete(r.completion, r.afterLoc), resetTryEntry(r), y;
            }
          });
          result.catch = $(function _catch(t) {
            for (var e = this.tryEntries.length - 1; e >= 0; --e) {
              var r = this.tryEntries[e];
              if (r.tryLoc === t) {
                var n = r.completion;
                if ("throw" === n.type) {
                  var o = n.arg;
                  resetTryEntry(r);
                }
                return o;
              }
            }
            throw new Error("illegal catch attempt");
          });
          result.delegateYield = $(function delegateYield(e, r, n) {
            return this.delegate = $(function () {
              let result = $Object.create(null, undefined);
              result.iterator = values(e);
              result.resultName = r;
              result.nextLoc = n;
              return result;
            })(), "next" === this.method && (this.arg = t), y;
          });
          return result;
        })(), e;
      }
      $(_regeneratorRuntime);
      module.exports = _regeneratorRuntime, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(regeneratorRuntime$1);
    return regeneratorRuntime$1.exports;
  }
  $(requireRegeneratorRuntime);
  var regenerator;
  var hasRequiredRegenerator;
  function requireRegenerator() {
    if (hasRequiredRegenerator) return regenerator;
    hasRequiredRegenerator = 1;
    var runtime = requireRegeneratorRuntime()();
    regenerator = runtime;
    try {
      regeneratorRuntime = runtime;
    } catch (accidentalStrictMode) {
      if (typeof globalThis === "object") {
        globalThis.regeneratorRuntime = runtime;
      } else {
        Function("r", "regeneratorRuntime = r")(runtime);
      }
    }
    return regenerator;
  }
  $(requireRegenerator);
  var asyncToGenerator = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredAsyncToGenerator;
  function requireAsyncToGenerator() {
    if (hasRequiredAsyncToGenerator) return asyncToGenerator.exports;
    hasRequiredAsyncToGenerator = 1;
    $(function (module) {
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
      $(asyncGeneratorStep);
      function _asyncToGenerator(fn) {
        return $(function () {
          var self = this,
            args = arguments;
          return new Promise($(function (resolve, reject) {
            var gen = fn.apply(self, args);
            function _next(value) {
              asyncGeneratorStep(gen, resolve, reject, _next, _throw, "next", value);
            }
            $(_next);
            function _throw(err) {
              asyncGeneratorStep(gen, resolve, reject, _next, _throw, "throw", err);
            }
            $(_throw);
            _next(undefined);
          }));
        });
      }
      $(_asyncToGenerator);
      module.exports = _asyncToGenerator, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(asyncToGenerator);
    return asyncToGenerator.exports;
  }
  $(requireAsyncToGenerator);
  var classCallCheck = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredClassCallCheck;
  function requireClassCallCheck() {
    if (hasRequiredClassCallCheck) return classCallCheck.exports;
    hasRequiredClassCallCheck = 1;
    $(function (module) {
      function _classCallCheck(instance, Constructor) {
        if (!(instance instanceof Constructor)) {
          throw new TypeError("Cannot call a class as a function");
        }
      }
      $(_classCallCheck);
      module.exports = _classCallCheck, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(classCallCheck);
    return classCallCheck.exports;
  }
  $(requireClassCallCheck);
  var createClass = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var toPropertyKey = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var toPrimitive = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredToPrimitive;
  function requireToPrimitive() {
    if (hasRequiredToPrimitive) return toPrimitive.exports;
    hasRequiredToPrimitive = 1;
    $(function (module) {
      var _typeof = require_typeof()["default"];
      function _toPrimitive(input, hint) {
        if (_typeof(input) !== "object" || input === null) return input;
        var prim = input[Symbol.toPrimitive];
        if (prim !== undefined) {
          var res = prim.call(input, hint || "default");
          if (_typeof(res) !== "object") return res;
          throw new TypeError("@@toPrimitive must return a primitive value.");
        }
        return (hint === "string" ? String : Number)(input);
      }
      $(_toPrimitive);
      module.exports = _toPrimitive, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(toPrimitive);
    return toPrimitive.exports;
  }
  $(requireToPrimitive);
  var hasRequiredToPropertyKey;
  function requireToPropertyKey() {
    if (hasRequiredToPropertyKey) return toPropertyKey.exports;
    hasRequiredToPropertyKey = 1;
    $(function (module) {
      var _typeof = require_typeof()["default"];
      var toPrimitive = requireToPrimitive();
      function _toPropertyKey(arg) {
        var key = toPrimitive(arg, "string");
        return _typeof(key) === "symbol" ? key : String(key);
      }
      $(_toPropertyKey);
      module.exports = _toPropertyKey, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(toPropertyKey);
    return toPropertyKey.exports;
  }
  $(requireToPropertyKey);
  var hasRequiredCreateClass;
  function requireCreateClass() {
    if (hasRequiredCreateClass) return createClass.exports;
    hasRequiredCreateClass = 1;
    $(function (module) {
      var toPropertyKey = requireToPropertyKey();
      function _defineProperties(target, props) {
        for (var i = 0; i < props.length; i++) {
          var descriptor = props[i];
          descriptor.enumerable = descriptor.enumerable || false;
          descriptor.configurable = true;
          if ("value" in descriptor) descriptor.writable = true;
          $Object.defineProperty(target, toPropertyKey(descriptor.key), descriptor);
        }
      }
      $(_defineProperties);
      function _createClass(Constructor, protoProps, staticProps) {
        if (protoProps) _defineProperties(Constructor.prototype, protoProps);
        if (staticProps) _defineProperties(Constructor, staticProps);
        $Object.defineProperty(Constructor, "prototype", $(function () {
          let result = $Object.create(null, undefined);
          result.writable = false;
          return result;
        })());
        return Constructor;
      }
      $(_createClass);
      module.exports = _createClass, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(createClass);
    return createClass.exports;
  }
  $(requireCreateClass);
  var inherits = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var setPrototypeOf = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredSetPrototypeOf;
  function requireSetPrototypeOf() {
    if (hasRequiredSetPrototypeOf) return setPrototypeOf.exports;
    hasRequiredSetPrototypeOf = 1;
    $(function (module) {
      function _setPrototypeOf(o, p) {
        module.exports = _setPrototypeOf = $Object.setPrototypeOf ? $Object.setPrototypeOf.bind() : $(function _setPrototypeOf(o, p) {
          o.__proto__ = p;
          return o;
        }), module.exports.__esModule = true, module.exports["default"] = module.exports;
        return _setPrototypeOf(o, p);
      }
      $(_setPrototypeOf);
      module.exports = _setPrototypeOf, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(setPrototypeOf);
    return setPrototypeOf.exports;
  }
  $(requireSetPrototypeOf);
  var hasRequiredInherits;
  function requireInherits() {
    if (hasRequiredInherits) return inherits.exports;
    hasRequiredInherits = 1;
    $(function (module) {
      var setPrototypeOf = requireSetPrototypeOf();
      function _inherits(subClass, superClass) {
        if (typeof superClass !== "function" && superClass !== null) {
          throw new TypeError("Super expression must either be null or a function");
        }
        subClass.prototype = $Object.create(superClass && superClass.prototype, $(function () {
          let result = $Object.create(null, undefined);
          result.constructor = $(function () {
            let result = $Object.create(null, undefined);
            result.value = subClass;
            result.writable = true;
            result.configurable = true;
            return result;
          })();
          return result;
        })());
        $Object.defineProperty(subClass, "prototype", $(function () {
          let result = $Object.create(null, undefined);
          result.writable = false;
          return result;
        })());
        if (superClass) setPrototypeOf(subClass, superClass);
      }
      $(_inherits);
      module.exports = _inherits, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(inherits);
    return inherits.exports;
  }
  $(requireInherits);
  var possibleConstructorReturn = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var assertThisInitialized = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredAssertThisInitialized;
  function requireAssertThisInitialized() {
    if (hasRequiredAssertThisInitialized) return assertThisInitialized.exports;
    hasRequiredAssertThisInitialized = 1;
    $(function (module) {
      function _assertThisInitialized(self) {
        if (self === void 0) {
          throw new ReferenceError("this hasn't been initialised - super() hasn't been called");
        }
        return self;
      }
      $(_assertThisInitialized);
      module.exports = _assertThisInitialized, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(assertThisInitialized);
    return assertThisInitialized.exports;
  }
  $(requireAssertThisInitialized);
  var hasRequiredPossibleConstructorReturn;
  function requirePossibleConstructorReturn() {
    if (hasRequiredPossibleConstructorReturn) return possibleConstructorReturn.exports;
    hasRequiredPossibleConstructorReturn = 1;
    $(function (module) {
      var _typeof = require_typeof()["default"];
      var assertThisInitialized = requireAssertThisInitialized();
      function _possibleConstructorReturn(self, call) {
        if (call && (_typeof(call) === "object" || typeof call === "function")) {
          return call;
        } else if (call !== void 0) {
          throw new TypeError("Derived constructors may only return object or undefined");
        }
        return assertThisInitialized(self);
      }
      $(_possibleConstructorReturn);
      module.exports = _possibleConstructorReturn, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(possibleConstructorReturn);
    return possibleConstructorReturn.exports;
  }
  $(requirePossibleConstructorReturn);
  var getPrototypeOf = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredGetPrototypeOf;
  function requireGetPrototypeOf() {
    if (hasRequiredGetPrototypeOf) return getPrototypeOf.exports;
    hasRequiredGetPrototypeOf = 1;
    $(function (module) {
      function _getPrototypeOf(o) {
        module.exports = _getPrototypeOf = $Object.setPrototypeOf ? $Object.getPrototypeOf.bind() : $(function _getPrototypeOf(o) {
          return o.__proto__ || $Object.getPrototypeOf(o);
        }), module.exports.__esModule = true, module.exports["default"] = module.exports;
        return _getPrototypeOf(o);
      }
      $(_getPrototypeOf);
      module.exports = _getPrototypeOf, module.exports.__esModule = true, module.exports["default"] = module.exports;
    })(getPrototypeOf);
    return getPrototypeOf.exports;
  }
  $(requireGetPrototypeOf);
  var eventemitter3 = $(function () {
    let result = $Object.create(null, undefined);
    result.exports = $Object.create(null, undefined);
    return result;
  })();
  var hasRequiredEventemitter3;
  function requireEventemitter3() {
    if (hasRequiredEventemitter3) return eventemitter3.exports;
    hasRequiredEventemitter3 = 1;
    $(function (module) {
      var has = $Object.prototype.hasOwnProperty,
        prefix = "~";
      function Events() {}
      $(Events);
      if ($Object.create) {
        Events.prototype = $Object.create(null);
        if (!new Events().__proto__) prefix = false;
      }
      function EE(fn, context, once) {
        this.fn = fn;
        this.context = context;
        this.once = once || false;
      }
      $(EE);
      function addListener(emitter, event, fn, context, once) {
        if (typeof fn !== "function") {
          throw new TypeError("The listener must be a function");
        }
        var listener = new EE(fn, context || emitter, once),
          evt = prefix ? prefix + event : event;
        if (!emitter._events[evt]) emitter._events[evt] = listener, emitter._eventsCount++;else if (!emitter._events[evt].fn) emitter._events[evt].push(listener);else emitter._events[evt] = $Array.of(emitter._events[evt], listener);
        return emitter;
      }
      $(addListener);
      function clearEvent(emitter, evt) {
        if (--emitter._eventsCount === 0) emitter._events = new Events();else delete emitter._events[evt];
      }
      $(clearEvent);
      function EventEmitter() {
        this._events = new Events();
        this._eventsCount = 0;
      }
      $(EventEmitter);
      EventEmitter.prototype.eventNames = $(function eventNames() {
        var names = $Array.of(),
          events,
          name;
        if (this._eventsCount === 0) return names;
        for (name in events = this._events) {
          if (has.call(events, name)) names.push(prefix ? name.slice(1) : name);
        }
        if ($Object.getOwnPropertySymbols) {
          return names.concat($Object.getOwnPropertySymbols(events));
        }
        return names;
      });
      EventEmitter.prototype.listeners = $(function listeners(event) {
        var evt = prefix ? prefix + event : event,
          handlers = this._events[evt];
        if (!handlers) return $Array.of();
        if (handlers.fn) return $Array.of(handlers.fn);
        for (var i = 0, l = handlers.length, ee = new Array(l); i < l; i++) {
          ee[i] = handlers[i].fn;
        }
        return ee;
      });
      EventEmitter.prototype.listenerCount = $(function listenerCount(event) {
        var evt = prefix ? prefix + event : event,
          listeners = this._events[evt];
        if (!listeners) return 0;
        if (listeners.fn) return 1;
        return listeners.length;
      });
      EventEmitter.prototype.emit = $(function emit(event, a1, a2, a3, a4, a5) {
        var evt = prefix ? prefix + event : event;
        if (!this._events[evt]) return false;
        var listeners = this._events[evt],
          len = arguments.length,
          args,
          i;
        if (listeners.fn) {
          if (listeners.once) this.removeListener(event, listeners.fn, undefined, true);
          switch (len) {
            case 1:
              return listeners.fn.call(listeners.context), true;
            case 2:
              return listeners.fn.call(listeners.context, a1), true;
            case 3:
              return listeners.fn.call(listeners.context, a1, a2), true;
            case 4:
              return listeners.fn.call(listeners.context, a1, a2, a3), true;
            case 5:
              return listeners.fn.call(listeners.context, a1, a2, a3, a4), true;
            case 6:
              return listeners.fn.call(listeners.context, a1, a2, a3, a4, a5), true;
          }
          for (i = 1, args = new Array(len - 1); i < len; i++) {
            args[i - 1] = arguments[i];
          }
          listeners.fn.apply(listeners.context, args);
        } else {
          var length = listeners.length,
            j;
          for (i = 0; i < length; i++) {
            if (listeners[i].once) this.removeListener(event, listeners[i].fn, undefined, true);
            switch (len) {
              case 1:
                listeners[i].fn.call(listeners[i].context);
                break;
              case 2:
                listeners[i].fn.call(listeners[i].context, a1);
                break;
              case 3:
                listeners[i].fn.call(listeners[i].context, a1, a2);
                break;
              case 4:
                listeners[i].fn.call(listeners[i].context, a1, a2, a3);
                break;
              default:
                if (!args) for (j = 1, args = new Array(len - 1); j < len; j++) {
                  args[j - 1] = arguments[j];
                }
                listeners[i].fn.apply(listeners[i].context, args);
            }
          }
        }
        return true;
      });
      EventEmitter.prototype.on = $(function on(event, fn, context) {
        return addListener(this, event, fn, context, false);
      });
      EventEmitter.prototype.once = $(function once(event, fn, context) {
        return addListener(this, event, fn, context, true);
      });
      EventEmitter.prototype.removeListener = $(function removeListener(event, fn, context, once) {
        var evt = prefix ? prefix + event : event;
        if (!this._events[evt]) return this;
        if (!fn) {
          clearEvent(this, evt);
          return this;
        }
        var listeners = this._events[evt];
        if (listeners.fn) {
          if (listeners.fn === fn && (!once || listeners.once) && (!context || listeners.context === context)) {
            clearEvent(this, evt);
          }
        } else {
          for (var i = 0, events = $Array.of(), length = listeners.length; i < length; i++) {
            if (listeners[i].fn !== fn || once && !listeners[i].once || context && listeners[i].context !== context) {
              events.push(listeners[i]);
            }
          }
          if (events.length) this._events[evt] = events.length === 1 ? events[0] : events;else clearEvent(this, evt);
        }
        return this;
      });
      EventEmitter.prototype.removeAllListeners = $(function removeAllListeners(event) {
        var evt;
        if (event) {
          evt = prefix ? prefix + event : event;
          if (this._events[evt]) clearEvent(this, evt);
        } else {
          this._events = new Events();
          this._eventsCount = 0;
        }
        return this;
      });
      EventEmitter.prototype.off = EventEmitter.prototype.removeListener;
      EventEmitter.prototype.addListener = EventEmitter.prototype.on;
      EventEmitter.prefixed = prefix;
      EventEmitter.EventEmitter = EventEmitter;
      {
        module.exports = EventEmitter;
      }
    })(eventemitter3);
    return eventemitter3.exports;
  }
  $(requireEventemitter3);
  $(function (exports) {
    var _interopRequireDefault = interopRequireDefaultExports;
    $Object.defineProperty(exports, "__esModule", $(function () {
      let result = $Object.create(null, undefined);
      result.value = true;
      return result;
    })());
    exports["default"] = void 0;
    var _regenerator = _interopRequireDefault(requireRegenerator());
    var _asyncToGenerator2 = _interopRequireDefault(requireAsyncToGenerator());
    var _typeof2 = _interopRequireDefault(require_typeof());
    var _classCallCheck2 = _interopRequireDefault(requireClassCallCheck());
    var _createClass2 = _interopRequireDefault(requireCreateClass());
    var _inherits2 = _interopRequireDefault(requireInherits());
    var _possibleConstructorReturn2 = _interopRequireDefault(requirePossibleConstructorReturn());
    var _getPrototypeOf2 = _interopRequireDefault(requireGetPrototypeOf());
    var _eventemitter = requireEventemitter3();
    function _createSuper(Derived) {
      var hasNativeReflectConstruct = _isNativeReflectConstruct();
      return $(function _createSuperInternal() {
        var Super = (0, _getPrototypeOf2["default"])(Derived),
          result;
        if (hasNativeReflectConstruct) {
          var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor;
          result = Reflect.construct(Super, arguments, NewTarget);
        } else {
          result = Super.apply(this, arguments);
        }
        return (0, _possibleConstructorReturn2["default"])(this, result);
      });
    }
    $(_createSuper);
    function _isNativeReflectConstruct() {
      if (typeof Reflect === "undefined" || !Reflect.construct) return false;
      if (Reflect.construct.sham) return false;
      if (typeof Proxy === "function") return true;
      try {
        Boolean.prototype.valueOf.call(Reflect.construct(Boolean, $Array.of(), $(function () {})));
        return true;
      } catch (e) {
        return false;
      }
    }
    $(_isNativeReflectConstruct);
    var __rest = function (s, e) {
      var t = $Object.create(null, undefined);
      for (var p in s) {
        if ($Object.prototype.hasOwnProperty.call(s, p) && e.indexOf(p) < 0) t[p] = s[p];
      }
      if (s != null && typeof $Object.getOwnPropertySymbols === "function") for (var i = 0, p = $Object.getOwnPropertySymbols(s); i < p.length; i++) {
        if (e.indexOf(p[i]) < 0 && $Object.prototype.propertyIsEnumerable.call(s, p[i])) t[p[i]] = s[p[i]];
      }
      return t;
    };
    $(__rest);
    var CommonClient = $(function (_EventEmitter) {
      (0, _inherits2["default"])(CommonClient, _EventEmitter);
      var _super = _createSuper(CommonClient);
      function CommonClient(webSocketFactory) {
        var _this;
        var address = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : "ws://localhost:8080";
        var _a = arguments.length > 2 && arguments[2] !== undefined ? arguments[2] : $Object.create(null, undefined);
        var generate_request_id = arguments.length > 3 ? arguments[3] : undefined;
        (0, _classCallCheck2["default"])(this, CommonClient);
        var _a$autoconnect = _a.autoconnect,
          autoconnect = _a$autoconnect === void 0 ? true : _a$autoconnect,
          _a$reconnect = _a.reconnect,
          reconnect = _a$reconnect === void 0 ? true : _a$reconnect,
          _a$reconnect_interval = _a.reconnect_interval,
          reconnect_interval = _a$reconnect_interval === void 0 ? 1e3 : _a$reconnect_interval,
          _a$max_reconnects = _a.max_reconnects,
          max_reconnects = _a$max_reconnects === void 0 ? 5 : _a$max_reconnects,
          rest_options = __rest(_a, $Array.of("autoconnect", "reconnect", "reconnect_interval", "max_reconnects"));
        _this = _super.call(this);
        _this.webSocketFactory = webSocketFactory;
        _this.queue = $Object.create(null, undefined);
        _this.rpc_id = 0;
        _this.address = address;
        _this.autoconnect = autoconnect;
        _this.ready = false;
        _this.reconnect = reconnect;
        _this.reconnect_timer_id = undefined;
        _this.reconnect_interval = reconnect_interval;
        _this.max_reconnects = max_reconnects;
        _this.rest_options = rest_options;
        _this.current_reconnects = 0;
        _this.generate_request_id = generate_request_id || $(function () {
          return ++_this.rpc_id;
        });
        if (_this.autoconnect) _this._connect(_this.address, $Object.assign($(function () {
          let result = $Object.create(null, undefined);
          result.autoconnect = _this.autoconnect;
          result.reconnect = _this.reconnect;
          result.reconnect_interval = _this.reconnect_interval;
          result.max_reconnects = _this.max_reconnects;
          return result;
        })(), _this.rest_options));
        return _this;
      }
      $(CommonClient);
      (0, _createClass2["default"])(CommonClient, $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.key = "connect";
        result.value = $(function connect() {
          if (this.socket) return;
          this._connect(this.address, $Object.assign($(function () {
            let result = $Object.create(null, undefined);
            result.autoconnect = this.autoconnect;
            result.reconnect = this.reconnect;
            result.reconnect_interval = this.reconnect_interval;
            result.max_reconnects = this.max_reconnects;
            return result;
          })(), this.rest_options));
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "call";
        result.value = $(function call(method, params, timeout, ws_opts) {
          var _this2 = this;
          if (!ws_opts && "object" === (0, _typeof2["default"])(timeout)) {
            ws_opts = timeout;
            timeout = null;
          }
          return new Promise($(function (resolve, reject) {
            if (!_this2.ready) return reject(new Error("socket not ready"));
            var rpc_id = _this2.generate_request_id(method, params);
            var message = $(function () {
              let result = $Object.create(null, undefined);
              result.jsonrpc = "2.0";
              result.method = method;
              result.params = params || null;
              result.id = rpc_id;
              return result;
            })();
            _this2.socket.send(JSON.stringify(message), ws_opts, $(function (error) {
              if (error) return reject(error);
              _this2.queue[rpc_id] = $(function () {
                let result = $Object.create(null, undefined);
                result.promise = $Array.of(resolve, reject);
                return result;
              })();
              if (timeout) {
                _this2.queue[rpc_id].timeout = setTimeout($(function () {
                  delete _this2.queue[rpc_id];
                  reject(new Error("reply timeout"));
                }), timeout);
              }
            }));
          }));
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "login";
        result.value = $(function () {
          var _login = (0, _asyncToGenerator2["default"])(_regenerator["default"].mark($(function _callee(params) {
            var resp;
            return _regenerator["default"].wrap($(function _callee$(_context) {
              while (1) {
                switch (_context.prev = _context.next) {
                  case 0:
                    _context.next = 2;
                    return this.call("rpc.login", params);
                  case 2:
                    resp = _context.sent;
                    if (resp) {
                      _context.next = 5;
                      break;
                    }
                    throw new Error("authentication failed");
                  case 5:
                    return _context.abrupt("return", resp);
                  case 6:
                  case "end":
                    return _context.stop();
                }
              }
            }), _callee, this);
          })));
          function login(_x) {
            return _login.apply(this, arguments);
          }
          $(login);
          return login;
        })();
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "listMethods";
        result.value = $(function () {
          var _listMethods = (0, _asyncToGenerator2["default"])(_regenerator["default"].mark($(function _callee2() {
            return _regenerator["default"].wrap($(function _callee2$(_context2) {
              while (1) {
                switch (_context2.prev = _context2.next) {
                  case 0:
                    _context2.next = 2;
                    return this.call("__listMethods");
                  case 2:
                    return _context2.abrupt("return", _context2.sent);
                  case 3:
                  case "end":
                    return _context2.stop();
                }
              }
            }), _callee2, this);
          })));
          function listMethods() {
            return _listMethods.apply(this, arguments);
          }
          $(listMethods);
          return listMethods;
        })();
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "notify";
        result.value = $(function notify(method, params) {
          var _this3 = this;
          return new Promise($(function (resolve, reject) {
            if (!_this3.ready) return reject(new Error("socket not ready"));
            var message = $(function () {
              let result = $Object.create(null, undefined);
              result.jsonrpc = "2.0";
              result.method = method;
              result.params = params || null;
              return result;
            })();
            _this3.socket.send(JSON.stringify(message), $(function (error) {
              if (error) return reject(error);
              resolve();
            }));
          }));
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "subscribe";
        result.value = $(function () {
          var _subscribe = (0, _asyncToGenerator2["default"])(_regenerator["default"].mark($(function _callee3(event) {
            var result;
            return _regenerator["default"].wrap($(function _callee3$(_context3) {
              while (1) {
                switch (_context3.prev = _context3.next) {
                  case 0:
                    if (typeof event === "string") event = $Array.of(event);
                    _context3.next = 3;
                    return this.call("rpc.on", event);
                  case 3:
                    result = _context3.sent;
                    if (!(typeof event === "string" && result[event] !== "ok")) {
                      _context3.next = 6;
                      break;
                    }
                    throw new Error("Failed subscribing to an event '" + event + "' with: " + result[event]);
                  case 6:
                    return _context3.abrupt("return", result);
                  case 7:
                  case "end":
                    return _context3.stop();
                }
              }
            }), _callee3, this);
          })));
          function subscribe(_x2) {
            return _subscribe.apply(this, arguments);
          }
          $(subscribe);
          return subscribe;
        })();
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "unsubscribe";
        result.value = $(function () {
          var _unsubscribe = (0, _asyncToGenerator2["default"])(_regenerator["default"].mark($(function _callee4(event) {
            var result;
            return _regenerator["default"].wrap($(function _callee4$(_context4) {
              while (1) {
                switch (_context4.prev = _context4.next) {
                  case 0:
                    if (typeof event === "string") event = $Array.of(event);
                    _context4.next = 3;
                    return this.call("rpc.off", event);
                  case 3:
                    result = _context4.sent;
                    if (!(typeof event === "string" && result[event] !== "ok")) {
                      _context4.next = 6;
                      break;
                    }
                    throw new Error("Failed unsubscribing from an event with: " + result);
                  case 6:
                    return _context4.abrupt("return", result);
                  case 7:
                  case "end":
                    return _context4.stop();
                }
              }
            }), _callee4, this);
          })));
          function unsubscribe(_x3) {
            return _unsubscribe.apply(this, arguments);
          }
          $(unsubscribe);
          return unsubscribe;
        })();
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "close";
        result.value = $(function close(code, data) {
          this.socket.close(code || 1e3, data);
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "_connect";
        result.value = $(function _connect(address, options) {
          var _this4 = this;
          clearTimeout(this.reconnect_timer_id);
          this.socket = this.webSocketFactory(address, options);
          this.socket.addEventListener("open", $(function () {
            _this4.ready = true;
            _this4.emit("open");
            _this4.current_reconnects = 0;
          }));
          this.socket.addEventListener("message", $(function (_ref) {
            var message = _ref.data;
            if (message instanceof ArrayBuffer) message = Buffer.from(message).toString();
            try {
              message = JSON.parse(message);
            } catch (error) {
              return;
            }
            if (message.notification && _this4.listeners(message.notification).length) {
              if (!$Object.keys(message.params).length) return _this4.emit(message.notification);
              var args = $Array.of(message.notification);
              if (message.params.constructor === Object) args.push(message.params);else for (var i = 0; i < message.params.length; i++) {
                args.push(message.params[i]);
              }
              return Promise.resolve().then($(function () {
                _this4.emit.apply(_this4, args);
              }));
            }
            if (!_this4.queue[message.id]) {
              if (message.method && message.params) {
                return Promise.resolve().then($(function () {
                  _this4.emit(message.method, message.params);
                }));
              }
              return;
            }
            if ("error" in message === "result" in message) _this4.queue[message.id].promise[1](new Error('Server response malformed. Response must include either "result"' + ' or "error", but not both.'));
            if (_this4.queue[message.id].timeout) clearTimeout(_this4.queue[message.id].timeout);
            if (message.error) _this4.queue[message.id].promise[1](message.error);else _this4.queue[message.id].promise[0](message.result);
            delete _this4.queue[message.id];
          }));
          this.socket.addEventListener("error", $(function (error) {
            return _this4.emit("error", error);
          }));
          this.socket.addEventListener("close", $(function (_ref2) {
            var code = _ref2.code,
              reason = _ref2.reason;
            if (_this4.ready) setTimeout($(function () {
              return _this4.emit("close", code, reason);
            }), 0);
            _this4.ready = false;
            _this4.socket = undefined;
            if (code === 1e3) return;
            _this4.current_reconnects++;
            if (_this4.reconnect && (_this4.max_reconnects > _this4.current_reconnects || _this4.max_reconnects === 0)) _this4.reconnect_timer_id = setTimeout($(function () {
              return _this4._connect(address, options);
            }), _this4.reconnect_interval);
          }));
        });
        return result;
      })()));
      return CommonClient;
    })(_eventemitter.EventEmitter);
    exports["default"] = CommonClient;
  })(client);
  var RpcWebSocketCommonClient = getDefaultExportFromCjs(client);
  var websocket_browser = $Object.create(null, undefined);
  $(function (exports) {
    var _interopRequireDefault = interopRequireDefaultExports;
    $Object.defineProperty(exports, "__esModule", $(function () {
      let result = $Object.create(null, undefined);
      result.value = true;
      return result;
    })());
    exports["default"] = _default;
    var _classCallCheck2 = _interopRequireDefault(requireClassCallCheck());
    var _createClass2 = _interopRequireDefault(requireCreateClass());
    var _inherits2 = _interopRequireDefault(requireInherits());
    var _possibleConstructorReturn2 = _interopRequireDefault(requirePossibleConstructorReturn());
    var _getPrototypeOf2 = _interopRequireDefault(requireGetPrototypeOf());
    var _eventemitter = requireEventemitter3();
    function _createSuper(Derived) {
      var hasNativeReflectConstruct = _isNativeReflectConstruct();
      return $(function _createSuperInternal() {
        var Super = (0, _getPrototypeOf2["default"])(Derived),
          result;
        if (hasNativeReflectConstruct) {
          var NewTarget = (0, _getPrototypeOf2["default"])(this).constructor;
          result = Reflect.construct(Super, arguments, NewTarget);
        } else {
          result = Super.apply(this, arguments);
        }
        return (0, _possibleConstructorReturn2["default"])(this, result);
      });
    }
    $(_createSuper);
    function _isNativeReflectConstruct() {
      if (typeof Reflect === "undefined" || !Reflect.construct) return false;
      if (Reflect.construct.sham) return false;
      if (typeof Proxy === "function") return true;
      try {
        Boolean.prototype.valueOf.call(Reflect.construct(Boolean, $Array.of(), $(function () {})));
        return true;
      } catch (e) {
        return false;
      }
    }
    $(_isNativeReflectConstruct);
    var WebSocketBrowserImpl = $(function (_EventEmitter) {
      (0, _inherits2["default"])(WebSocketBrowserImpl, _EventEmitter);
      var _super = _createSuper(WebSocketBrowserImpl);
      function WebSocketBrowserImpl(address, options, protocols) {
        var _this;
        (0, _classCallCheck2["default"])(this, WebSocketBrowserImpl);
        _this = _super.call(this);
        _this.socket = new window.WebSocket(address, protocols);
        _this.socket.onopen = $(function () {
          return _this.emit("open");
        });
        _this.socket.onmessage = $(function (event) {
          return _this.emit("message", event.data);
        });
        _this.socket.onerror = $(function (error) {
          return _this.emit("error", error);
        });
        _this.socket.onclose = $(function (event) {
          _this.emit("close", event.code, event.reason);
        });
        return _this;
      }
      $(WebSocketBrowserImpl);
      (0, _createClass2["default"])(WebSocketBrowserImpl, $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.key = "send";
        result.value = $(function send(data, optionsOrCallback, callback) {
          var cb = callback || optionsOrCallback;
          try {
            this.socket.send(data);
            cb();
          } catch (error) {
            cb(error);
          }
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "close";
        result.value = $(function close(code, reason) {
          this.socket.close(code, reason);
        });
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.key = "addEventListener";
        result.value = $(function addEventListener(type, listener, options) {
          this.socket.addEventListener(type, listener, options);
        });
        return result;
      })()));
      return WebSocketBrowserImpl;
    })(_eventemitter.EventEmitter);
    function _default(address, options) {
      return new WebSocketBrowserImpl(address, options);
    }
    $(_default);
  })(websocket_browser);
  var createRpc = getDefaultExportFromCjs(websocket_browser);
  class RpcWebSocketClient extends RpcWebSocketCommonClient {
    constructor(address, options, generate_request_id) {
      const webSocketFactory = url => {
        const rpc = createRpc(url, $(function () {
          let result = $Object.create(null, undefined);
          result.autoconnect = true;
          result.max_reconnects = 5;
          result.reconnect = true;
          result.reconnect_interval = 1e3;
          $Object.assign(result, options);
          return result;
        })());
        if ("socket" in rpc) {
          this.underlyingSocket = rpc.socket;
        } else {
          this.underlyingSocket = rpc;
        }
        return rpc;
      };
      $(webSocketFactory);
      super(webSocketFactory, address, options, generate_request_id);
      this.underlyingSocket = void 0;
    }
    call(...args) {
      const readyState = this.underlyingSocket?.readyState;
      if (readyState === 1) {
        return super.call(...args);
      }
      return Promise.reject(new Error("Tried to call a JSON-RPC method `" + args[0] + "` but the socket was not `CONNECTING` or `OPEN` (`readyState` was " + readyState + ")"));
    }
    notify(...args) {
      const readyState = this.underlyingSocket?.readyState;
      if (readyState === 1) {
        return super.notify(...args);
      }
      return Promise.reject(new Error("Tried to send a JSON-RPC notification `" + args[0] + "` but the socket was not `CONNECTING` or `OPEN` (`readyState` was " + readyState + ")"));
    }
  }
  function decodeData(type, data) {
    let decoded;
    try {
      decoded = type.layout.decode(data);
    } catch (err) {
      throw new Error("invalid instruction; " + err);
    }
    if (decoded.typeIndex !== type.index) {
      throw new Error(`invalid account data; account type mismatch ${decoded.typeIndex} != ${type.index}`);
    }
    return decoded;
  }
  $(decodeData);
  const LOOKUP_TABLE_META_SIZE = 56;
  class AddressLookupTableAccount {
    constructor(args) {
      this.key = void 0;
      this.state = void 0;
      this.key = args.key;
      this.state = args.state;
    }
    isActive() {
      const U64_MAX = BigInt("0xffffffffffffffff");
      return this.state.deactivationSlot === U64_MAX;
    }
    static deserialize(accountData) {
      const meta = decodeData(LookupTableMetaLayout, accountData);
      const serializedAddressesLen = accountData.length - LOOKUP_TABLE_META_SIZE;
      assert$1(serializedAddressesLen >= 0, "lookup table is invalid");
      assert$1(serializedAddressesLen % 32 === 0, "lookup table is invalid");
      const numSerializedAddresses = serializedAddressesLen / 32;
      const {
        addresses: addresses
      } = struct($Array.of(seq(publicKey(), numSerializedAddresses, "addresses"))).decode(accountData.slice(LOOKUP_TABLE_META_SIZE));
      return $(function () {
        let result = $Object.create(null, undefined);
        result.deactivationSlot = meta.deactivationSlot;
        result.lastExtendedSlot = meta.lastExtendedSlot;
        result.lastExtendedSlotStartIndex = meta.lastExtendedStartIndex;
        result.authority = meta.authority.length !== 0 ? new PublicKey(meta.authority[0]) : undefined;
        result.addresses = addresses.map($(address => new PublicKey(address)));
        return result;
      })();
    }
  }
  const LookupTableMetaLayout = $(function () {
    let result = $Object.create(null, undefined);
    result.index = 1;
    result.layout = struct($Array.of(u32("typeIndex"), u64("deactivationSlot"), nu64("lastExtendedSlot"), u8("lastExtendedStartIndex"), u8(), seq(publicKey(), offset(u8(), -1), "authority")));
    return result;
  })();
  const URL_RE = /^[^:]+:\/\/([^:[]+|\[[^\]]+\])(:\d+)?(.*)/i;
  function makeWebsocketUrl(endpoint) {
    const matches = endpoint.match(URL_RE);
    if (matches == null) {
      throw TypeError(`Failed to validate endpoint URL \`${endpoint}\``);
    }
    const [_, hostish, portWithColon, rest] = matches;
    const protocol = endpoint.startsWith("https:") ? "wss:" : "ws:";
    const startPort = portWithColon == null ? null : parseInt(portWithColon.slice(1), 10);
    const websocketPort = startPort == null ? "" : `:${startPort + 1}`;
    return `${protocol}//${hostish}${websocketPort}${rest}`;
  }
  $(makeWebsocketUrl);
  const PublicKeyFromString = coerce(instance(PublicKey), string(), $(value => new PublicKey(value)));
  const RawAccountDataResult = tuple($Array.of(string(), literal("base64")));
  const BufferFromRawAccountData = coerce(instance(buffer.Buffer), RawAccountDataResult, $(value => buffer.Buffer.from(value[0], "base64")));
  const BLOCKHASH_CACHE_TIMEOUT_MS = 30 * 1e3;
  function assertEndpointUrl(putativeUrl) {
    if (/^https?:/.test(putativeUrl) === false) {
      throw new TypeError("Endpoint URL must start with `http:` or `https:`.");
    }
    return putativeUrl;
  }
  $(assertEndpointUrl);
  function extractCommitmentFromConfig(commitmentOrConfig) {
    let commitment;
    let config;
    if (typeof commitmentOrConfig === "string") {
      commitment = commitmentOrConfig;
    } else if (commitmentOrConfig) {
      const {
        commitment: specifiedCommitment,
        ...specifiedConfig
      } = commitmentOrConfig;
      commitment = specifiedCommitment;
      config = specifiedConfig;
    }
    return $(function () {
      let result = $Object.create(null, undefined);
      result.commitment = commitment;
      result.config = config;
      return result;
    })();
  }
  $(extractCommitmentFromConfig);
  function createRpcResult(result) {
    return union($Array.of(type($(function () {
      let result = $Object.create(null, undefined);
      result.jsonrpc = literal("2.0");
      result.id = string();
      result.result = result;
      return result;
    })()), type($(function () {
      let result = $Object.create(null, undefined);
      result.jsonrpc = literal("2.0");
      result.id = string();
      result.error = type($(function () {
        let result = $Object.create(null, undefined);
        result.code = unknown();
        result.message = string();
        result.data = optional(any());
        return result;
      })());
      return result;
    })())));
  }
  $(createRpcResult);
  const UnknownRpcResult = createRpcResult(unknown());
  function jsonRpcResult(schema) {
    return coerce(createRpcResult(schema), UnknownRpcResult, $(value => {
      if ("error" in value) {
        return value;
      } else {
        return $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, value);
          result.result = create(value.result, schema);
          return result;
        })();
      }
    }));
  }
  $(jsonRpcResult);
  function jsonRpcResultAndContext(value) {
    return jsonRpcResult(type($(function () {
      let result = $Object.create(null, undefined);
      result.context = type($(function () {
        let result = $Object.create(null, undefined);
        result.slot = number();
        return result;
      })());
      result.value = value;
      return result;
    })()));
  }
  $(jsonRpcResultAndContext);
  function notificationResultAndContext(value) {
    return type($(function () {
      let result = $Object.create(null, undefined);
      result.context = type($(function () {
        let result = $Object.create(null, undefined);
        result.slot = number();
        return result;
      })());
      result.value = value;
      return result;
    })());
  }
  $(notificationResultAndContext);
  function versionedMessageFromResponse(version, response) {
    if (version === 0) {
      return new MessageV0($(function () {
        let result = $Object.create(null, undefined);
        result.header = response.header;
        result.staticAccountKeys = response.accountKeys.map($(accountKey => new PublicKey(accountKey)));
        result.recentBlockhash = response.recentBlockhash;
        result.compiledInstructions = response.instructions.map($(ix => $(function () {
          let result = $Object.create(null, undefined);
          result.programIdIndex = ix.programIdIndex;
          result.accountKeyIndexes = ix.accounts;
          result.data = bs58$1.decode(ix.data);
          return result;
        })()));
        result.addressTableLookups = response.addressTableLookups;
        return result;
      })());
    } else {
      return new Message(response);
    }
  }
  $(versionedMessageFromResponse);
  const GetInflationGovernorResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.foundation = number();
    result.foundationTerm = number();
    result.initial = number();
    result.taper = number();
    result.terminal = number();
    return result;
  })());
  const GetInflationRewardResult = jsonRpcResult(array(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.epoch = number();
    result.effectiveSlot = number();
    result.amount = number();
    result.postBalance = number();
    result.commission = optional(nullable(number()));
    return result;
  })()))));
  const GetRecentPrioritizationFeesResult = array(type($(function () {
    let result = $Object.create(null, undefined);
    result.slot = number();
    result.prioritizationFee = number();
    return result;
  })()));
  const GetInflationRateResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.total = number();
    result.validator = number();
    result.foundation = number();
    result.epoch = number();
    return result;
  })());
  const GetEpochInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.epoch = number();
    result.slotIndex = number();
    result.slotsInEpoch = number();
    result.absoluteSlot = number();
    result.blockHeight = optional(number());
    result.transactionCount = optional(number());
    return result;
  })());
  const GetEpochScheduleResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.slotsPerEpoch = number();
    result.leaderScheduleSlotOffset = number();
    result.warmup = boolean();
    result.firstNormalEpoch = number();
    result.firstNormalSlot = number();
    return result;
  })());
  const GetLeaderScheduleResult = record(string(), array(number()));
  const TransactionErrorResult = nullable(union($Array.of(type($Object.create(null, undefined)), string())));
  const SignatureStatusResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.err = TransactionErrorResult;
    return result;
  })());
  const SignatureReceivedResult = literal("receivedSignature");
  const VersionResult = type($(function () {
    let result = $Object.create(null, undefined);
    result["solana-core"] = string();
    result["feature-set"] = optional(number());
    return result;
  })());
  const SimulatedTransactionResponseStruct = jsonRpcResultAndContext(type($(function () {
    let result = $Object.create(null, undefined);
    result.err = nullable(union($Array.of(type($Object.create(null, undefined)), string())));
    result.logs = nullable(array(string()));
    result.accounts = optional(nullable(array(nullable(type($(function () {
      let result = $Object.create(null, undefined);
      result.executable = boolean();
      result.owner = string();
      result.lamports = number();
      result.data = array(string());
      result.rentEpoch = optional(number());
      return result;
    })())))));
    result.unitsConsumed = optional(number());
    result.returnData = optional(nullable(type($(function () {
      let result = $Object.create(null, undefined);
      result.programId = string();
      result.data = tuple($Array.of(string(), literal("base64")));
      return result;
    })())));
    return result;
  })()));
  const BlockProductionResponseStruct = jsonRpcResultAndContext(type($(function () {
    let result = $Object.create(null, undefined);
    result.byIdentity = record(string(), array(number()));
    result.range = type($(function () {
      let result = $Object.create(null, undefined);
      result.firstSlot = number();
      result.lastSlot = number();
      return result;
    })());
    return result;
  })()));
  function createRpcClient(url, httpHeaders, customFetch, fetchMiddleware, disableRetryOnRateLimit, httpAgent) {
    const fetch = customFetch ? customFetch : fetchImpl;
    let agent;
    {
      if (httpAgent != null) {
        console.warn("You have supplied an `httpAgent` when creating a `Connection` in a browser environment." + "It has been ignored; `httpAgent` is only used in Node environments.");
      }
    }
    let fetchWithMiddleware;
    if (fetchMiddleware) {
      fetchWithMiddleware = $(async (info, init) => {
        const modifiedFetchArgs = await new Promise($((resolve, reject) => {
          try {
            fetchMiddleware(info, init, $((modifiedInfo, modifiedInit) => resolve($Array.of(modifiedInfo, modifiedInit))));
          } catch (error) {
            reject(error);
          }
        }));
        return await fetch(...modifiedFetchArgs);
      });
    }
    const clientBrowser = new RpcClient($(async (request, callback) => {
      const options = $(function () {
        let result = $Object.create(null, undefined);
        result.method = "POST";
        result.body = request;
        result.agent = agent;
        result.headers = $Object.assign($(function () {
          let result = $Object.create(null, undefined);
          result["Content-Type"] = "application/json";
          return result;
        })(), httpHeaders || $Object.create(null, undefined), COMMON_HTTP_HEADERS);
        return result;
      })();
      try {
        let too_many_requests_retries = 5;
        let res;
        let waitTime = 500;
        for (;;) {
          if (fetchWithMiddleware) {
            res = await fetchWithMiddleware(url, options);
          } else {
            res = await fetch(url, options);
          }
          if (res.status !== 429) {
            break;
          }
          if (disableRetryOnRateLimit === true) {
            break;
          }
          too_many_requests_retries -= 1;
          if (too_many_requests_retries === 0) {
            break;
          }
          console.error(`Server responded with ${res.status} ${res.statusText}.  Retrying after ${waitTime}ms delay...`);
          await sleep(waitTime);
          waitTime *= 2;
        }
        const text = await res.text();
        if (res.ok) {
          callback(null, text);
        } else {
          callback(new Error(`${res.status} ${res.statusText}: ${text}`));
        }
      } catch (err) {
        if (err instanceof Error) callback(err);
      }
    }), $Object.create(null, undefined));
    return clientBrowser;
  }
  $(createRpcClient);
  function createRpcRequest(client) {
    return $((method, args) => new Promise($((resolve, reject) => {
      client.request(method, args, $((err, response) => {
        if (err) {
          reject(err);
          return;
        }
        resolve(response);
      }));
    })));
  }
  $(createRpcRequest);
  function createRpcBatchRequest(client) {
    return $(requests => new Promise($((resolve, reject) => {
      if (requests.length === 0) resolve($Array.of());
      const batch = requests.map($(params => client.request(params.methodName, params.args)));
      client.request(batch, $((err, response) => {
        if (err) {
          reject(err);
          return;
        }
        resolve(response);
      }));
    })));
  }
  $(createRpcBatchRequest);
  const GetInflationGovernorRpcResult = jsonRpcResult(GetInflationGovernorResult);
  const GetInflationRateRpcResult = jsonRpcResult(GetInflationRateResult);
  const GetRecentPrioritizationFeesRpcResult = jsonRpcResult(GetRecentPrioritizationFeesResult);
  const GetEpochInfoRpcResult = jsonRpcResult(GetEpochInfoResult);
  const GetEpochScheduleRpcResult = jsonRpcResult(GetEpochScheduleResult);
  const GetLeaderScheduleRpcResult = jsonRpcResult(GetLeaderScheduleResult);
  const SlotRpcResult = jsonRpcResult(number());
  const GetSupplyRpcResult = jsonRpcResultAndContext(type($(function () {
    let result = $Object.create(null, undefined);
    result.total = number();
    result.circulating = number();
    result.nonCirculating = number();
    result.nonCirculatingAccounts = array(PublicKeyFromString);
    return result;
  })()));
  const TokenAmountResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.amount = string();
    result.uiAmount = nullable(number());
    result.decimals = number();
    result.uiAmountString = optional(string());
    return result;
  })());
  const GetTokenLargestAccountsResult = jsonRpcResultAndContext(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.address = PublicKeyFromString;
    result.amount = string();
    result.uiAmount = nullable(number());
    result.decimals = number();
    result.uiAmountString = optional(string());
    return result;
  })())));
  const GetTokenAccountsByOwner = jsonRpcResultAndContext(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.account = type($(function () {
      let result = $Object.create(null, undefined);
      result.executable = boolean();
      result.owner = PublicKeyFromString;
      result.lamports = number();
      result.data = BufferFromRawAccountData;
      result.rentEpoch = number();
      return result;
    })());
    return result;
  })())));
  const ParsedAccountDataResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.program = string();
    result.parsed = unknown();
    result.space = number();
    return result;
  })());
  const GetParsedTokenAccountsByOwner = jsonRpcResultAndContext(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.account = type($(function () {
      let result = $Object.create(null, undefined);
      result.executable = boolean();
      result.owner = PublicKeyFromString;
      result.lamports = number();
      result.data = ParsedAccountDataResult;
      result.rentEpoch = number();
      return result;
    })());
    return result;
  })())));
  const GetLargestAccountsRpcResult = jsonRpcResultAndContext(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.lamports = number();
    result.address = PublicKeyFromString;
    return result;
  })())));
  const AccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.executable = boolean();
    result.owner = PublicKeyFromString;
    result.lamports = number();
    result.data = BufferFromRawAccountData;
    result.rentEpoch = number();
    return result;
  })());
  const KeyedAccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.account = AccountInfoResult;
    return result;
  })());
  const ParsedOrRawAccountData = coerce(union($Array.of(instance(buffer.Buffer), ParsedAccountDataResult)), union($Array.of(RawAccountDataResult, ParsedAccountDataResult)), $(value => {
    if ($Array.isArray(value)) {
      return create(value, BufferFromRawAccountData);
    } else {
      return value;
    }
  }));
  const ParsedAccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.executable = boolean();
    result.owner = PublicKeyFromString;
    result.lamports = number();
    result.data = ParsedOrRawAccountData;
    result.rentEpoch = number();
    return result;
  })());
  const KeyedParsedAccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.account = ParsedAccountInfoResult;
    return result;
  })());
  const StakeActivationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.state = union($Array.of(literal("active"), literal("inactive"), literal("activating"), literal("deactivating")));
    result.active = number();
    result.inactive = number();
    return result;
  })());
  const GetConfirmedSignaturesForAddress2RpcResult = jsonRpcResult(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.signature = string();
    result.slot = number();
    result.err = TransactionErrorResult;
    result.memo = nullable(string());
    result.blockTime = optional(nullable(number()));
    return result;
  })())));
  const GetSignaturesForAddressRpcResult = jsonRpcResult(array(type($(function () {
    let result = $Object.create(null, undefined);
    result.signature = string();
    result.slot = number();
    result.err = TransactionErrorResult;
    result.memo = nullable(string());
    result.blockTime = optional(nullable(number()));
    return result;
  })())));
  const AccountNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = notificationResultAndContext(AccountInfoResult);
    return result;
  })());
  const ProgramAccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.account = AccountInfoResult;
    return result;
  })());
  const ProgramAccountNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = notificationResultAndContext(ProgramAccountInfoResult);
    return result;
  })());
  const SlotInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.parent = number();
    result.slot = number();
    result.root = number();
    return result;
  })());
  const SlotNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = SlotInfoResult;
    return result;
  })());
  const SlotUpdateResult = union($Array.of(type($(function () {
    let result = $Object.create(null, undefined);
    result.type = union($Array.of(literal("firstShredReceived"), literal("completed"), literal("optimisticConfirmation"), literal("root")));
    result.slot = number();
    result.timestamp = number();
    return result;
  })()), type($(function () {
    let result = $Object.create(null, undefined);
    result.type = literal("createdBank");
    result.parent = number();
    result.slot = number();
    result.timestamp = number();
    return result;
  })()), type($(function () {
    let result = $Object.create(null, undefined);
    result.type = literal("frozen");
    result.slot = number();
    result.timestamp = number();
    result.stats = type($(function () {
      let result = $Object.create(null, undefined);
      result.numTransactionEntries = number();
      result.numSuccessfulTransactions = number();
      result.numFailedTransactions = number();
      result.maxTransactionsPerEntry = number();
      return result;
    })());
    return result;
  })()), type($(function () {
    let result = $Object.create(null, undefined);
    result.type = literal("dead");
    result.slot = number();
    result.timestamp = number();
    result.err = string();
    return result;
  })())));
  const SlotUpdateNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = SlotUpdateResult;
    return result;
  })());
  const SignatureNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = notificationResultAndContext(union($Array.of(SignatureStatusResult, SignatureReceivedResult)));
    return result;
  })());
  const RootNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.subscription = number();
    result.result = number();
    return result;
  })());
  const ContactInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = string();
    result.gossip = nullable(string());
    result.tpu = nullable(string());
    result.rpc = nullable(string());
    result.version = nullable(string());
    return result;
  })());
  const VoteAccountInfoResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.votePubkey = string();
    result.nodePubkey = string();
    result.activatedStake = number();
    result.epochVoteAccount = boolean();
    result.epochCredits = array(tuple($Array.of(number(), number(), number())));
    result.commission = number();
    result.lastVote = number();
    result.rootSlot = nullable(number());
    return result;
  })());
  const GetVoteAccounts = jsonRpcResult(type($(function () {
    let result = $Object.create(null, undefined);
    result.current = array(VoteAccountInfoResult);
    result.delinquent = array(VoteAccountInfoResult);
    return result;
  })()));
  const ConfirmationStatus = union($Array.of(literal("processed"), literal("confirmed"), literal("finalized")));
  const SignatureStatusResponse = type($(function () {
    let result = $Object.create(null, undefined);
    result.slot = number();
    result.confirmations = nullable(number());
    result.err = TransactionErrorResult;
    result.confirmationStatus = optional(ConfirmationStatus);
    return result;
  })());
  const GetSignatureStatusesRpcResult = jsonRpcResultAndContext(array(nullable(SignatureStatusResponse)));
  const GetMinimumBalanceForRentExemptionRpcResult = jsonRpcResult(number());
  const AddressTableLookupStruct = type($(function () {
    let result = $Object.create(null, undefined);
    result.accountKey = PublicKeyFromString;
    result.writableIndexes = array(number());
    result.readonlyIndexes = array(number());
    return result;
  })());
  const ConfirmedTransactionResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.signatures = array(string());
    result.message = type($(function () {
      let result = $Object.create(null, undefined);
      result.accountKeys = array(string());
      result.header = type($(function () {
        let result = $Object.create(null, undefined);
        result.numRequiredSignatures = number();
        result.numReadonlySignedAccounts = number();
        result.numReadonlyUnsignedAccounts = number();
        return result;
      })());
      result.instructions = array(type($(function () {
        let result = $Object.create(null, undefined);
        result.accounts = array(number());
        result.data = string();
        result.programIdIndex = number();
        return result;
      })()));
      result.recentBlockhash = string();
      result.addressTableLookups = optional(array(AddressTableLookupStruct));
      return result;
    })());
    return result;
  })());
  const AnnotatedAccountKey = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = PublicKeyFromString;
    result.signer = boolean();
    result.writable = boolean();
    result.source = optional(union($Array.of(literal("transaction"), literal("lookupTable"))));
    return result;
  })());
  const ConfirmedTransactionAccountsModeResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.accountKeys = array(AnnotatedAccountKey);
    result.signatures = array(string());
    return result;
  })());
  const ParsedInstructionResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.parsed = unknown();
    result.program = string();
    result.programId = PublicKeyFromString;
    return result;
  })());
  const RawInstructionResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.accounts = array(PublicKeyFromString);
    result.data = string();
    result.programId = PublicKeyFromString;
    return result;
  })());
  const InstructionResult = union($Array.of(RawInstructionResult, ParsedInstructionResult));
  const UnknownInstructionResult = union($Array.of(type($(function () {
    let result = $Object.create(null, undefined);
    result.parsed = unknown();
    result.program = string();
    result.programId = string();
    return result;
  })()), type($(function () {
    let result = $Object.create(null, undefined);
    result.accounts = array(string());
    result.data = string();
    result.programId = string();
    return result;
  })())));
  const ParsedOrRawInstruction = coerce(InstructionResult, UnknownInstructionResult, $(value => {
    if ("accounts" in value) {
      return create(value, RawInstructionResult);
    } else {
      return create(value, ParsedInstructionResult);
    }
  }));
  const ParsedConfirmedTransactionResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.signatures = array(string());
    result.message = type($(function () {
      let result = $Object.create(null, undefined);
      result.accountKeys = array(AnnotatedAccountKey);
      result.instructions = array(ParsedOrRawInstruction);
      result.recentBlockhash = string();
      result.addressTableLookups = optional(nullable(array(AddressTableLookupStruct)));
      return result;
    })());
    return result;
  })());
  const TokenBalanceResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.accountIndex = number();
    result.mint = string();
    result.owner = optional(string());
    result.uiTokenAmount = TokenAmountResult;
    return result;
  })());
  const LoadedAddressesResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.writable = array(PublicKeyFromString);
    result.readonly = array(PublicKeyFromString);
    return result;
  })());
  const ConfirmedTransactionMetaResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.err = TransactionErrorResult;
    result.fee = number();
    result.innerInstructions = optional(nullable(array(type($(function () {
      let result = $Object.create(null, undefined);
      result.index = number();
      result.instructions = array(type($(function () {
        let result = $Object.create(null, undefined);
        result.accounts = array(number());
        result.data = string();
        result.programIdIndex = number();
        return result;
      })()));
      return result;
    })()))));
    result.preBalances = array(number());
    result.postBalances = array(number());
    result.logMessages = optional(nullable(array(string())));
    result.preTokenBalances = optional(nullable(array(TokenBalanceResult)));
    result.postTokenBalances = optional(nullable(array(TokenBalanceResult)));
    result.loadedAddresses = optional(LoadedAddressesResult);
    result.computeUnitsConsumed = optional(number());
    return result;
  })());
  const ParsedConfirmedTransactionMetaResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.err = TransactionErrorResult;
    result.fee = number();
    result.innerInstructions = optional(nullable(array(type($(function () {
      let result = $Object.create(null, undefined);
      result.index = number();
      result.instructions = array(ParsedOrRawInstruction);
      return result;
    })()))));
    result.preBalances = array(number());
    result.postBalances = array(number());
    result.logMessages = optional(nullable(array(string())));
    result.preTokenBalances = optional(nullable(array(TokenBalanceResult)));
    result.postTokenBalances = optional(nullable(array(TokenBalanceResult)));
    result.loadedAddresses = optional(LoadedAddressesResult);
    result.computeUnitsConsumed = optional(number());
    return result;
  })());
  const TransactionVersionStruct = union($Array.of(literal(0), literal("legacy")));
  const RewardsResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.pubkey = string();
    result.lamports = number();
    result.postBalance = nullable(number());
    result.rewardType = nullable(string());
    result.commission = optional(nullable(number()));
    return result;
  })());
  const GetBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.transactions = array(type($(function () {
      let result = $Object.create(null, undefined);
      result.transaction = ConfirmedTransactionResult;
      result.meta = nullable(ConfirmedTransactionMetaResult);
      result.version = optional(TransactionVersionStruct);
      return result;
    })()));
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetNoneModeBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetAccountsModeBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.transactions = array(type($(function () {
      let result = $Object.create(null, undefined);
      result.transaction = ConfirmedTransactionAccountsModeResult;
      result.meta = nullable(ConfirmedTransactionMetaResult);
      result.version = optional(TransactionVersionStruct);
      return result;
    })()));
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetParsedBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.transactions = array(type($(function () {
      let result = $Object.create(null, undefined);
      result.transaction = ParsedConfirmedTransactionResult;
      result.meta = nullable(ParsedConfirmedTransactionMetaResult);
      result.version = optional(TransactionVersionStruct);
      return result;
    })()));
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetParsedAccountsModeBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.transactions = array(type($(function () {
      let result = $Object.create(null, undefined);
      result.transaction = ConfirmedTransactionAccountsModeResult;
      result.meta = nullable(ParsedConfirmedTransactionMetaResult);
      result.version = optional(TransactionVersionStruct);
      return result;
    })()));
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetParsedNoneModeBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    result.blockHeight = nullable(number());
    return result;
  })())));
  const GetConfirmedBlockRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.transactions = array(type($(function () {
      let result = $Object.create(null, undefined);
      result.transaction = ConfirmedTransactionResult;
      result.meta = nullable(ConfirmedTransactionMetaResult);
      return result;
    })()));
    result.rewards = optional(array(RewardsResult));
    result.blockTime = nullable(number());
    return result;
  })())));
  const GetBlockSignaturesRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.previousBlockhash = string();
    result.parentSlot = number();
    result.signatures = array(string());
    result.blockTime = nullable(number());
    return result;
  })())));
  const GetTransactionRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.slot = number();
    result.meta = nullable(ConfirmedTransactionMetaResult);
    result.blockTime = optional(nullable(number()));
    result.transaction = ConfirmedTransactionResult;
    result.version = optional(TransactionVersionStruct);
    return result;
  })())));
  const GetParsedTransactionRpcResult = jsonRpcResult(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.slot = number();
    result.transaction = ParsedConfirmedTransactionResult;
    result.meta = nullable(ParsedConfirmedTransactionMetaResult);
    result.blockTime = optional(nullable(number()));
    result.version = optional(TransactionVersionStruct);
    return result;
  })())));
  const GetRecentBlockhashAndContextRpcResult = jsonRpcResultAndContext(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.feeCalculator = type($(function () {
      let result = $Object.create(null, undefined);
      result.lamportsPerSignature = number();
      return result;
    })());
    return result;
  })()));
  const GetLatestBlockhashRpcResult = jsonRpcResultAndContext(type($(function () {
    let result = $Object.create(null, undefined);
    result.blockhash = string();
    result.lastValidBlockHeight = number();
    return result;
  })()));
  const IsBlockhashValidRpcResult = jsonRpcResultAndContext(boolean());
  const PerfSampleResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.slot = number();
    result.numTransactions = number();
    result.numSlots = number();
    result.samplePeriodSecs = number();
    return result;
  })());
  const GetRecentPerformanceSamplesRpcResult = jsonRpcResult(array(PerfSampleResult));
  const GetFeeCalculatorRpcResult = jsonRpcResultAndContext(nullable(type($(function () {
    let result = $Object.create(null, undefined);
    result.feeCalculator = type($(function () {
      let result = $Object.create(null, undefined);
      result.lamportsPerSignature = number();
      return result;
    })());
    return result;
  })())));
  const RequestAirdropRpcResult = jsonRpcResult(string());
  const SendTransactionRpcResult = jsonRpcResult(string());
  const LogsResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.err = TransactionErrorResult;
    result.logs = array(string());
    result.signature = string();
    return result;
  })());
  const LogsNotificationResult = type($(function () {
    let result = $Object.create(null, undefined);
    result.result = notificationResultAndContext(LogsResult);
    result.subscription = number();
    return result;
  })());
  const COMMON_HTTP_HEADERS = $(function () {
    let result = $Object.create(null, undefined);
    result["solana-client"] = `js/${"0.0.0-development"}`;
    return result;
  })();
  class Connection {
    constructor(endpoint, _commitmentOrConfig) {
      this._commitment = void 0;
      this._confirmTransactionInitialTimeout = void 0;
      this._rpcEndpoint = void 0;
      this._rpcWsEndpoint = void 0;
      this._rpcClient = void 0;
      this._rpcRequest = void 0;
      this._rpcBatchRequest = void 0;
      this._rpcWebSocket = void 0;
      this._rpcWebSocketConnected = false;
      this._rpcWebSocketHeartbeat = null;
      this._rpcWebSocketIdleTimeout = null;
      this._rpcWebSocketGeneration = 0;
      this._disableBlockhashCaching = false;
      this._pollingBlockhash = false;
      this._blockhashInfo = $(function () {
        let result = $Object.create(null, undefined);
        result.latestBlockhash = null;
        result.lastFetch = 0;
        result.transactionSignatures = $Array.of();
        result.simulatedSignatures = $Array.of();
        return result;
      })();
      this._nextClientSubscriptionId = 0;
      this._subscriptionDisposeFunctionsByClientSubscriptionId = $Object.create(null, undefined);
      this._subscriptionHashByClientSubscriptionId = $Object.create(null, undefined);
      this._subscriptionStateChangeCallbacksByHash = $Object.create(null, undefined);
      this._subscriptionCallbacksByServerSubscriptionId = $Object.create(null, undefined);
      this._subscriptionsByHash = $Object.create(null, undefined);
      this._subscriptionsAutoDisposedByRpc = new Set();
      this.getBlockHeight = $(() => {
        const requestPromises = $Object.create(null, undefined);
        return $(async commitmentOrConfig => {
          const {
            commitment: commitment,
            config: config
          } = extractCommitmentFromConfig(commitmentOrConfig);
          const args = this._buildArgs($Array.of(), commitment, undefined, config);
          const requestHash = fastStableStringify$1(args);
          requestPromises[requestHash] = requestPromises[requestHash] ?? $(async () => {
            try {
              const unsafeRes = await this._rpcRequest("getBlockHeight", args);
              const res = create(unsafeRes, jsonRpcResult(number()));
              if ("error" in res) {
                throw new SolanaJSONRPCError(res.error, "failed to get block height information");
              }
              return res.result;
            } finally {
              delete requestPromises[requestHash];
            }
          })();
          return await requestPromises[requestHash];
        });
      })();
      let wsEndpoint;
      let httpHeaders;
      let fetch;
      let fetchMiddleware;
      let disableRetryOnRateLimit;
      let httpAgent;
      if (_commitmentOrConfig && typeof _commitmentOrConfig === "string") {
        this._commitment = _commitmentOrConfig;
      } else if (_commitmentOrConfig) {
        this._commitment = _commitmentOrConfig.commitment;
        this._confirmTransactionInitialTimeout = _commitmentOrConfig.confirmTransactionInitialTimeout;
        wsEndpoint = _commitmentOrConfig.wsEndpoint;
        httpHeaders = _commitmentOrConfig.httpHeaders;
        fetch = _commitmentOrConfig.fetch;
        fetchMiddleware = _commitmentOrConfig.fetchMiddleware;
        disableRetryOnRateLimit = _commitmentOrConfig.disableRetryOnRateLimit;
        httpAgent = _commitmentOrConfig.httpAgent;
      }
      this._rpcEndpoint = assertEndpointUrl(endpoint);
      this._rpcWsEndpoint = wsEndpoint || makeWebsocketUrl(endpoint);
      this._rpcClient = createRpcClient(endpoint, httpHeaders, fetch, fetchMiddleware, disableRetryOnRateLimit, httpAgent);
      this._rpcRequest = createRpcRequest(this._rpcClient);
      this._rpcBatchRequest = createRpcBatchRequest(this._rpcClient);
      this._rpcWebSocket = new RpcWebSocketClient(this._rpcWsEndpoint, $(function () {
        let result = $Object.create(null, undefined);
        result.autoconnect = false;
        result.max_reconnects = Infinity;
        return result;
      })());
      this._rpcWebSocket.on("open", this._wsOnOpen.bind(this));
      this._rpcWebSocket.on("error", this._wsOnError.bind(this));
      this._rpcWebSocket.on("close", this._wsOnClose.bind(this));
      this._rpcWebSocket.on("accountNotification", this._wsOnAccountNotification.bind(this));
      this._rpcWebSocket.on("programNotification", this._wsOnProgramAccountNotification.bind(this));
      this._rpcWebSocket.on("slotNotification", this._wsOnSlotNotification.bind(this));
      this._rpcWebSocket.on("slotsUpdatesNotification", this._wsOnSlotUpdatesNotification.bind(this));
      this._rpcWebSocket.on("signatureNotification", this._wsOnSignatureNotification.bind(this));
      this._rpcWebSocket.on("rootNotification", this._wsOnRootNotification.bind(this));
      this._rpcWebSocket.on("logsNotification", this._wsOnLogsNotification.bind(this));
    }
    get commitment() {
      return this._commitment;
    }
    get rpcEndpoint() {
      return this._rpcEndpoint;
    }
    async getBalanceAndContext(publicKey, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(publicKey.toBase58()), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getBalance", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(number()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get balance for ${publicKey.toBase58()}`);
      }
      return res.result;
    }
    async getBalance(publicKey, commitmentOrConfig) {
      return await this.getBalanceAndContext(publicKey, commitmentOrConfig).then($(x => x.value)).catch($(e => {
        throw new Error("failed to get balance of account " + publicKey.toBase58() + ": " + e);
      }));
    }
    async getBlockTime(slot) {
      const unsafeRes = await this._rpcRequest("getBlockTime", $Array.of(slot));
      const res = create(unsafeRes, jsonRpcResult(nullable(number())));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get block time for slot ${slot}`);
      }
      return res.result;
    }
    async getMinimumLedgerSlot() {
      const unsafeRes = await this._rpcRequest("minimumLedgerSlot", $Array.of());
      const res = create(unsafeRes, jsonRpcResult(number()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get minimum ledger slot");
      }
      return res.result;
    }
    async getFirstAvailableBlock() {
      const unsafeRes = await this._rpcRequest("getFirstAvailableBlock", $Array.of());
      const res = create(unsafeRes, SlotRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get first available block");
      }
      return res.result;
    }
    async getSupply(config) {
      let configArg = $Object.create(null, undefined);
      if (typeof config === "string") {
        configArg = $(function () {
          let result = $Object.create(null, undefined);
          result.commitment = config;
          return result;
        })();
      } else if (config) {
        configArg = $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, config);
          result.commitment = config && config.commitment || this.commitment;
          return result;
        }).bind(this)();
      } else {
        configArg = $(function () {
          let result = $Object.create(null, undefined);
          result.commitment = this.commitment;
          return result;
        }).bind(this)();
      }
      const unsafeRes = await this._rpcRequest("getSupply", $Array.of(configArg));
      const res = create(unsafeRes, GetSupplyRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get supply");
      }
      return res.result;
    }
    async getTokenSupply(tokenMintAddress, commitment) {
      const args = this._buildArgs($Array.of(tokenMintAddress.toBase58()), commitment);
      const unsafeRes = await this._rpcRequest("getTokenSupply", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(TokenAmountResult));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get token supply");
      }
      return res.result;
    }
    async getTokenAccountBalance(tokenAddress, commitment) {
      const args = this._buildArgs($Array.of(tokenAddress.toBase58()), commitment);
      const unsafeRes = await this._rpcRequest("getTokenAccountBalance", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(TokenAmountResult));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get token account balance");
      }
      return res.result;
    }
    async getTokenAccountsByOwner(ownerAddress, filter, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      let _args = $Array.of(ownerAddress.toBase58());
      if ("mint" in filter) {
        _args.push($(function () {
          let result = $Object.create(null, undefined);
          result.mint = filter.mint.toBase58();
          return result;
        })());
      } else {
        _args.push($(function () {
          let result = $Object.create(null, undefined);
          result.programId = filter.programId.toBase58();
          return result;
        })());
      }
      const args = this._buildArgs(_args, commitment, "base64", config);
      const unsafeRes = await this._rpcRequest("getTokenAccountsByOwner", args);
      const res = create(unsafeRes, GetTokenAccountsByOwner);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get token accounts owned by account ${ownerAddress.toBase58()}`);
      }
      return res.result;
    }
    async getParsedTokenAccountsByOwner(ownerAddress, filter, commitment) {
      let _args = $Array.of(ownerAddress.toBase58());
      if ("mint" in filter) {
        _args.push($(function () {
          let result = $Object.create(null, undefined);
          result.mint = filter.mint.toBase58();
          return result;
        })());
      } else {
        _args.push($(function () {
          let result = $Object.create(null, undefined);
          result.programId = filter.programId.toBase58();
          return result;
        })());
      }
      const args = this._buildArgs(_args, commitment, "jsonParsed");
      const unsafeRes = await this._rpcRequest("getTokenAccountsByOwner", args);
      const res = create(unsafeRes, GetParsedTokenAccountsByOwner);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get token accounts owned by account ${ownerAddress.toBase58()}`);
      }
      return res.result;
    }
    async getLargestAccounts(config) {
      const arg = $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, config);
        result.commitment = config && config.commitment || this.commitment;
        return result;
      }).bind(this)();
      const args = arg.filter || arg.commitment ? $Array.of(arg) : $Array.of();
      const unsafeRes = await this._rpcRequest("getLargestAccounts", args);
      const res = create(unsafeRes, GetLargestAccountsRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get largest accounts");
      }
      return res.result;
    }
    async getTokenLargestAccounts(mintAddress, commitment) {
      const args = this._buildArgs($Array.of(mintAddress.toBase58()), commitment);
      const unsafeRes = await this._rpcRequest("getTokenLargestAccounts", args);
      const res = create(unsafeRes, GetTokenLargestAccountsResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get token largest accounts");
      }
      return res.result;
    }
    async getAccountInfoAndContext(publicKey, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(publicKey.toBase58()), commitment, "base64", config);
      const unsafeRes = await this._rpcRequest("getAccountInfo", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(nullable(AccountInfoResult)));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get info about account ${publicKey.toBase58()}`);
      }
      return res.result;
    }
    async getParsedAccountInfo(publicKey, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(publicKey.toBase58()), commitment, "jsonParsed", config);
      const unsafeRes = await this._rpcRequest("getAccountInfo", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(nullable(ParsedAccountInfoResult)));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get info about account ${publicKey.toBase58()}`);
      }
      return res.result;
    }
    async getAccountInfo(publicKey, commitmentOrConfig) {
      try {
        const res = await this.getAccountInfoAndContext(publicKey, commitmentOrConfig);
        return res.value;
      } catch (e) {
        throw new Error("failed to get info about account " + publicKey.toBase58() + ": " + e);
      }
    }
    async getMultipleParsedAccounts(publicKeys, rawConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(rawConfig);
      const keys = publicKeys.map($(key => key.toBase58()));
      const args = this._buildArgs($Array.of(keys), commitment, "jsonParsed", config);
      const unsafeRes = await this._rpcRequest("getMultipleAccounts", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(array(nullable(ParsedAccountInfoResult))));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get info for accounts ${keys}`);
      }
      return res.result;
    }
    async getMultipleAccountsInfoAndContext(publicKeys, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const keys = publicKeys.map($(key => key.toBase58()));
      const args = this._buildArgs($Array.of(keys), commitment, "base64", config);
      const unsafeRes = await this._rpcRequest("getMultipleAccounts", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(array(nullable(AccountInfoResult))));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get info for accounts ${keys}`);
      }
      return res.result;
    }
    async getMultipleAccountsInfo(publicKeys, commitmentOrConfig) {
      const res = await this.getMultipleAccountsInfoAndContext(publicKeys, commitmentOrConfig);
      return res.value;
    }
    async getStakeActivation(publicKey, commitmentOrConfig, epoch) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(publicKey.toBase58()), commitment, undefined, $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, config);
        result.epoch = epoch != null ? epoch : config?.epoch;
        return result;
      })());
      const unsafeRes = await this._rpcRequest("getStakeActivation", args);
      const res = create(unsafeRes, jsonRpcResult(StakeActivationResult));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get Stake Activation ${publicKey.toBase58()}`);
      }
      return res.result;
    }
    async getProgramAccounts(programId, configOrCommitment) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(configOrCommitment);
      const {
        encoding: encoding,
        ...configWithoutEncoding
      } = config || $Object.create(null, undefined);
      const args = this._buildArgs($Array.of(programId.toBase58()), commitment, encoding || "base64", configWithoutEncoding);
      const unsafeRes = await this._rpcRequest("getProgramAccounts", args);
      const baseSchema = array(KeyedAccountInfoResult);
      const res = configWithoutEncoding.withContext === true ? create(unsafeRes, jsonRpcResultAndContext(baseSchema)) : create(unsafeRes, jsonRpcResult(baseSchema));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get accounts owned by program ${programId.toBase58()}`);
      }
      return res.result;
    }
    async getParsedProgramAccounts(programId, configOrCommitment) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(configOrCommitment);
      const args = this._buildArgs($Array.of(programId.toBase58()), commitment, "jsonParsed", config);
      const unsafeRes = await this._rpcRequest("getProgramAccounts", args);
      const res = create(unsafeRes, jsonRpcResult(array(KeyedParsedAccountInfoResult)));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get accounts owned by program ${programId.toBase58()}`);
      }
      return res.result;
    }
    async confirmTransaction(strategy, commitment) {
      let rawSignature;
      if (typeof strategy == "string") {
        rawSignature = strategy;
      } else {
        const config = strategy;
        if (config.abortSignal?.aborted) {
          return Promise.reject(config.abortSignal.reason);
        }
        rawSignature = config.signature;
      }
      let decodedSignature;
      try {
        decodedSignature = bs58$1.decode(rawSignature);
      } catch (err) {
        throw new Error("signature must be base58 encoded: " + rawSignature);
      }
      assert$1(decodedSignature.length === 64, "signature has invalid length");
      if (typeof strategy === "string") {
        return await this.confirmTransactionUsingLegacyTimeoutStrategy($(function () {
          let result = $Object.create(null, undefined);
          result.commitment = commitment || this.commitment;
          result.signature = rawSignature;
          return result;
        }).bind(this)());
      } else if ("lastValidBlockHeight" in strategy) {
        return await this.confirmTransactionUsingBlockHeightExceedanceStrategy($(function () {
          let result = $Object.create(null, undefined);
          result.commitment = commitment || this.commitment;
          result.strategy = strategy;
          return result;
        }).bind(this)());
      } else {
        return await this.confirmTransactionUsingDurableNonceStrategy($(function () {
          let result = $Object.create(null, undefined);
          result.commitment = commitment || this.commitment;
          result.strategy = strategy;
          return result;
        }).bind(this)());
      }
    }
    getCancellationPromise(signal) {
      return new Promise($((_, reject) => {
        if (signal == null) {
          return;
        }
        if (signal.aborted) {
          reject(signal.reason);
        } else {
          signal.addEventListener("abort", $(() => {
            reject(signal.reason);
          }));
        }
      }));
    }
    getTransactionConfirmationPromise({
      commitment: commitment,
      signature: signature
    }) {
      let signatureSubscriptionId;
      let disposeSignatureSubscriptionStateChangeObserver;
      let done = false;
      const confirmationPromise = new Promise($((resolve, reject) => {
        try {
          signatureSubscriptionId = this.onSignature(signature, $((result, context) => {
            signatureSubscriptionId = undefined;
            const response = $(function () {
              let result = $Object.create(null, undefined);
              result.context = context;
              result.value = result;
              return result;
            })();
            resolve($(function () {
              let result = $Object.create(null, undefined);
              result.__type = TransactionStatus.PROCESSED;
              result.response = response;
              return result;
            })());
          }), commitment);
          const subscriptionSetupPromise = new Promise($(resolveSubscriptionSetup => {
            if (signatureSubscriptionId == null) {
              resolveSubscriptionSetup();
            } else {
              disposeSignatureSubscriptionStateChangeObserver = this._onSubscriptionStateChange(signatureSubscriptionId, $(nextState => {
                if (nextState === "subscribed") {
                  resolveSubscriptionSetup();
                }
              }));
            }
          }));
          $(async () => {
            await subscriptionSetupPromise;
            if (done) return;
            const response = await this.getSignatureStatus(signature);
            if (done) return;
            if (response == null) {
              return;
            }
            const {
              context: context,
              value: value
            } = response;
            if (value == null) {
              return;
            }
            if (value?.err) {
              reject(value.err);
            } else {
              switch (commitment) {
                case "confirmed":
                case "single":
                case "singleGossip":
                  {
                    if (value.confirmationStatus === "processed") {
                      return;
                    }
                    break;
                  }
                case "finalized":
                case "max":
                case "root":
                  {
                    if (value.confirmationStatus === "processed" || value.confirmationStatus === "confirmed") {
                      return;
                    }
                    break;
                  }
                case "processed":
                case "recent":
              }
              done = true;
              resolve($(function () {
                let result = $Object.create(null, undefined);
                result.__type = TransactionStatus.PROCESSED;
                result.response = $(function () {
                  let result = $Object.create(null, undefined);
                  result.context = context;
                  result.value = value;
                  return result;
                })();
                return result;
              })());
            }
          })();
        } catch (err) {
          reject(err);
        }
      }));
      const abortConfirmation = () => {
        if (disposeSignatureSubscriptionStateChangeObserver) {
          disposeSignatureSubscriptionStateChangeObserver();
          disposeSignatureSubscriptionStateChangeObserver = undefined;
        }
        if (signatureSubscriptionId != null) {
          this.removeSignatureListener(signatureSubscriptionId);
          signatureSubscriptionId = undefined;
        }
      };
      $(abortConfirmation);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.abortConfirmation = abortConfirmation;
        result.confirmationPromise = confirmationPromise;
        return result;
      })();
    }
    async confirmTransactionUsingBlockHeightExceedanceStrategy({
      commitment: commitment,
      strategy: {
        abortSignal: abortSignal,
        lastValidBlockHeight: lastValidBlockHeight,
        signature: signature
      }
    }) {
      let done = false;
      const expiryPromise = new Promise($(resolve => {
        const checkBlockHeight = async () => {
          try {
            const blockHeight = await this.getBlockHeight(commitment);
            return blockHeight;
          } catch (_e) {
            return -1;
          }
        };
        $(checkBlockHeight);
        $(async () => {
          let currentBlockHeight = await checkBlockHeight();
          if (done) return;
          while (currentBlockHeight <= lastValidBlockHeight) {
            await sleep(1e3);
            if (done) return;
            currentBlockHeight = await checkBlockHeight();
            if (done) return;
          }
          resolve($(function () {
            let result = $Object.create(null, undefined);
            result.__type = TransactionStatus.BLOCKHEIGHT_EXCEEDED;
            return result;
          })());
        })();
      }));
      const {
        abortConfirmation: abortConfirmation,
        confirmationPromise: confirmationPromise
      } = this.getTransactionConfirmationPromise($(function () {
        let result = $Object.create(null, undefined);
        result.commitment = commitment;
        result.signature = signature;
        return result;
      })());
      const cancellationPromise = this.getCancellationPromise(abortSignal);
      let result;
      try {
        const outcome = await Promise.race($Array.of(cancellationPromise, confirmationPromise, expiryPromise));
        if (outcome.__type === TransactionStatus.PROCESSED) {
          result = outcome.response;
        } else {
          throw new TransactionExpiredBlockheightExceededError(signature);
        }
      } finally {
        done = true;
        abortConfirmation();
      }
      return result;
    }
    async confirmTransactionUsingDurableNonceStrategy({
      commitment: commitment,
      strategy: {
        abortSignal: abortSignal,
        minContextSlot: minContextSlot,
        nonceAccountPubkey: nonceAccountPubkey,
        nonceValue: nonceValue,
        signature: signature
      }
    }) {
      let done = false;
      const expiryPromise = new Promise($(resolve => {
        let currentNonceValue = nonceValue;
        let lastCheckedSlot = null;
        const getCurrentNonceValue = async () => {
          try {
            const {
              context: context,
              value: nonceAccount
            } = await this.getNonceAndContext(nonceAccountPubkey, $(function () {
              let result = $Object.create(null, undefined);
              result.commitment = commitment;
              result.minContextSlot = minContextSlot;
              return result;
            })());
            lastCheckedSlot = context.slot;
            return nonceAccount?.nonce;
          } catch (e) {
            return currentNonceValue;
          }
        };
        $(getCurrentNonceValue);
        $(async () => {
          currentNonceValue = await getCurrentNonceValue();
          if (done) return;
          while (true) {
            if (nonceValue !== currentNonceValue) {
              resolve($(function () {
                let result = $Object.create(null, undefined);
                result.__type = TransactionStatus.NONCE_INVALID;
                result.slotInWhichNonceDidAdvance = lastCheckedSlot;
                return result;
              })());
              return;
            }
            await sleep(2e3);
            if (done) return;
            currentNonceValue = await getCurrentNonceValue();
            if (done) return;
          }
        })();
      }));
      const {
        abortConfirmation: abortConfirmation,
        confirmationPromise: confirmationPromise
      } = this.getTransactionConfirmationPromise($(function () {
        let result = $Object.create(null, undefined);
        result.commitment = commitment;
        result.signature = signature;
        return result;
      })());
      const cancellationPromise = this.getCancellationPromise(abortSignal);
      let result;
      try {
        const outcome = await Promise.race($Array.of(cancellationPromise, confirmationPromise, expiryPromise));
        if (outcome.__type === TransactionStatus.PROCESSED) {
          result = outcome.response;
        } else {
          let signatureStatus;
          while (true) {
            const status = await this.getSignatureStatus(signature);
            if (status == null) {
              break;
            }
            if (status.context.slot < (outcome.slotInWhichNonceDidAdvance ?? minContextSlot)) {
              await sleep(400);
              continue;
            }
            signatureStatus = status;
            break;
          }
          if (signatureStatus?.value) {
            const commitmentForStatus = commitment || "finalized";
            const {
              confirmationStatus: confirmationStatus
            } = signatureStatus.value;
            switch (commitmentForStatus) {
              case "processed":
              case "recent":
                if (confirmationStatus !== "processed" && confirmationStatus !== "confirmed" && confirmationStatus !== "finalized") {
                  throw new TransactionExpiredNonceInvalidError(signature);
                }
                break;
              case "confirmed":
              case "single":
              case "singleGossip":
                if (confirmationStatus !== "confirmed" && confirmationStatus !== "finalized") {
                  throw new TransactionExpiredNonceInvalidError(signature);
                }
                break;
              case "finalized":
              case "max":
              case "root":
                if (confirmationStatus !== "finalized") {
                  throw new TransactionExpiredNonceInvalidError(signature);
                }
                break;
              default:
                $(_ => {})(commitmentForStatus);
            }
            result = $(function () {
              let result = $Object.create(null, undefined);
              result.context = signatureStatus.context;
              result.value = $(function () {
                let result = $Object.create(null, undefined);
                result.err = signatureStatus.value.err;
                return result;
              })();
              return result;
            })();
          } else {
            throw new TransactionExpiredNonceInvalidError(signature);
          }
        }
      } finally {
        done = true;
        abortConfirmation();
      }
      return result;
    }
    async confirmTransactionUsingLegacyTimeoutStrategy({
      commitment: commitment,
      signature: signature
    }) {
      let timeoutId;
      const expiryPromise = new Promise($(resolve => {
        let timeoutMs = this._confirmTransactionInitialTimeout || 60 * 1e3;
        switch (commitment) {
          case "processed":
          case "recent":
          case "single":
          case "confirmed":
          case "singleGossip":
            {
              timeoutMs = this._confirmTransactionInitialTimeout || 30 * 1e3;
              break;
            }
        }
        timeoutId = setTimeout($(() => resolve($(function () {
          let result = $Object.create(null, undefined);
          result.__type = TransactionStatus.TIMED_OUT;
          result.timeoutMs = timeoutMs;
          return result;
        })())), timeoutMs);
      }));
      const {
        abortConfirmation: abortConfirmation,
        confirmationPromise: confirmationPromise
      } = this.getTransactionConfirmationPromise($(function () {
        let result = $Object.create(null, undefined);
        result.commitment = commitment;
        result.signature = signature;
        return result;
      })());
      let result;
      try {
        const outcome = await Promise.race($Array.of(confirmationPromise, expiryPromise));
        if (outcome.__type === TransactionStatus.PROCESSED) {
          result = outcome.response;
        } else {
          throw new TransactionExpiredTimeoutError(signature, outcome.timeoutMs / 1e3);
        }
      } finally {
        clearTimeout(timeoutId);
        abortConfirmation();
      }
      return result;
    }
    async getClusterNodes() {
      const unsafeRes = await this._rpcRequest("getClusterNodes", $Array.of());
      const res = create(unsafeRes, jsonRpcResult(array(ContactInfoResult)));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get cluster nodes");
      }
      return res.result;
    }
    async getVoteAccounts(commitment) {
      const args = this._buildArgs($Array.of(), commitment);
      const unsafeRes = await this._rpcRequest("getVoteAccounts", args);
      const res = create(unsafeRes, GetVoteAccounts);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get vote accounts");
      }
      return res.result;
    }
    async getSlot(commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getSlot", args);
      const res = create(unsafeRes, jsonRpcResult(number()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get slot");
      }
      return res.result;
    }
    async getSlotLeader(commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getSlotLeader", args);
      const res = create(unsafeRes, jsonRpcResult(string()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get slot leader");
      }
      return res.result;
    }
    async getSlotLeaders(startSlot, limit) {
      const args = $Array.of(startSlot, limit);
      const unsafeRes = await this._rpcRequest("getSlotLeaders", args);
      const res = create(unsafeRes, jsonRpcResult(array(PublicKeyFromString)));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get slot leaders");
      }
      return res.result;
    }
    async getSignatureStatus(signature, config) {
      const {
        context: context,
        value: values
      } = await this.getSignatureStatuses($Array.of(signature), config);
      assert$1(values.length === 1);
      const value = values[0];
      return $(function () {
        let result = $Object.create(null, undefined);
        result.context = context;
        result.value = value;
        return result;
      })();
    }
    async getSignatureStatuses(signatures, config) {
      const params = $Array.of(signatures);
      if (config) {
        params.push(config);
      }
      const unsafeRes = await this._rpcRequest("getSignatureStatuses", params);
      const res = create(unsafeRes, GetSignatureStatusesRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get signature status");
      }
      return res.result;
    }
    async getTransactionCount(commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getTransactionCount", args);
      const res = create(unsafeRes, jsonRpcResult(number()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get transaction count");
      }
      return res.result;
    }
    async getTotalSupply(commitment) {
      const result = await this.getSupply($(function () {
        let result = $Object.create(null, undefined);
        result.commitment = commitment;
        result.excludeNonCirculatingAccountsList = true;
        return result;
      })());
      return result.value.total;
    }
    async getInflationGovernor(commitment) {
      const args = this._buildArgs($Array.of(), commitment);
      const unsafeRes = await this._rpcRequest("getInflationGovernor", args);
      const res = create(unsafeRes, GetInflationGovernorRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get inflation");
      }
      return res.result;
    }
    async getInflationReward(addresses, epoch, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(addresses.map($(pubkey => pubkey.toBase58()))), commitment, undefined, $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, config);
        result.epoch = epoch != null ? epoch : config?.epoch;
        return result;
      })());
      const unsafeRes = await this._rpcRequest("getInflationReward", args);
      const res = create(unsafeRes, GetInflationRewardResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get inflation reward");
      }
      return res.result;
    }
    async getInflationRate() {
      const unsafeRes = await this._rpcRequest("getInflationRate", $Array.of());
      const res = create(unsafeRes, GetInflationRateRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get inflation rate");
      }
      return res.result;
    }
    async getEpochInfo(commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getEpochInfo", args);
      const res = create(unsafeRes, GetEpochInfoRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get epoch info");
      }
      return res.result;
    }
    async getEpochSchedule() {
      const unsafeRes = await this._rpcRequest("getEpochSchedule", $Array.of());
      const res = create(unsafeRes, GetEpochScheduleRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get epoch schedule");
      }
      const epochSchedule = res.result;
      return new EpochSchedule(epochSchedule.slotsPerEpoch, epochSchedule.leaderScheduleSlotOffset, epochSchedule.warmup, epochSchedule.firstNormalEpoch, epochSchedule.firstNormalSlot);
    }
    async getLeaderSchedule() {
      const unsafeRes = await this._rpcRequest("getLeaderSchedule", $Array.of());
      const res = create(unsafeRes, GetLeaderScheduleRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get leader schedule");
      }
      return res.result;
    }
    async getMinimumBalanceForRentExemption(dataLength, commitment) {
      const args = this._buildArgs($Array.of(dataLength), commitment);
      const unsafeRes = await this._rpcRequest("getMinimumBalanceForRentExemption", args);
      const res = create(unsafeRes, GetMinimumBalanceForRentExemptionRpcResult);
      if ("error" in res) {
        console.warn("Unable to fetch minimum balance for rent exemption");
        return 0;
      }
      return res.result;
    }
    async getRecentBlockhashAndContext(commitment) {
      const args = this._buildArgs($Array.of(), commitment);
      const unsafeRes = await this._rpcRequest("getRecentBlockhash", args);
      const res = create(unsafeRes, GetRecentBlockhashAndContextRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get recent blockhash");
      }
      return res.result;
    }
    async getRecentPerformanceSamples(limit) {
      const unsafeRes = await this._rpcRequest("getRecentPerformanceSamples", limit ? $Array.of(limit) : $Array.of());
      const res = create(unsafeRes, GetRecentPerformanceSamplesRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get recent performance samples");
      }
      return res.result;
    }
    async getFeeCalculatorForBlockhash(blockhash, commitment) {
      const args = this._buildArgs($Array.of(blockhash), commitment);
      const unsafeRes = await this._rpcRequest("getFeeCalculatorForBlockhash", args);
      const res = create(unsafeRes, GetFeeCalculatorRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get fee calculator");
      }
      const {
        context: context,
        value: value
      } = res.result;
      return $(function () {
        let result = $Object.create(null, undefined);
        result.context = context;
        result.value = value !== null ? value.feeCalculator : null;
        return result;
      })();
    }
    async getFeeForMessage(message, commitment) {
      const wireMessage = toBuffer(message.serialize()).toString("base64");
      const args = this._buildArgs($Array.of(wireMessage), commitment);
      const unsafeRes = await this._rpcRequest("getFeeForMessage", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(nullable(number())));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get fee for message");
      }
      if (res.result === null) {
        throw new Error("invalid blockhash");
      }
      return res.result;
    }
    async getRecentPrioritizationFees(config) {
      const accounts = config?.lockedWritableAccounts?.map($(key => key.toBase58()));
      const args = accounts?.length ? $Array.of(accounts) : $Array.of();
      const unsafeRes = await this._rpcRequest("getRecentPrioritizationFees", args);
      const res = create(unsafeRes, GetRecentPrioritizationFeesRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get recent prioritization fees");
      }
      return res.result;
    }
    async getRecentBlockhash(commitment) {
      try {
        const res = await this.getRecentBlockhashAndContext(commitment);
        return res.value;
      } catch (e) {
        throw new Error("failed to get recent blockhash: " + e);
      }
    }
    async getLatestBlockhash(commitmentOrConfig) {
      try {
        const res = await this.getLatestBlockhashAndContext(commitmentOrConfig);
        return res.value;
      } catch (e) {
        throw new Error("failed to get recent blockhash: " + e);
      }
    }
    async getLatestBlockhashAndContext(commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgs($Array.of(), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getLatestBlockhash", args);
      const res = create(unsafeRes, GetLatestBlockhashRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get latest blockhash");
      }
      return res.result;
    }
    async isBlockhashValid(blockhash, rawConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(rawConfig);
      const args = this._buildArgs($Array.of(blockhash), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("isBlockhashValid", args);
      const res = create(unsafeRes, IsBlockhashValidRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to determine if the blockhash `" + blockhash + "`is valid");
      }
      return res.result;
    }
    async getVersion() {
      const unsafeRes = await this._rpcRequest("getVersion", $Array.of());
      const res = create(unsafeRes, jsonRpcResult(VersionResult));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get version");
      }
      return res.result;
    }
    async getGenesisHash() {
      const unsafeRes = await this._rpcRequest("getGenesisHash", $Array.of());
      const res = create(unsafeRes, jsonRpcResult(string()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get genesis hash");
      }
      return res.result;
    }
    async getBlock(slot, rawConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(rawConfig);
      const args = this._buildArgsAtLeastConfirmed($Array.of(slot), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getBlock", args);
      try {
        switch (config?.transactionDetails) {
          case "accounts":
            {
              const res = create(unsafeRes, GetAccountsModeBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              return res.result;
            }
          case "none":
            {
              const res = create(unsafeRes, GetNoneModeBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              return res.result;
            }
          default:
            {
              const res = create(unsafeRes, GetBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              const {
                result: result
              } = res;
              return result ? $(function () {
                let result = $Object.create(null, undefined);
                $Object.assign(result, result);
                result.transactions = result.transactions.map($(({
                  transaction: transaction,
                  meta: meta,
                  version: version
                }) => $(function () {
                  let result = $Object.create(null, undefined);
                  result.meta = meta;
                  result.transaction = $(function () {
                    let result = $Object.create(null, undefined);
                    $Object.assign(result, transaction);
                    result.message = versionedMessageFromResponse(version, transaction.message);
                    return result;
                  })();
                  result.version = version;
                  return result;
                })()));
                return result;
              })() : null;
            }
        }
      } catch (e) {
        throw new SolanaJSONRPCError(e, "failed to get confirmed block");
      }
    }
    async getParsedBlock(slot, rawConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(rawConfig);
      const args = this._buildArgsAtLeastConfirmed($Array.of(slot), commitment, "jsonParsed", config);
      const unsafeRes = await this._rpcRequest("getBlock", args);
      try {
        switch (config?.transactionDetails) {
          case "accounts":
            {
              const res = create(unsafeRes, GetParsedAccountsModeBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              return res.result;
            }
          case "none":
            {
              const res = create(unsafeRes, GetParsedNoneModeBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              return res.result;
            }
          default:
            {
              const res = create(unsafeRes, GetParsedBlockRpcResult);
              if ("error" in res) {
                throw res.error;
              }
              return res.result;
            }
        }
      } catch (e) {
        throw new SolanaJSONRPCError(e, "failed to get block");
      }
    }
    async getBlockProduction(configOrCommitment) {
      let extra;
      let commitment;
      if (typeof configOrCommitment === "string") {
        commitment = configOrCommitment;
      } else if (configOrCommitment) {
        const {
          commitment: c,
          ...rest
        } = configOrCommitment;
        commitment = c;
        extra = rest;
      }
      const args = this._buildArgs($Array.of(), commitment, "base64", extra);
      const unsafeRes = await this._rpcRequest("getBlockProduction", args);
      const res = create(unsafeRes, BlockProductionResponseStruct);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get block production information");
      }
      return res.result;
    }
    async getTransaction(signature, rawConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(rawConfig);
      const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, undefined, config);
      const unsafeRes = await this._rpcRequest("getTransaction", args);
      const res = create(unsafeRes, GetTransactionRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get transaction");
      }
      const result = res.result;
      if (!result) return result;
      return $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, result);
        result.transaction = $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, result.transaction);
          result.message = versionedMessageFromResponse(result.version, result.transaction.message);
          return result;
        })();
        return result;
      })();
    }
    async getParsedTransaction(signature, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, "jsonParsed", config);
      const unsafeRes = await this._rpcRequest("getTransaction", args);
      const res = create(unsafeRes, GetParsedTransactionRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get transaction");
      }
      return res.result;
    }
    async getParsedTransactions(signatures, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const batch = signatures.map($(signature => {
        const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, "jsonParsed", config);
        return $(function () {
          let result = $Object.create(null, undefined);
          result.methodName = "getTransaction";
          result.args = args;
          return result;
        })();
      }));
      const unsafeRes = await this._rpcBatchRequest(batch);
      const res = unsafeRes.map($(unsafeRes => {
        const res = create(unsafeRes, GetParsedTransactionRpcResult);
        if ("error" in res) {
          throw new SolanaJSONRPCError(res.error, "failed to get transactions");
        }
        return res.result;
      }));
      return res;
    }
    async getTransactions(signatures, commitmentOrConfig) {
      const {
        commitment: commitment,
        config: config
      } = extractCommitmentFromConfig(commitmentOrConfig);
      const batch = signatures.map($(signature => {
        const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, undefined, config);
        return $(function () {
          let result = $Object.create(null, undefined);
          result.methodName = "getTransaction";
          result.args = args;
          return result;
        })();
      }));
      const unsafeRes = await this._rpcBatchRequest(batch);
      const res = unsafeRes.map($(unsafeRes => {
        const res = create(unsafeRes, GetTransactionRpcResult);
        if ("error" in res) {
          throw new SolanaJSONRPCError(res.error, "failed to get transactions");
        }
        const result = res.result;
        if (!result) return result;
        return $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, result);
          result.transaction = $(function () {
            let result = $Object.create(null, undefined);
            $Object.assign(result, result.transaction);
            result.message = versionedMessageFromResponse(result.version, result.transaction.message);
            return result;
          })();
          return result;
        })();
      }));
      return res;
    }
    async getConfirmedBlock(slot, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(slot), commitment);
      const unsafeRes = await this._rpcRequest("getConfirmedBlock", args);
      const res = create(unsafeRes, GetConfirmedBlockRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get confirmed block");
      }
      const result = res.result;
      if (!result) {
        throw new Error("Confirmed block " + slot + " not found");
      }
      const block = $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, result);
        result.transactions = result.transactions.map($(({
          transaction: transaction,
          meta: meta
        }) => {
          const message = new Message(transaction.message);
          return $(function () {
            let result = $Object.create(null, undefined);
            result.meta = meta;
            result.transaction = $(function () {
              let result = $Object.create(null, undefined);
              $Object.assign(result, transaction);
              result.message = message;
              return result;
            })();
            return result;
          })();
        }));
        return result;
      })();
      return $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, block);
        result.transactions = block.transactions.map($(({
          transaction: transaction,
          meta: meta
        }) => $(function () {
          let result = $Object.create(null, undefined);
          result.meta = meta;
          result.transaction = Transaction.populate(transaction.message, transaction.signatures);
          return result;
        })()));
        return result;
      })();
    }
    async getBlocks(startSlot, endSlot, commitment) {
      const args = this._buildArgsAtLeastConfirmed(endSlot !== undefined ? $Array.of(startSlot, endSlot) : $Array.of(startSlot), commitment);
      const unsafeRes = await this._rpcRequest("getBlocks", args);
      const res = create(unsafeRes, jsonRpcResult(array(number())));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get blocks");
      }
      return res.result;
    }
    async getBlockSignatures(slot, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(slot), commitment, undefined, $(function () {
        let result = $Object.create(null, undefined);
        result.transactionDetails = "signatures";
        result.rewards = false;
        return result;
      })());
      const unsafeRes = await this._rpcRequest("getBlock", args);
      const res = create(unsafeRes, GetBlockSignaturesRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get block");
      }
      const result = res.result;
      if (!result) {
        throw new Error("Block " + slot + " not found");
      }
      return result;
    }
    async getConfirmedBlockSignatures(slot, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(slot), commitment, undefined, $(function () {
        let result = $Object.create(null, undefined);
        result.transactionDetails = "signatures";
        result.rewards = false;
        return result;
      })());
      const unsafeRes = await this._rpcRequest("getConfirmedBlock", args);
      const res = create(unsafeRes, GetBlockSignaturesRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get confirmed block");
      }
      const result = res.result;
      if (!result) {
        throw new Error("Confirmed block " + slot + " not found");
      }
      return result;
    }
    async getConfirmedTransaction(signature, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment);
      const unsafeRes = await this._rpcRequest("getConfirmedTransaction", args);
      const res = create(unsafeRes, GetTransactionRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get transaction");
      }
      const result = res.result;
      if (!result) return result;
      const message = new Message(result.transaction.message);
      const signatures = result.transaction.signatures;
      return $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, result);
        result.transaction = Transaction.populate(message, signatures);
        return result;
      })();
    }
    async getParsedConfirmedTransaction(signature, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, "jsonParsed");
      const unsafeRes = await this._rpcRequest("getConfirmedTransaction", args);
      const res = create(unsafeRes, GetParsedTransactionRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get confirmed transaction");
      }
      return res.result;
    }
    async getParsedConfirmedTransactions(signatures, commitment) {
      const batch = signatures.map($(signature => {
        const args = this._buildArgsAtLeastConfirmed($Array.of(signature), commitment, "jsonParsed");
        return $(function () {
          let result = $Object.create(null, undefined);
          result.methodName = "getConfirmedTransaction";
          result.args = args;
          return result;
        })();
      }));
      const unsafeRes = await this._rpcBatchRequest(batch);
      const res = unsafeRes.map($(unsafeRes => {
        const res = create(unsafeRes, GetParsedTransactionRpcResult);
        if ("error" in res) {
          throw new SolanaJSONRPCError(res.error, "failed to get confirmed transactions");
        }
        return res.result;
      }));
      return res;
    }
    async getConfirmedSignaturesForAddress(address, startSlot, endSlot) {
      let options = $Object.create(null, undefined);
      let firstAvailableBlock = await this.getFirstAvailableBlock();
      while (!("until" in options)) {
        startSlot--;
        if (startSlot <= 0 || startSlot < firstAvailableBlock) {
          break;
        }
        try {
          const block = await this.getConfirmedBlockSignatures(startSlot, "finalized");
          if (block.signatures.length > 0) {
            options.until = block.signatures[block.signatures.length - 1].toString();
          }
        } catch (err) {
          if (err instanceof Error && err.message.includes("skipped")) {
            continue;
          } else {
            throw err;
          }
        }
      }
      let highestConfirmedRoot = await this.getSlot("finalized");
      while (!("before" in options)) {
        endSlot++;
        if (endSlot > highestConfirmedRoot) {
          break;
        }
        try {
          const block = await this.getConfirmedBlockSignatures(endSlot);
          if (block.signatures.length > 0) {
            options.before = block.signatures[block.signatures.length - 1].toString();
          }
        } catch (err) {
          if (err instanceof Error && err.message.includes("skipped")) {
            continue;
          } else {
            throw err;
          }
        }
      }
      const confirmedSignatureInfo = await this.getConfirmedSignaturesForAddress2(address, options);
      return confirmedSignatureInfo.map($(info => info.signature));
    }
    async getConfirmedSignaturesForAddress2(address, options, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(address.toBase58()), commitment, undefined, options);
      const unsafeRes = await this._rpcRequest("getConfirmedSignaturesForAddress2", args);
      const res = create(unsafeRes, GetConfirmedSignaturesForAddress2RpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get confirmed signatures for address");
      }
      return res.result;
    }
    async getSignaturesForAddress(address, options, commitment) {
      const args = this._buildArgsAtLeastConfirmed($Array.of(address.toBase58()), commitment, undefined, options);
      const unsafeRes = await this._rpcRequest("getSignaturesForAddress", args);
      const res = create(unsafeRes, GetSignaturesForAddressRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, "failed to get signatures for address");
      }
      return res.result;
    }
    async getAddressLookupTable(accountKey, config) {
      const {
        context: context,
        value: accountInfo
      } = await this.getAccountInfoAndContext(accountKey, config);
      let value = null;
      if (accountInfo !== null) {
        value = new AddressLookupTableAccount($(function () {
          let result = $Object.create(null, undefined);
          result.key = accountKey;
          result.state = AddressLookupTableAccount.deserialize(accountInfo.data);
          return result;
        })());
      }
      return $(function () {
        let result = $Object.create(null, undefined);
        result.context = context;
        result.value = value;
        return result;
      })();
    }
    async getNonceAndContext(nonceAccount, commitmentOrConfig) {
      const {
        context: context,
        value: accountInfo
      } = await this.getAccountInfoAndContext(nonceAccount, commitmentOrConfig);
      let value = null;
      if (accountInfo !== null) {
        value = NonceAccount.fromAccountData(accountInfo.data);
      }
      return $(function () {
        let result = $Object.create(null, undefined);
        result.context = context;
        result.value = value;
        return result;
      })();
    }
    async getNonce(nonceAccount, commitmentOrConfig) {
      return await this.getNonceAndContext(nonceAccount, commitmentOrConfig).then($(x => x.value)).catch($(e => {
        throw new Error("failed to get nonce for account " + nonceAccount.toBase58() + ": " + e);
      }));
    }
    async requestAirdrop(to, lamports) {
      const unsafeRes = await this._rpcRequest("requestAirdrop", $Array.of(to.toBase58(), lamports));
      const res = create(unsafeRes, RequestAirdropRpcResult);
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `airdrop to ${to.toBase58()} failed`);
      }
      return res.result;
    }
    async _blockhashWithExpiryBlockHeight(disableCache) {
      if (!disableCache) {
        while (this._pollingBlockhash) {
          await sleep(100);
        }
        const timeSinceFetch = Date.now() - this._blockhashInfo.lastFetch;
        const expired = timeSinceFetch >= BLOCKHASH_CACHE_TIMEOUT_MS;
        if (this._blockhashInfo.latestBlockhash !== null && !expired) {
          return this._blockhashInfo.latestBlockhash;
        }
      }
      return await this._pollNewBlockhash();
    }
    async _pollNewBlockhash() {
      this._pollingBlockhash = true;
      try {
        const startTime = Date.now();
        const cachedLatestBlockhash = this._blockhashInfo.latestBlockhash;
        const cachedBlockhash = cachedLatestBlockhash ? cachedLatestBlockhash.blockhash : null;
        for (let i = 0; i < 50; i++) {
          const latestBlockhash = await this.getLatestBlockhash("finalized");
          if (cachedBlockhash !== latestBlockhash.blockhash) {
            this._blockhashInfo = $(function () {
              let result = $Object.create(null, undefined);
              result.latestBlockhash = latestBlockhash;
              result.lastFetch = Date.now();
              result.transactionSignatures = $Array.of();
              result.simulatedSignatures = $Array.of();
              return result;
            })();
            return latestBlockhash;
          }
          await sleep(MS_PER_SLOT / 2);
        }
        throw new Error(`Unable to obtain a new blockhash after ${Date.now() - startTime}ms`);
      } finally {
        this._pollingBlockhash = false;
      }
    }
    async getStakeMinimumDelegation(config) {
      const {
        commitment: commitment,
        config: configArg
      } = extractCommitmentFromConfig(config);
      const args = this._buildArgs($Array.of(), commitment, "base64", configArg);
      const unsafeRes = await this._rpcRequest("getStakeMinimumDelegation", args);
      const res = create(unsafeRes, jsonRpcResultAndContext(number()));
      if ("error" in res) {
        throw new SolanaJSONRPCError(res.error, `failed to get stake minimum delegation`);
      }
      return res.result;
    }
    async simulateTransaction(transactionOrMessage, configOrSigners, includeAccounts) {
      if ("message" in transactionOrMessage) {
        const versionedTx = transactionOrMessage;
        const wireTransaction = versionedTx.serialize();
        const encodedTransaction = buffer.Buffer.from(wireTransaction).toString("base64");
        if ($Array.isArray(configOrSigners) || includeAccounts !== undefined) {
          throw new Error("Invalid arguments");
        }
        const config = configOrSigners || $Object.create(null, undefined);
        config.encoding = "base64";
        if (!("commitment" in config)) {
          config.commitment = this.commitment;
        }
        const args = $Array.of(encodedTransaction, config);
        const unsafeRes = await this._rpcRequest("simulateTransaction", args);
        const res = create(unsafeRes, SimulatedTransactionResponseStruct);
        if ("error" in res) {
          throw new Error("failed to simulate transaction: " + res.error.message);
        }
        return res.result;
      }
      let transaction;
      if (transactionOrMessage instanceof Transaction) {
        let originalTx = transactionOrMessage;
        transaction = new Transaction();
        transaction.feePayer = originalTx.feePayer;
        transaction.instructions = transactionOrMessage.instructions;
        transaction.nonceInfo = originalTx.nonceInfo;
        transaction.signatures = originalTx.signatures;
      } else {
        transaction = Transaction.populate(transactionOrMessage);
        transaction._message = transaction._json = undefined;
      }
      if (configOrSigners !== undefined && !$Array.isArray(configOrSigners)) {
        throw new Error("Invalid arguments");
      }
      const signers = configOrSigners;
      if (transaction.nonceInfo && signers) {
        transaction.sign(...signers);
      } else {
        let disableCache = this._disableBlockhashCaching;
        for (;;) {
          const latestBlockhash = await this._blockhashWithExpiryBlockHeight(disableCache);
          transaction.lastValidBlockHeight = latestBlockhash.lastValidBlockHeight;
          transaction.recentBlockhash = latestBlockhash.blockhash;
          if (!signers) break;
          transaction.sign(...signers);
          if (!transaction.signature) {
            throw new Error("!signature");
          }
          const signature = transaction.signature.toString("base64");
          if (!this._blockhashInfo.simulatedSignatures.includes(signature) && !this._blockhashInfo.transactionSignatures.includes(signature)) {
            this._blockhashInfo.simulatedSignatures.push(signature);
            break;
          } else {
            disableCache = true;
          }
        }
      }
      const message = transaction._compile();
      const signData = message.serialize();
      const wireTransaction = transaction._serialize(signData);
      const encodedTransaction = wireTransaction.toString("base64");
      const config = $(function () {
        let result = $Object.create(null, undefined);
        result.encoding = "base64";
        result.commitment = this.commitment;
        return result;
      }).bind(this)();
      if (includeAccounts) {
        const addresses = ($Array.isArray(includeAccounts) ? includeAccounts : message.nonProgramIds()).map($(key => key.toBase58()));
        config["accounts"] = $(function () {
          let result = $Object.create(null, undefined);
          result.encoding = "base64";
          result.addresses = addresses;
          return result;
        })();
      }
      if (signers) {
        config.sigVerify = true;
      }
      const args = $Array.of(encodedTransaction, config);
      const unsafeRes = await this._rpcRequest("simulateTransaction", args);
      const res = create(unsafeRes, SimulatedTransactionResponseStruct);
      if ("error" in res) {
        let logs;
        if ("data" in res.error) {
          logs = res.error.data.logs;
          if (logs && $Array.isArray(logs)) {
            const traceIndent = "\n    ";
            const logTrace = traceIndent + logs.join(traceIndent);
            console.error(res.error.message, logTrace);
          }
        }
        throw new SendTransactionError("failed to simulate transaction: " + res.error.message, logs);
      }
      return res.result;
    }
    async sendTransaction(transaction, signersOrOptions, options) {
      if ("version" in transaction) {
        if (signersOrOptions && $Array.isArray(signersOrOptions)) {
          throw new Error("Invalid arguments");
        }
        const wireTransaction = transaction.serialize();
        return await this.sendRawTransaction(wireTransaction, signersOrOptions);
      }
      if (signersOrOptions === undefined || !$Array.isArray(signersOrOptions)) {
        throw new Error("Invalid arguments");
      }
      const signers = signersOrOptions;
      if (transaction.nonceInfo) {
        transaction.sign(...signers);
      } else {
        let disableCache = this._disableBlockhashCaching;
        for (;;) {
          const latestBlockhash = await this._blockhashWithExpiryBlockHeight(disableCache);
          transaction.lastValidBlockHeight = latestBlockhash.lastValidBlockHeight;
          transaction.recentBlockhash = latestBlockhash.blockhash;
          transaction.sign(...signers);
          if (!transaction.signature) {
            throw new Error("!signature");
          }
          const signature = transaction.signature.toString("base64");
          if (!this._blockhashInfo.transactionSignatures.includes(signature)) {
            this._blockhashInfo.transactionSignatures.push(signature);
            break;
          } else {
            disableCache = true;
          }
        }
      }
      const wireTransaction = transaction.serialize();
      return await this.sendRawTransaction(wireTransaction, options);
    }
    async sendRawTransaction(rawTransaction, options) {
      const encodedTransaction = toBuffer(rawTransaction).toString("base64");
      const result = await this.sendEncodedTransaction(encodedTransaction, options);
      return result;
    }
    async sendEncodedTransaction(encodedTransaction, options) {
      const config = $(function () {
        let result = $Object.create(null, undefined);
        result.encoding = "base64";
        return result;
      })();
      const skipPreflight = options && options.skipPreflight;
      const preflightCommitment = skipPreflight === true ? "processed" : options && options.preflightCommitment || this.commitment;
      if (options && options.maxRetries != null) {
        config.maxRetries = options.maxRetries;
      }
      if (options && options.minContextSlot != null) {
        config.minContextSlot = options.minContextSlot;
      }
      if (skipPreflight) {
        config.skipPreflight = skipPreflight;
      }
      if (preflightCommitment) {
        config.preflightCommitment = preflightCommitment;
      }
      const args = $Array.of(encodedTransaction, config);
      const unsafeRes = await this._rpcRequest("sendTransaction", args);
      const res = create(unsafeRes, SendTransactionRpcResult);
      if ("error" in res) {
        let logs;
        if ("data" in res.error) {
          logs = res.error.data.logs;
        }
        throw new SendTransactionError("failed to send transaction: " + res.error.message, logs);
      }
      return res.result;
    }
    _wsOnOpen() {
      this._rpcWebSocketConnected = true;
      this._rpcWebSocketHeartbeat = setInterval($(() => {
        $(async () => {
          try {
            await this._rpcWebSocket.notify("ping");
          } catch {}
        })();
      }), 5e3);
      this._updateSubscriptions();
    }
    _wsOnError(err) {
      this._rpcWebSocketConnected = false;
      console.error("ws error:", err.message);
    }
    _wsOnClose(code) {
      this._rpcWebSocketConnected = false;
      this._rpcWebSocketGeneration = (this._rpcWebSocketGeneration + 1) % Number.MAX_SAFE_INTEGER;
      if (this._rpcWebSocketIdleTimeout) {
        clearTimeout(this._rpcWebSocketIdleTimeout);
        this._rpcWebSocketIdleTimeout = null;
      }
      if (this._rpcWebSocketHeartbeat) {
        clearInterval(this._rpcWebSocketHeartbeat);
        this._rpcWebSocketHeartbeat = null;
      }
      if (code === 1e3) {
        this._updateSubscriptions();
        return;
      }
      this._subscriptionCallbacksByServerSubscriptionId = $Object.create(null, undefined);
      $Object.entries(this._subscriptionsByHash).forEach($(([hash, subscription]) => {
        this._setSubscription(hash, $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, subscription);
          result.state = "pending";
          return result;
        })());
      }));
    }
    _setSubscription(hash, nextSubscription) {
      const prevState = this._subscriptionsByHash[hash]?.state;
      this._subscriptionsByHash[hash] = nextSubscription;
      if (prevState !== nextSubscription.state) {
        const stateChangeCallbacks = this._subscriptionStateChangeCallbacksByHash[hash];
        if (stateChangeCallbacks) {
          stateChangeCallbacks.forEach($(cb => {
            try {
              cb(nextSubscription.state);
            } catch {}
          }));
        }
      }
    }
    _onSubscriptionStateChange(clientSubscriptionId, callback) {
      const hash = this._subscriptionHashByClientSubscriptionId[clientSubscriptionId];
      if (hash == null) {
        return $(() => {});
      }
      const stateChangeCallbacks = this._subscriptionStateChangeCallbacksByHash[hash] ||= new Set();
      stateChangeCallbacks.add(callback);
      return $(() => {
        stateChangeCallbacks.delete(callback);
        if (stateChangeCallbacks.size === 0) {
          delete this._subscriptionStateChangeCallbacksByHash[hash];
        }
      });
    }
    async _updateSubscriptions() {
      if ($Object.keys(this._subscriptionsByHash).length === 0) {
        if (this._rpcWebSocketConnected) {
          this._rpcWebSocketConnected = false;
          this._rpcWebSocketIdleTimeout = setTimeout($(() => {
            this._rpcWebSocketIdleTimeout = null;
            try {
              this._rpcWebSocket.close();
            } catch (err) {
              if (err instanceof Error) {
                console.log(`Error when closing socket connection: ${err.message}`);
              }
            }
          }), 500);
        }
        return;
      }
      if (this._rpcWebSocketIdleTimeout !== null) {
        clearTimeout(this._rpcWebSocketIdleTimeout);
        this._rpcWebSocketIdleTimeout = null;
        this._rpcWebSocketConnected = true;
      }
      if (!this._rpcWebSocketConnected) {
        this._rpcWebSocket.connect();
        return;
      }
      const activeWebSocketGeneration = this._rpcWebSocketGeneration;
      const isCurrentConnectionStillActive = () => activeWebSocketGeneration === this._rpcWebSocketGeneration;
      $(isCurrentConnectionStillActive);
      await Promise.all($Object.keys(this._subscriptionsByHash).map($(async hash => {
        const subscription = this._subscriptionsByHash[hash];
        if (subscription === undefined) {
          return;
        }
        switch (subscription.state) {
          case "pending":
          case "unsubscribed":
            if (subscription.callbacks.size === 0) {
              delete this._subscriptionsByHash[hash];
              if (subscription.state === "unsubscribed") {
                delete this._subscriptionCallbacksByServerSubscriptionId[subscription.serverSubscriptionId];
              }
              await this._updateSubscriptions();
              return;
            }
            await $(async () => {
              const {
                args: args,
                method: method
              } = subscription;
              try {
                this._setSubscription(hash, $(function () {
                  let result = $Object.create(null, undefined);
                  $Object.assign(result, subscription);
                  result.state = "subscribing";
                  return result;
                })());
                const serverSubscriptionId = await this._rpcWebSocket.call(method, args);
                this._setSubscription(hash, $(function () {
                  let result = $Object.create(null, undefined);
                  $Object.assign(result, subscription);
                  result.serverSubscriptionId = serverSubscriptionId;
                  result.state = "subscribed";
                  return result;
                })());
                this._subscriptionCallbacksByServerSubscriptionId[serverSubscriptionId] = subscription.callbacks;
                await this._updateSubscriptions();
              } catch (e) {
                if (e instanceof Error) {
                  console.error(`${method} error for argument`, args, e.message);
                }
                if (!isCurrentConnectionStillActive()) {
                  return;
                }
                this._setSubscription(hash, $(function () {
                  let result = $Object.create(null, undefined);
                  $Object.assign(result, subscription);
                  result.state = "pending";
                  return result;
                })());
                await this._updateSubscriptions();
              }
            })();
            break;
          case "subscribed":
            if (subscription.callbacks.size === 0) {
              await $(async () => {
                const {
                  serverSubscriptionId: serverSubscriptionId,
                  unsubscribeMethod: unsubscribeMethod
                } = subscription;
                if (this._subscriptionsAutoDisposedByRpc.has(serverSubscriptionId)) {
                  this._subscriptionsAutoDisposedByRpc.delete(serverSubscriptionId);
                } else {
                  this._setSubscription(hash, $(function () {
                    let result = $Object.create(null, undefined);
                    $Object.assign(result, subscription);
                    result.state = "unsubscribing";
                    return result;
                  })());
                  this._setSubscription(hash, $(function () {
                    let result = $Object.create(null, undefined);
                    $Object.assign(result, subscription);
                    result.state = "unsubscribing";
                    return result;
                  })());
                  try {
                    await this._rpcWebSocket.call(unsubscribeMethod, $Array.of(serverSubscriptionId));
                  } catch (e) {
                    if (e instanceof Error) {
                      console.error(`${unsubscribeMethod} error:`, e.message);
                    }
                    if (!isCurrentConnectionStillActive()) {
                      return;
                    }
                    this._setSubscription(hash, $(function () {
                      let result = $Object.create(null, undefined);
                      $Object.assign(result, subscription);
                      result.state = "subscribed";
                      return result;
                    })());
                    await this._updateSubscriptions();
                    return;
                  }
                }
                this._setSubscription(hash, $(function () {
                  let result = $Object.create(null, undefined);
                  $Object.assign(result, subscription);
                  result.state = "unsubscribed";
                  return result;
                })());
                await this._updateSubscriptions();
              })();
            }
            break;
        }
      })));
    }
    _handleServerNotification(serverSubscriptionId, callbackArgs) {
      const callbacks = this._subscriptionCallbacksByServerSubscriptionId[serverSubscriptionId];
      if (callbacks === undefined) {
        return;
      }
      callbacks.forEach($(cb => {
        try {
          cb(...callbackArgs);
        } catch (e) {
          console.error(e);
        }
      }));
    }
    _wsOnAccountNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, AccountNotificationResult);
      this._handleServerNotification(subscription, $Array.of(result.value, result.context));
    }
    _makeSubscription(subscriptionConfig, args) {
      const clientSubscriptionId = this._nextClientSubscriptionId++;
      const hash = fastStableStringify$1($Array.of(subscriptionConfig.method, args));
      const existingSubscription = this._subscriptionsByHash[hash];
      if (existingSubscription === undefined) {
        this._subscriptionsByHash[hash] = $(function () {
          let result = $Object.create(null, undefined);
          $Object.assign(result, subscriptionConfig);
          result.args = args;
          result.callbacks = new Set($Array.of(subscriptionConfig.callback));
          result.state = "pending";
          return result;
        })();
      } else {
        existingSubscription.callbacks.add(subscriptionConfig.callback);
      }
      this._subscriptionHashByClientSubscriptionId[clientSubscriptionId] = hash;
      this._subscriptionDisposeFunctionsByClientSubscriptionId[clientSubscriptionId] = $(async () => {
        delete this._subscriptionDisposeFunctionsByClientSubscriptionId[clientSubscriptionId];
        delete this._subscriptionHashByClientSubscriptionId[clientSubscriptionId];
        const subscription = this._subscriptionsByHash[hash];
        assert$1(subscription !== undefined, `Could not find a \`Subscription\` when tearing down client subscription #${clientSubscriptionId}`);
        subscription.callbacks.delete(subscriptionConfig.callback);
        await this._updateSubscriptions();
      });
      this._updateSubscriptions();
      return clientSubscriptionId;
    }
    onAccountChange(publicKey, callback, commitment) {
      const args = this._buildArgs($Array.of(publicKey.toBase58()), commitment || this._commitment || "finalized", "base64");
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "accountSubscribe";
        result.unsubscribeMethod = "accountUnsubscribe";
        return result;
      })(), args);
    }
    async removeAccountChangeListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "account change");
    }
    _wsOnProgramAccountNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, ProgramAccountNotificationResult);
      this._handleServerNotification(subscription, $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.accountId = result.value.pubkey;
        result.accountInfo = result.value.account;
        return result;
      })(), result.context));
    }
    onProgramAccountChange(programId, callback, commitment, filters) {
      const args = this._buildArgs($Array.of(programId.toBase58()), commitment || this._commitment || "finalized", "base64", filters ? $(function () {
        let result = $Object.create(null, undefined);
        result.filters = filters;
        return result;
      })() : undefined);
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "programSubscribe";
        result.unsubscribeMethod = "programUnsubscribe";
        return result;
      })(), args);
    }
    async removeProgramAccountChangeListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "program account change");
    }
    onLogs(filter, callback, commitment) {
      const args = this._buildArgs($Array.of(typeof filter === "object" ? $(function () {
        let result = $Object.create(null, undefined);
        result.mentions = $Array.of(filter.toString());
        return result;
      })() : filter), commitment || this._commitment || "finalized");
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "logsSubscribe";
        result.unsubscribeMethod = "logsUnsubscribe";
        return result;
      })(), args);
    }
    async removeOnLogsListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "logs");
    }
    _wsOnLogsNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, LogsNotificationResult);
      this._handleServerNotification(subscription, $Array.of(result.value, result.context));
    }
    _wsOnSlotNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, SlotNotificationResult);
      this._handleServerNotification(subscription, $Array.of(result));
    }
    onSlotChange(callback) {
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "slotSubscribe";
        result.unsubscribeMethod = "slotUnsubscribe";
        return result;
      })(), $Array.of());
    }
    async removeSlotChangeListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "slot change");
    }
    _wsOnSlotUpdatesNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, SlotUpdateNotificationResult);
      this._handleServerNotification(subscription, $Array.of(result));
    }
    onSlotUpdate(callback) {
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "slotsUpdatesSubscribe";
        result.unsubscribeMethod = "slotsUpdatesUnsubscribe";
        return result;
      })(), $Array.of());
    }
    async removeSlotUpdateListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "slot update");
    }
    async _unsubscribeClientSubscription(clientSubscriptionId, subscriptionName) {
      const dispose = this._subscriptionDisposeFunctionsByClientSubscriptionId[clientSubscriptionId];
      if (dispose) {
        await dispose();
      } else {
        console.warn("Ignored unsubscribe request because an active subscription with id " + `\`${clientSubscriptionId}\` for '${subscriptionName}' events ` + "could not be found.");
      }
    }
    _buildArgs(args, override, encoding, extra) {
      const commitment = override || this._commitment;
      if (commitment || encoding || extra) {
        let options = $Object.create(null, undefined);
        if (encoding) {
          options.encoding = encoding;
        }
        if (commitment) {
          options.commitment = commitment;
        }
        if (extra) {
          options = $Object.assign(options, extra);
        }
        args.push(options);
      }
      return args;
    }
    _buildArgsAtLeastConfirmed(args, override, encoding, extra) {
      const commitment = override || this._commitment;
      if (commitment && !$Array.of("confirmed", "finalized").includes(commitment)) {
        throw new Error("Using Connection with default commitment: `" + this._commitment + "`, but method requires at least `confirmed`");
      }
      return this._buildArgs(args, override, encoding, extra);
    }
    _wsOnSignatureNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, SignatureNotificationResult);
      if (result.value !== "receivedSignature") {
        this._subscriptionsAutoDisposedByRpc.add(subscription);
      }
      this._handleServerNotification(subscription, result.value === "receivedSignature" ? $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.type = "received";
        return result;
      })(), result.context) : $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.type = "status";
        result.result = result.value;
        return result;
      })(), result.context));
    }
    onSignature(signature, callback, commitment) {
      const args = this._buildArgs($Array.of(signature), commitment || this._commitment || "finalized");
      const clientSubscriptionId = this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = $((notification, context) => {
          if (notification.type === "status") {
            callback(notification.result, context);
            try {
              this.removeSignatureListener(clientSubscriptionId);
            } catch (_err) {}
          }
        });
        result.method = "signatureSubscribe";
        result.unsubscribeMethod = "signatureUnsubscribe";
        return result;
      }).bind(this)(), args);
      return clientSubscriptionId;
    }
    onSignatureWithOptions(signature, callback, options) {
      const {
        commitment: commitment,
        ...extra
      } = $(function () {
        let result = $Object.create(null, undefined);
        $Object.assign(result, options);
        result.commitment = options && options.commitment || this._commitment || "finalized";
        return result;
      }).bind(this)();
      const args = this._buildArgs($Array.of(signature), commitment, undefined, extra);
      const clientSubscriptionId = this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = $((notification, context) => {
          callback(notification, context);
          try {
            this.removeSignatureListener(clientSubscriptionId);
          } catch (_err) {}
        });
        result.method = "signatureSubscribe";
        result.unsubscribeMethod = "signatureUnsubscribe";
        return result;
      }).bind(this)(), args);
      return clientSubscriptionId;
    }
    async removeSignatureListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "signature result");
    }
    _wsOnRootNotification(notification) {
      const {
        result: result,
        subscription: subscription
      } = create(notification, RootNotificationResult);
      this._handleServerNotification(subscription, $Array.of(result));
    }
    onRootChange(callback) {
      return this._makeSubscription($(function () {
        let result = $Object.create(null, undefined);
        result.callback = callback;
        result.method = "rootSubscribe";
        result.unsubscribeMethod = "rootUnsubscribe";
        return result;
      })(), $Array.of());
    }
    async removeRootChangeListener(clientSubscriptionId) {
      await this._unsubscribeClientSubscription(clientSubscriptionId, "root change");
    }
  }
  class Keypair {
    constructor(keypair) {
      this._keypair = void 0;
      this._keypair = keypair ?? generateKeypair();
    }
    static generate() {
      return new Keypair(generateKeypair());
    }
    static fromSecretKey(secretKey, options) {
      if (secretKey.byteLength !== 64) {
        throw new Error("bad secret key size");
      }
      const publicKey = secretKey.slice(32, 64);
      if (!options || !options.skipValidation) {
        const privateScalar = secretKey.slice(0, 32);
        const computedPublicKey = getPublicKey(privateScalar);
        for (let ii = 0; ii < 32; ii++) {
          if (publicKey[ii] !== computedPublicKey[ii]) {
            throw new Error("provided secretKey is invalid");
          }
        }
      }
      return new Keypair($(function () {
        let result = $Object.create(null, undefined);
        result.publicKey = publicKey;
        result.secretKey = secretKey;
        return result;
      })());
    }
    static fromSeed(seed) {
      const publicKey = getPublicKey(seed);
      const secretKey = new Uint8Array(64);
      secretKey.set(seed);
      secretKey.set(publicKey, 32);
      return new Keypair($(function () {
        let result = $Object.create(null, undefined);
        result.publicKey = publicKey;
        result.secretKey = secretKey;
        return result;
      })());
    }
    get publicKey() {
      return new PublicKey(this._keypair.publicKey);
    }
    get secretKey() {
      return new Uint8Array(this._keypair.secretKey);
    }
  }
  const LOOKUP_TABLE_INSTRUCTION_LAYOUTS = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.CreateLookupTable = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      result.layout = struct($Array.of(u32("instruction"), u64("recentSlot"), u8("bumpSeed")));
      return result;
    })();
    result.FreezeLookupTable = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.ExtendLookupTable = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 2;
      result.layout = struct($Array.of(u32("instruction"), u64(), seq(publicKey(), offset(u32(), -8), "addresses")));
      return result;
    })();
    result.DeactivateLookupTable = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 3;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.CloseLookupTable = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 4;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    return result;
  })());
  class AddressLookupTableInstruction {
    constructor() {}
    static decodeInstructionType(instruction) {
      this.checkProgramId(instruction.programId);
      const instructionTypeLayout = u32("instruction");
      const index = instructionTypeLayout.decode(instruction.data);
      let type;
      for (const [layoutType, layout] of $Object.entries(LOOKUP_TABLE_INSTRUCTION_LAYOUTS)) {
        if (layout.index == index) {
          type = layoutType;
          break;
        }
      }
      if (!type) {
        throw new Error("Invalid Instruction. Should be a LookupTable Instruction");
      }
      return type;
    }
    static decodeCreateLookupTable(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeysLength(instruction.keys, 4);
      const {
        recentSlot: recentSlot
      } = decodeData$1(LOOKUP_TABLE_INSTRUCTION_LAYOUTS.CreateLookupTable, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.authority = instruction.keys[1].pubkey;
        result.payer = instruction.keys[2].pubkey;
        result.recentSlot = Number(recentSlot);
        return result;
      })();
    }
    static decodeExtendLookupTable(instruction) {
      this.checkProgramId(instruction.programId);
      if (instruction.keys.length < 2) {
        throw new Error(`invalid instruction; found ${instruction.keys.length} keys, expected at least 2`);
      }
      const {
        addresses: addresses
      } = decodeData$1(LOOKUP_TABLE_INSTRUCTION_LAYOUTS.ExtendLookupTable, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.lookupTable = instruction.keys[0].pubkey;
        result.authority = instruction.keys[1].pubkey;
        result.payer = instruction.keys.length > 2 ? instruction.keys[2].pubkey : undefined;
        result.addresses = addresses.map($(buffer => new PublicKey(buffer)));
        return result;
      })();
    }
    static decodeCloseLookupTable(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeysLength(instruction.keys, 3);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.lookupTable = instruction.keys[0].pubkey;
        result.authority = instruction.keys[1].pubkey;
        result.recipient = instruction.keys[2].pubkey;
        return result;
      })();
    }
    static decodeFreezeLookupTable(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeysLength(instruction.keys, 2);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.lookupTable = instruction.keys[0].pubkey;
        result.authority = instruction.keys[1].pubkey;
        return result;
      })();
    }
    static decodeDeactivateLookupTable(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeysLength(instruction.keys, 2);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.lookupTable = instruction.keys[0].pubkey;
        result.authority = instruction.keys[1].pubkey;
        return result;
      })();
    }
    static checkProgramId(programId) {
      if (!programId.equals(AddressLookupTableProgram.programId)) {
        throw new Error("invalid instruction; programId is not AddressLookupTable Program");
      }
    }
    static checkKeysLength(keys, expectedLength) {
      if (keys.length < expectedLength) {
        throw new Error(`invalid instruction; found ${keys.length} keys, expected at least ${expectedLength}`);
      }
    }
  }
  class AddressLookupTableProgram {
    constructor() {}
    static createLookupTable(params) {
      const [lookupTableAddress, bumpSeed] = PublicKey.findProgramAddressSync($Array.of(params.authority.toBuffer(), toBufferLE_1(BigInt(params.recentSlot), 8)), this.programId);
      const type = LOOKUP_TABLE_INSTRUCTION_LAYOUTS.CreateLookupTable;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.recentSlot = BigInt(params.recentSlot);
        result.bumpSeed = bumpSeed;
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = lookupTableAddress;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.authority;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.payer;
        result.isSigner = true;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SystemProgram.programId;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })());
      return $Array.of(new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.programId = this.programId;
        result.keys = keys;
        result.data = data;
        return result;
      }).bind(this)()), lookupTableAddress);
    }
    static freezeLookupTable(params) {
      const type = LOOKUP_TABLE_INSTRUCTION_LAYOUTS.FreezeLookupTable;
      const data = encodeData(type);
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.lookupTable;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.authority;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.programId = this.programId;
        result.keys = keys;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static extendLookupTable(params) {
      const type = LOOKUP_TABLE_INSTRUCTION_LAYOUTS.ExtendLookupTable;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.addresses = params.addresses.map($(addr => addr.toBytes()));
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.lookupTable;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.authority;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      if (params.payer) {
        keys.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = params.payer;
          result.isSigner = true;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SystemProgram.programId;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })());
      }
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.programId = this.programId;
        result.keys = keys;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static deactivateLookupTable(params) {
      const type = LOOKUP_TABLE_INSTRUCTION_LAYOUTS.DeactivateLookupTable;
      const data = encodeData(type);
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.lookupTable;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.authority;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.programId = this.programId;
        result.keys = keys;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static closeLookupTable(params) {
      const type = LOOKUP_TABLE_INSTRUCTION_LAYOUTS.CloseLookupTable;
      const data = encodeData(type);
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.lookupTable;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.authority;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = params.recipient;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.programId = this.programId;
        result.keys = keys;
        result.data = data;
        return result;
      }).bind(this)());
    }
  }
  AddressLookupTableProgram.programId = new PublicKey("AddressLookupTab1e1111111111111111111111111");
  class ComputeBudgetInstruction {
    constructor() {}
    static decodeInstructionType(instruction) {
      this.checkProgramId(instruction.programId);
      const instructionTypeLayout = u8("instruction");
      const typeIndex = instructionTypeLayout.decode(instruction.data);
      let type;
      for (const [ixType, layout] of $Object.entries(COMPUTE_BUDGET_INSTRUCTION_LAYOUTS)) {
        if (layout.index == typeIndex) {
          type = ixType;
          break;
        }
      }
      if (!type) {
        throw new Error("Instruction type incorrect; not a ComputeBudgetInstruction");
      }
      return type;
    }
    static decodeRequestUnits(instruction) {
      this.checkProgramId(instruction.programId);
      const {
        units: units,
        additionalFee: additionalFee
      } = decodeData$1(COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.RequestUnits, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.units = units;
        result.additionalFee = additionalFee;
        return result;
      })();
    }
    static decodeRequestHeapFrame(instruction) {
      this.checkProgramId(instruction.programId);
      const {
        bytes: bytes
      } = decodeData$1(COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.RequestHeapFrame, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.bytes = bytes;
        return result;
      })();
    }
    static decodeSetComputeUnitLimit(instruction) {
      this.checkProgramId(instruction.programId);
      const {
        units: units
      } = decodeData$1(COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.SetComputeUnitLimit, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.units = units;
        return result;
      })();
    }
    static decodeSetComputeUnitPrice(instruction) {
      this.checkProgramId(instruction.programId);
      const {
        microLamports: microLamports
      } = decodeData$1(COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.SetComputeUnitPrice, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.microLamports = microLamports;
        return result;
      })();
    }
    static checkProgramId(programId) {
      if (!programId.equals(ComputeBudgetProgram.programId)) {
        throw new Error("invalid instruction; programId is not ComputeBudgetProgram");
      }
    }
  }
  const COMPUTE_BUDGET_INSTRUCTION_LAYOUTS = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.RequestUnits = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      result.layout = struct($Array.of(u8("instruction"), u32("units"), u32("additionalFee")));
      return result;
    })();
    result.RequestHeapFrame = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      result.layout = struct($Array.of(u8("instruction"), u32("bytes")));
      return result;
    })();
    result.SetComputeUnitLimit = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 2;
      result.layout = struct($Array.of(u8("instruction"), u32("units")));
      return result;
    })();
    result.SetComputeUnitPrice = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 3;
      result.layout = struct($Array.of(u8("instruction"), u64("microLamports")));
      return result;
    })();
    return result;
  })());
  class ComputeBudgetProgram {
    constructor() {}
    static requestUnits(params) {
      const type = COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.RequestUnits;
      const data = encodeData(type, params);
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static requestHeapFrame(params) {
      const type = COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.RequestHeapFrame;
      const data = encodeData(type, params);
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static setComputeUnitLimit(params) {
      const type = COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.SetComputeUnitLimit;
      const data = encodeData(type, params);
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static setComputeUnitPrice(params) {
      const type = COMPUTE_BUDGET_INSTRUCTION_LAYOUTS.SetComputeUnitPrice;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.microLamports = BigInt(params.microLamports);
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
  }
  ComputeBudgetProgram.programId = new PublicKey("ComputeBudget111111111111111111111111111111");
  const PRIVATE_KEY_BYTES$1 = 64;
  const PUBLIC_KEY_BYTES$1 = 32;
  const SIGNATURE_BYTES = 64;
  const ED25519_INSTRUCTION_LAYOUT = struct($Array.of(u8("numSignatures"), u8("padding"), u16("signatureOffset"), u16("signatureInstructionIndex"), u16("publicKeyOffset"), u16("publicKeyInstructionIndex"), u16("messageDataOffset"), u16("messageDataSize"), u16("messageInstructionIndex")));
  class Ed25519Program {
    constructor() {}
    static createInstructionWithPublicKey(params) {
      const {
        publicKey: publicKey,
        message: message,
        signature: signature,
        instructionIndex: instructionIndex
      } = params;
      assert$1(publicKey.length === PUBLIC_KEY_BYTES$1, `Public Key must be ${PUBLIC_KEY_BYTES$1} bytes but received ${publicKey.length} bytes`);
      assert$1(signature.length === SIGNATURE_BYTES, `Signature must be ${SIGNATURE_BYTES} bytes but received ${signature.length} bytes`);
      const publicKeyOffset = ED25519_INSTRUCTION_LAYOUT.span;
      const signatureOffset = publicKeyOffset + publicKey.length;
      const messageDataOffset = signatureOffset + signature.length;
      const numSignatures = 1;
      const instructionData = buffer.Buffer.alloc(messageDataOffset + message.length);
      const index = instructionIndex == null ? 65535 : instructionIndex;
      ED25519_INSTRUCTION_LAYOUT.encode($(function () {
        let result = $Object.create(null, undefined);
        result.numSignatures = numSignatures;
        result.padding = 0;
        result.signatureOffset = signatureOffset;
        result.signatureInstructionIndex = index;
        result.publicKeyOffset = publicKeyOffset;
        result.publicKeyInstructionIndex = index;
        result.messageDataOffset = messageDataOffset;
        result.messageDataSize = message.length;
        result.messageInstructionIndex = index;
        return result;
      })(), instructionData);
      instructionData.fill(publicKey, publicKeyOffset);
      instructionData.fill(signature, signatureOffset);
      instructionData.fill(message, messageDataOffset);
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = Ed25519Program.programId;
        result.data = instructionData;
        return result;
      })());
    }
    static createInstructionWithPrivateKey(params) {
      const {
        privateKey: privateKey,
        message: message,
        instructionIndex: instructionIndex
      } = params;
      assert$1(privateKey.length === PRIVATE_KEY_BYTES$1, `Private key must be ${PRIVATE_KEY_BYTES$1} bytes but received ${privateKey.length} bytes`);
      try {
        const keypair = Keypair.fromSecretKey(privateKey);
        const publicKey = keypair.publicKey.toBytes();
        const signature = sign(message, keypair.secretKey);
        return this.createInstructionWithPublicKey($(function () {
          let result = $Object.create(null, undefined);
          result.publicKey = publicKey;
          result.message = message;
          result.signature = signature;
          result.instructionIndex = instructionIndex;
          return result;
        })());
      } catch (error) {
        throw new Error(`Error creating instruction; ${error}`);
      }
    }
  }
  Ed25519Program.programId = new PublicKey("Ed25519SigVerify111111111111111111111111111");
  const U32_MASK64 = BigInt(2 ** 32 - 1);
  const _32n = BigInt(32);
  function fromBig(n, le = false) {
    if (le) return $(function () {
      let result = $Object.create(null, undefined);
      result.h = Number(n & U32_MASK64);
      result.l = Number(n >> _32n & U32_MASK64);
      return result;
    })();
    return $(function () {
      let result = $Object.create(null, undefined);
      result.h = Number(n >> _32n & U32_MASK64) | 0;
      result.l = Number(n & U32_MASK64) | 0;
      return result;
    })();
  }
  $(fromBig);
  function split(lst, le = false) {
    let Ah = new Uint32Array(lst.length);
    let Al = new Uint32Array(lst.length);
    for (let i = 0; i < lst.length; i++) {
      const {
        h: h,
        l: l
      } = fromBig(lst[i], le);
      [Ah[i], Al[i]] = $Array.of(h, l);
    }
    return $Array.of(Ah, Al);
  }
  $(split);
  const rotlSH = (h, l, s) => h << s | l >>> 32 - s;
  $(rotlSH);
  const rotlSL = (h, l, s) => l << s | h >>> 32 - s;
  $(rotlSL);
  const rotlBH = (h, l, s) => l << s - 32 | h >>> 64 - s;
  $(rotlBH);
  const rotlBL = (h, l, s) => h << s - 32 | l >>> 64 - s;
  $(rotlBL);
  const [SHA3_PI, SHA3_ROTL, _SHA3_IOTA] = $Array.of($Array.of(), $Array.of(), $Array.of());
  const _0n$1 = BigInt(0);
  const _1n$2 = BigInt(1);
  const _2n$1 = BigInt(2);
  const _7n = BigInt(7);
  const _256n = BigInt(256);
  const _0x71n = BigInt(113);
  for (let round = 0, R = _1n$2, x = 1, y = 0; round < 24; round++) {
    [x, y] = $Array.of(y, (2 * x + 3 * y) % 5);
    SHA3_PI.push(2 * (5 * y + x));
    SHA3_ROTL.push((round + 1) * (round + 2) / 2 % 64);
    let t = _0n$1;
    for (let j = 0; j < 7; j++) {
      R = (R << _1n$2 ^ (R >> _7n) * _0x71n) % _256n;
      if (R & _2n$1) t ^= _1n$2 << (_1n$2 << BigInt(j)) - _1n$2;
    }
    _SHA3_IOTA.push(t);
  }
  const [SHA3_IOTA_H, SHA3_IOTA_L] = split(_SHA3_IOTA, true);
  const rotlH = (h, l, s) => s > 32 ? rotlBH(h, l, s) : rotlSH(h, l, s);
  $(rotlH);
  const rotlL = (h, l, s) => s > 32 ? rotlBL(h, l, s) : rotlSL(h, l, s);
  $(rotlL);
  function keccakP(s, rounds = 24) {
    const B = new Uint32Array(5 * 2);
    for (let round = 24 - rounds; round < 24; round++) {
      for (let x = 0; x < 10; x++) B[x] = s[x] ^ s[x + 10] ^ s[x + 20] ^ s[x + 30] ^ s[x + 40];
      for (let x = 0; x < 10; x += 2) {
        const idx1 = (x + 8) % 10;
        const idx0 = (x + 2) % 10;
        const B0 = B[idx0];
        const B1 = B[idx0 + 1];
        const Th = rotlH(B0, B1, 1) ^ B[idx1];
        const Tl = rotlL(B0, B1, 1) ^ B[idx1 + 1];
        for (let y = 0; y < 50; y += 10) {
          s[x + y] ^= Th;
          s[x + y + 1] ^= Tl;
        }
      }
      let curH = s[2];
      let curL = s[3];
      for (let t = 0; t < 24; t++) {
        const shift = SHA3_ROTL[t];
        const Th = rotlH(curH, curL, shift);
        const Tl = rotlL(curH, curL, shift);
        const PI = SHA3_PI[t];
        curH = s[PI];
        curL = s[PI + 1];
        s[PI] = Th;
        s[PI + 1] = Tl;
      }
      for (let y = 0; y < 50; y += 10) {
        for (let x = 0; x < 10; x++) B[x] = s[y + x];
        for (let x = 0; x < 10; x++) s[y + x] ^= ~B[(x + 2) % 10] & B[(x + 4) % 10];
      }
      s[0] ^= SHA3_IOTA_H[round];
      s[1] ^= SHA3_IOTA_L[round];
    }
    B.fill(0);
  }
  $(keccakP);
  class Keccak extends Hash {
    constructor(blockLen, suffix, outputLen, enableXOF = false, rounds = 24) {
      super();
      this.blockLen = blockLen;
      this.suffix = suffix;
      this.outputLen = outputLen;
      this.enableXOF = enableXOF;
      this.rounds = rounds;
      this.pos = 0;
      this.posOut = 0;
      this.finished = false;
      this.destroyed = false;
      number$1(outputLen);
      if (0 >= this.blockLen || this.blockLen >= 200) throw new Error("Sha3 supports only keccak-f1600 function");
      this.state = new Uint8Array(200);
      this.state32 = u32$1(this.state);
    }
    keccak() {
      keccakP(this.state32, this.rounds);
      this.posOut = 0;
      this.pos = 0;
    }
    update(data) {
      exists(this);
      const {
        blockLen: blockLen,
        state: state
      } = this;
      data = toBytes(data);
      const len = data.length;
      for (let pos = 0; pos < len;) {
        const take = Math.min(blockLen - this.pos, len - pos);
        for (let i = 0; i < take; i++) state[this.pos++] ^= data[pos++];
        if (this.pos === blockLen) this.keccak();
      }
      return this;
    }
    finish() {
      if (this.finished) return;
      this.finished = true;
      const {
        state: state,
        suffix: suffix,
        pos: pos,
        blockLen: blockLen
      } = this;
      state[pos] ^= suffix;
      if ((suffix & 128) !== 0 && pos === blockLen - 1) this.keccak();
      state[blockLen - 1] ^= 128;
      this.keccak();
    }
    writeInto(out) {
      exists(this, false);
      bytes(out);
      this.finish();
      const bufferOut = this.state;
      const {
        blockLen: blockLen
      } = this;
      for (let pos = 0, len = out.length; pos < len;) {
        if (this.posOut >= blockLen) this.keccak();
        const take = Math.min(blockLen - this.posOut, len - pos);
        out.set(bufferOut.subarray(this.posOut, this.posOut + take), pos);
        this.posOut += take;
        pos += take;
      }
      return out;
    }
    xofInto(out) {
      if (!this.enableXOF) throw new Error("XOF is not possible for this instance");
      return this.writeInto(out);
    }
    xof(bytes) {
      number$1(bytes);
      return this.xofInto(new Uint8Array(bytes));
    }
    digestInto(out) {
      output(out, this);
      if (this.finished) throw new Error("digest() was already called");
      this.writeInto(out);
      this.destroy();
      return out;
    }
    digest() {
      return this.digestInto(new Uint8Array(this.outputLen));
    }
    destroy() {
      this.destroyed = true;
      this.state.fill(0);
    }
    _cloneInto(to) {
      const {
        blockLen: blockLen,
        suffix: suffix,
        outputLen: outputLen,
        rounds: rounds,
        enableXOF: enableXOF
      } = this;
      to || (to = new Keccak(blockLen, suffix, outputLen, enableXOF, rounds));
      to.state32.set(this.state32);
      to.pos = this.pos;
      to.posOut = this.posOut;
      to.finished = this.finished;
      to.rounds = rounds;
      to.suffix = suffix;
      to.outputLen = outputLen;
      to.enableXOF = enableXOF;
      to.destroyed = this.destroyed;
      return to;
    }
  }
  const gen = (suffix, blockLen, outputLen) => wrapConstructor($(() => new Keccak(blockLen, suffix, outputLen)));
  $(gen);
  const keccak_256 = gen(1, 136, 256 / 8);
  const SHA256_K = new Uint32Array($Array.of(1116352408, 1899447441, 3049323471, 3921009573, 961987163, 1508970993, 2453635748, 2870763221, 3624381080, 310598401, 607225278, 1426881987, 1925078388, 2162078206, 2614888103, 3248222580, 3835390401, 4022224774, 264347078, 604807628, 770255983, 1249150122, 1555081692, 1996064986, 2554220882, 2821834349, 2952996808, 3210313671, 3336571891, 3584528711, 113926993, 338241895, 666307205, 773529912, 1294757372, 1396182291, 1695183700, 1986661051, 2177026350, 2456956037, 2730485921, 2820302411, 3259730800, 3345764771, 3516065817, 3600352804, 4094571909, 275423344, 430227734, 506948616, 659060556, 883997877, 958139571, 1322822218, 1537002063, 1747873779, 1955562222, 2024104815, 2227730452, 2361852424, 2428436474, 2756734187, 3204031479, 3329325298));
  const SHA256_IV = new Uint32Array($Array.of(1779033703, 3144134277, 1013904242, 2773480762, 1359893119, 2600822924, 528734635, 1541459225));
  const SHA256_W = new Uint32Array(64);
  class SHA256 extends HashMD {
    constructor() {
      super(64, 32, 8, false);
      this.A = SHA256_IV[0] | 0;
      this.B = SHA256_IV[1] | 0;
      this.C = SHA256_IV[2] | 0;
      this.D = SHA256_IV[3] | 0;
      this.E = SHA256_IV[4] | 0;
      this.F = SHA256_IV[5] | 0;
      this.G = SHA256_IV[6] | 0;
      this.H = SHA256_IV[7] | 0;
    }
    get() {
      const {
        A: A,
        B: B,
        C: C,
        D: D,
        E: E,
        F: F,
        G: G,
        H: H
      } = this;
      return $Array.of(A, B, C, D, E, F, G, H);
    }
    set(A, B, C, D, E, F, G, H) {
      this.A = A | 0;
      this.B = B | 0;
      this.C = C | 0;
      this.D = D | 0;
      this.E = E | 0;
      this.F = F | 0;
      this.G = G | 0;
      this.H = H | 0;
    }
    process(view, offset) {
      for (let i = 0; i < 16; i++, offset += 4) SHA256_W[i] = view.getUint32(offset, false);
      for (let i = 16; i < 64; i++) {
        const W15 = SHA256_W[i - 15];
        const W2 = SHA256_W[i - 2];
        const s0 = rotr$1(W15, 7) ^ rotr$1(W15, 18) ^ W15 >>> 3;
        const s1 = rotr$1(W2, 17) ^ rotr$1(W2, 19) ^ W2 >>> 10;
        SHA256_W[i] = s1 + SHA256_W[i - 7] + s0 + SHA256_W[i - 16] | 0;
      }
      let {
        A: A,
        B: B,
        C: C,
        D: D,
        E: E,
        F: F,
        G: G,
        H: H
      } = this;
      for (let i = 0; i < 64; i++) {
        const sigma1 = rotr$1(E, 6) ^ rotr$1(E, 11) ^ rotr$1(E, 25);
        const T1 = H + sigma1 + Chi$1(E, F, G) + SHA256_K[i] + SHA256_W[i] | 0;
        const sigma0 = rotr$1(A, 2) ^ rotr$1(A, 13) ^ rotr$1(A, 22);
        const T2 = sigma0 + Maj$1(A, B, C) | 0;
        H = G;
        G = F;
        F = E;
        E = D + T1 | 0;
        D = C;
        C = B;
        B = A;
        A = T1 + T2 | 0;
      }
      A = A + this.A | 0;
      B = B + this.B | 0;
      C = C + this.C | 0;
      D = D + this.D | 0;
      E = E + this.E | 0;
      F = F + this.F | 0;
      G = G + this.G | 0;
      H = H + this.H | 0;
      this.set(A, B, C, D, E, F, G, H);
    }
    roundClean() {
      SHA256_W.fill(0);
    }
    destroy() {
      this.set(0, 0, 0, 0, 0, 0, 0, 0);
      this.buffer.fill(0);
    }
  }
  const sha256 = wrapConstructor$1($(() => new SHA256()));
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  function validatePointOpts(curve) {
    const opts = validateBasic(curve);
    validateObject(opts, $(function () {
      let result = $Object.create(null, undefined);
      result.a = "field";
      result.b = "field";
      return result;
    })(), $(function () {
      let result = $Object.create(null, undefined);
      result.allowedPrivateKeyLengths = "array";
      result.wrapPrivateKey = "boolean";
      result.isTorsionFree = "function";
      result.clearCofactor = "function";
      result.allowInfinityPoint = "boolean";
      result.fromBytes = "function";
      result.toBytes = "function";
      return result;
    })());
    const {
      endo: endo,
      Fp: Fp,
      a: a
    } = opts;
    if (endo) {
      if (!Fp.eql(a, Fp.ZERO)) {
        throw new Error("Endomorphism can only be defined for Koblitz curves that have a=0");
      }
      if (typeof endo !== "object" || typeof endo.beta !== "bigint" || typeof endo.splitScalar !== "function") {
        throw new Error("Expected endomorphism with beta: bigint and splitScalar: function");
      }
    }
    return $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, opts);
      return result;
    })());
  }
  $(validatePointOpts);
  const {
    bytesToNumberBE: b2n,
    hexToBytes: h2b
  } = ut;
  const DER = $(function () {
    let result = $Object.create(null, undefined);
    result.Err = class DERErr extends Error {
      constructor(m = "") {
        super(m);
      }
    };
    result._parseInt = $(function (data) {
      const {
        Err: E
      } = DER;
      if (data.length < 2 || data[0] !== 2) throw new E("Invalid signature integer tag");
      const len = data[1];
      const res = data.subarray(2, len + 2);
      if (!len || res.length !== len) throw new E("Invalid signature integer: wrong length");
      if (res[0] & 128) throw new E("Invalid signature integer: negative");
      if (res[0] === 0 && !(res[1] & 128)) throw new E("Invalid signature integer: unnecessary leading zero");
      return $(function () {
        let result = $Object.create(null, undefined);
        result.d = b2n(res);
        result.l = data.subarray(len + 2);
        return result;
      })();
    });
    result.toSig = $(function (hex) {
      const {
        Err: E
      } = DER;
      const data = typeof hex === "string" ? h2b(hex) : hex;
      abytes(data);
      let l = data.length;
      if (l < 2 || data[0] != 48) throw new E("Invalid signature tag");
      if (data[1] !== l - 2) throw new E("Invalid signature: incorrect length");
      const {
        d: r,
        l: sBytes
      } = DER._parseInt(data.subarray(2));
      const {
        d: s,
        l: rBytesLeft
      } = DER._parseInt(sBytes);
      if (rBytesLeft.length) throw new E("Invalid signature: left bytes after parsing");
      return $(function () {
        let result = $Object.create(null, undefined);
        result.r = r;
        result.s = s;
        return result;
      })();
    });
    result.hexFromSig = $(function (sig) {
      const slice = s => Number.parseInt(s[0], 16) & 8 ? "00" + s : s;
      $(slice);
      const h = num => {
        const hex = num.toString(16);
        return hex.length & 1 ? `0${hex}` : hex;
      };
      $(h);
      const s = slice(h(sig.s));
      const r = slice(h(sig.r));
      const shl = s.length / 2;
      const rhl = r.length / 2;
      const sl = h(shl);
      const rl = h(rhl);
      return `30${h(rhl + shl + 4)}02${rl}${r}02${sl}${s}`;
    });
    return result;
  })();
  const _0n = BigInt(0),
    _1n$1 = BigInt(1);
  BigInt(2);
  const _3n = BigInt(3);
  BigInt(4);
  function weierstrassPoints(opts) {
    const CURVE = validatePointOpts(opts);
    const {
      Fp: Fp
    } = CURVE;
    const toBytes = CURVE.toBytes || $((_c, point, _isCompressed) => {
      const a = point.toAffine();
      return concatBytes(Uint8Array.from($Array.of(4)), Fp.toBytes(a.x), Fp.toBytes(a.y));
    });
    const fromBytes = CURVE.fromBytes || $(bytes => {
      const tail = bytes.subarray(1);
      const x = Fp.fromBytes(tail.subarray(0, Fp.BYTES));
      const y = Fp.fromBytes(tail.subarray(Fp.BYTES, 2 * Fp.BYTES));
      return $(function () {
        let result = $Object.create(null, undefined);
        result.x = x;
        result.y = y;
        return result;
      })();
    });
    function weierstrassEquation(x) {
      const {
        a: a,
        b: b
      } = CURVE;
      const x2 = Fp.sqr(x);
      const x3 = Fp.mul(x2, x);
      return Fp.add(Fp.add(x3, Fp.mul(x, a)), b);
    }
    $(weierstrassEquation);
    if (!Fp.eql(Fp.sqr(CURVE.Gy), weierstrassEquation(CURVE.Gx))) throw new Error("bad generator point: equation left != right");
    function isWithinCurveOrder(num) {
      return typeof num === "bigint" && _0n < num && num < CURVE.n;
    }
    $(isWithinCurveOrder);
    function assertGE(num) {
      if (!isWithinCurveOrder(num)) throw new Error("Expected valid bigint: 0 < bigint < curve.n");
    }
    $(assertGE);
    function normPrivateKeyToScalar(key) {
      const {
        allowedPrivateKeyLengths: lengths,
        nByteLength: nByteLength,
        wrapPrivateKey: wrapPrivateKey,
        n: n
      } = CURVE;
      if (lengths && typeof key !== "bigint") {
        if (isBytes$2(key)) key = bytesToHex(key);
        if (typeof key !== "string" || !lengths.includes(key.length)) throw new Error("Invalid key");
        key = key.padStart(nByteLength * 2, "0");
      }
      let num;
      try {
        num = typeof key === "bigint" ? key : bytesToNumberBE(ensureBytes("private key", key, nByteLength));
      } catch (error) {
        throw new Error(`private key must be ${nByteLength} bytes, hex or bigint, not ${typeof key}`);
      }
      if (wrapPrivateKey) num = mod(num, n);
      assertGE(num);
      return num;
    }
    $(normPrivateKeyToScalar);
    const pointPrecomputes = new Map();
    function assertPrjPoint(other) {
      if (!(other instanceof Point)) throw new Error("ProjectivePoint expected");
    }
    $(assertPrjPoint);
    class Point {
      constructor(px, py, pz) {
        this.px = px;
        this.py = py;
        this.pz = pz;
        if (px == null || !Fp.isValid(px)) throw new Error("x required");
        if (py == null || !Fp.isValid(py)) throw new Error("y required");
        if (pz == null || !Fp.isValid(pz)) throw new Error("z required");
      }
      static fromAffine(p) {
        const {
          x: x,
          y: y
        } = p || $Object.create(null, undefined);
        if (!p || !Fp.isValid(x) || !Fp.isValid(y)) throw new Error("invalid affine point");
        if (p instanceof Point) throw new Error("projective point not allowed");
        const is0 = i => Fp.eql(i, Fp.ZERO);
        $(is0);
        if (is0(x) && is0(y)) return Point.ZERO;
        return new Point(x, y, Fp.ONE);
      }
      get x() {
        return this.toAffine().x;
      }
      get y() {
        return this.toAffine().y;
      }
      static normalizeZ(points) {
        const toInv = Fp.invertBatch(points.map($(p => p.pz)));
        return points.map($((p, i) => p.toAffine(toInv[i]))).map(Point.fromAffine);
      }
      static fromHex(hex) {
        const P = Point.fromAffine(fromBytes(ensureBytes("pointHex", hex)));
        P.assertValidity();
        return P;
      }
      static fromPrivateKey(privateKey) {
        return Point.BASE.multiply(normPrivateKeyToScalar(privateKey));
      }
      _setWindowSize(windowSize) {
        this._WINDOW_SIZE = windowSize;
        pointPrecomputes.delete(this);
      }
      assertValidity() {
        if (this.is0()) {
          if (CURVE.allowInfinityPoint && !Fp.is0(this.py)) return;
          throw new Error("bad point: ZERO");
        }
        const {
          x: x,
          y: y
        } = this.toAffine();
        if (!Fp.isValid(x) || !Fp.isValid(y)) throw new Error("bad point: x or y not FE");
        const left = Fp.sqr(y);
        const right = weierstrassEquation(x);
        if (!Fp.eql(left, right)) throw new Error("bad point: equation left != right");
        if (!this.isTorsionFree()) throw new Error("bad point: not in prime-order subgroup");
      }
      hasEvenY() {
        const {
          y: y
        } = this.toAffine();
        if (Fp.isOdd) return !Fp.isOdd(y);
        throw new Error("Field doesn't support isOdd");
      }
      equals(other) {
        assertPrjPoint(other);
        const {
          px: X1,
          py: Y1,
          pz: Z1
        } = this;
        const {
          px: X2,
          py: Y2,
          pz: Z2
        } = other;
        const U1 = Fp.eql(Fp.mul(X1, Z2), Fp.mul(X2, Z1));
        const U2 = Fp.eql(Fp.mul(Y1, Z2), Fp.mul(Y2, Z1));
        return U1 && U2;
      }
      negate() {
        return new Point(this.px, Fp.neg(this.py), this.pz);
      }
      double() {
        const {
          a: a,
          b: b
        } = CURVE;
        const b3 = Fp.mul(b, _3n);
        const {
          px: X1,
          py: Y1,
          pz: Z1
        } = this;
        let X3 = Fp.ZERO,
          Y3 = Fp.ZERO,
          Z3 = Fp.ZERO;
        let t0 = Fp.mul(X1, X1);
        let t1 = Fp.mul(Y1, Y1);
        let t2 = Fp.mul(Z1, Z1);
        let t3 = Fp.mul(X1, Y1);
        t3 = Fp.add(t3, t3);
        Z3 = Fp.mul(X1, Z1);
        Z3 = Fp.add(Z3, Z3);
        X3 = Fp.mul(a, Z3);
        Y3 = Fp.mul(b3, t2);
        Y3 = Fp.add(X3, Y3);
        X3 = Fp.sub(t1, Y3);
        Y3 = Fp.add(t1, Y3);
        Y3 = Fp.mul(X3, Y3);
        X3 = Fp.mul(t3, X3);
        Z3 = Fp.mul(b3, Z3);
        t2 = Fp.mul(a, t2);
        t3 = Fp.sub(t0, t2);
        t3 = Fp.mul(a, t3);
        t3 = Fp.add(t3, Z3);
        Z3 = Fp.add(t0, t0);
        t0 = Fp.add(Z3, t0);
        t0 = Fp.add(t0, t2);
        t0 = Fp.mul(t0, t3);
        Y3 = Fp.add(Y3, t0);
        t2 = Fp.mul(Y1, Z1);
        t2 = Fp.add(t2, t2);
        t0 = Fp.mul(t2, t3);
        X3 = Fp.sub(X3, t0);
        Z3 = Fp.mul(t2, t1);
        Z3 = Fp.add(Z3, Z3);
        Z3 = Fp.add(Z3, Z3);
        return new Point(X3, Y3, Z3);
      }
      add(other) {
        assertPrjPoint(other);
        const {
          px: X1,
          py: Y1,
          pz: Z1
        } = this;
        const {
          px: X2,
          py: Y2,
          pz: Z2
        } = other;
        let X3 = Fp.ZERO,
          Y3 = Fp.ZERO,
          Z3 = Fp.ZERO;
        const a = CURVE.a;
        const b3 = Fp.mul(CURVE.b, _3n);
        let t0 = Fp.mul(X1, X2);
        let t1 = Fp.mul(Y1, Y2);
        let t2 = Fp.mul(Z1, Z2);
        let t3 = Fp.add(X1, Y1);
        let t4 = Fp.add(X2, Y2);
        t3 = Fp.mul(t3, t4);
        t4 = Fp.add(t0, t1);
        t3 = Fp.sub(t3, t4);
        t4 = Fp.add(X1, Z1);
        let t5 = Fp.add(X2, Z2);
        t4 = Fp.mul(t4, t5);
        t5 = Fp.add(t0, t2);
        t4 = Fp.sub(t4, t5);
        t5 = Fp.add(Y1, Z1);
        X3 = Fp.add(Y2, Z2);
        t5 = Fp.mul(t5, X3);
        X3 = Fp.add(t1, t2);
        t5 = Fp.sub(t5, X3);
        Z3 = Fp.mul(a, t4);
        X3 = Fp.mul(b3, t2);
        Z3 = Fp.add(X3, Z3);
        X3 = Fp.sub(t1, Z3);
        Z3 = Fp.add(t1, Z3);
        Y3 = Fp.mul(X3, Z3);
        t1 = Fp.add(t0, t0);
        t1 = Fp.add(t1, t0);
        t2 = Fp.mul(a, t2);
        t4 = Fp.mul(b3, t4);
        t1 = Fp.add(t1, t2);
        t2 = Fp.sub(t0, t2);
        t2 = Fp.mul(a, t2);
        t4 = Fp.add(t4, t2);
        t0 = Fp.mul(t1, t4);
        Y3 = Fp.add(Y3, t0);
        t0 = Fp.mul(t5, t4);
        X3 = Fp.mul(t3, X3);
        X3 = Fp.sub(X3, t0);
        t0 = Fp.mul(t3, t1);
        Z3 = Fp.mul(t5, Z3);
        Z3 = Fp.add(Z3, t0);
        return new Point(X3, Y3, Z3);
      }
      subtract(other) {
        return this.add(other.negate());
      }
      is0() {
        return this.equals(Point.ZERO);
      }
      wNAF(n) {
        return wnaf.wNAFCached(this, pointPrecomputes, n, $(comp => {
          const toInv = Fp.invertBatch(comp.map($(p => p.pz)));
          return comp.map($((p, i) => p.toAffine(toInv[i]))).map(Point.fromAffine);
        }));
      }
      multiplyUnsafe(n) {
        const I = Point.ZERO;
        if (n === _0n) return I;
        assertGE(n);
        if (n === _1n$1) return this;
        const {
          endo: endo
        } = CURVE;
        if (!endo) return wnaf.unsafeLadder(this, n);
        let {
          k1neg: k1neg,
          k1: k1,
          k2neg: k2neg,
          k2: k2
        } = endo.splitScalar(n);
        let k1p = I;
        let k2p = I;
        let d = this;
        while (k1 > _0n || k2 > _0n) {
          if (k1 & _1n$1) k1p = k1p.add(d);
          if (k2 & _1n$1) k2p = k2p.add(d);
          d = d.double();
          k1 >>= _1n$1;
          k2 >>= _1n$1;
        }
        if (k1neg) k1p = k1p.negate();
        if (k2neg) k2p = k2p.negate();
        k2p = new Point(Fp.mul(k2p.px, endo.beta), k2p.py, k2p.pz);
        return k1p.add(k2p);
      }
      multiply(scalar) {
        assertGE(scalar);
        let n = scalar;
        let point, fake;
        const {
          endo: endo
        } = CURVE;
        if (endo) {
          const {
            k1neg: k1neg,
            k1: k1,
            k2neg: k2neg,
            k2: k2
          } = endo.splitScalar(n);
          let {
            p: k1p,
            f: f1p
          } = this.wNAF(k1);
          let {
            p: k2p,
            f: f2p
          } = this.wNAF(k2);
          k1p = wnaf.constTimeNegate(k1neg, k1p);
          k2p = wnaf.constTimeNegate(k2neg, k2p);
          k2p = new Point(Fp.mul(k2p.px, endo.beta), k2p.py, k2p.pz);
          point = k1p.add(k2p);
          fake = f1p.add(f2p);
        } else {
          const {
            p: p,
            f: f
          } = this.wNAF(n);
          point = p;
          fake = f;
        }
        return Point.normalizeZ($Array.of(point, fake))[0];
      }
      multiplyAndAddUnsafe(Q, a, b) {
        const G = Point.BASE;
        const mul = (P, a) => a === _0n || a === _1n$1 || !P.equals(G) ? P.multiplyUnsafe(a) : P.multiply(a);
        $(mul);
        const sum = mul(this, a).add(mul(Q, b));
        return sum.is0() ? undefined : sum;
      }
      toAffine(iz) {
        const {
          px: x,
          py: y,
          pz: z
        } = this;
        const is0 = this.is0();
        if (iz == null) iz = is0 ? Fp.ONE : Fp.inv(z);
        const ax = Fp.mul(x, iz);
        const ay = Fp.mul(y, iz);
        const zz = Fp.mul(z, iz);
        if (is0) return $(function () {
          let result = $Object.create(null, undefined);
          result.x = Fp.ZERO;
          result.y = Fp.ZERO;
          return result;
        })();
        if (!Fp.eql(zz, Fp.ONE)) throw new Error("invZ was invalid");
        return $(function () {
          let result = $Object.create(null, undefined);
          result.x = ax;
          result.y = ay;
          return result;
        })();
      }
      isTorsionFree() {
        const {
          h: cofactor,
          isTorsionFree: isTorsionFree
        } = CURVE;
        if (cofactor === _1n$1) return true;
        if (isTorsionFree) return isTorsionFree(Point, this);
        throw new Error("isTorsionFree() has not been declared for the elliptic curve");
      }
      clearCofactor() {
        const {
          h: cofactor,
          clearCofactor: clearCofactor
        } = CURVE;
        if (cofactor === _1n$1) return this;
        if (clearCofactor) return clearCofactor(Point, this);
        return this.multiplyUnsafe(CURVE.h);
      }
      toRawBytes(isCompressed = true) {
        this.assertValidity();
        return toBytes(Point, this, isCompressed);
      }
      toHex(isCompressed = true) {
        return bytesToHex(this.toRawBytes(isCompressed));
      }
    }
    Point.BASE = new Point(CURVE.Gx, CURVE.Gy, Fp.ONE);
    Point.ZERO = new Point(Fp.ZERO, Fp.ONE, Fp.ZERO);
    const _bits = CURVE.nBitLength;
    const wnaf = wNAF(Point, CURVE.endo ? Math.ceil(_bits / 2) : _bits);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.CURVE = CURVE;
      result.ProjectivePoint = Point;
      result.normPrivateKeyToScalar = normPrivateKeyToScalar;
      result.weierstrassEquation = weierstrassEquation;
      result.isWithinCurveOrder = isWithinCurveOrder;
      return result;
    })();
  }
  $(weierstrassPoints);
  function validateOpts(curve) {
    const opts = validateBasic(curve);
    validateObject(opts, $(function () {
      let result = $Object.create(null, undefined);
      result.hash = "hash";
      result.hmac = "function";
      result.randomBytes = "function";
      return result;
    })(), $(function () {
      let result = $Object.create(null, undefined);
      result.bits2int = "function";
      result.bits2int_modN = "function";
      result.lowS = "boolean";
      return result;
    })());
    return $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      result.lowS = true;
      $Object.assign(result, opts);
      return result;
    })());
  }
  $(validateOpts);
  function weierstrass(curveDef) {
    const CURVE = validateOpts(curveDef);
    const {
      Fp: Fp,
      n: CURVE_ORDER
    } = CURVE;
    const compressedLen = Fp.BYTES + 1;
    const uncompressedLen = 2 * Fp.BYTES + 1;
    function isValidFieldElement(num) {
      return _0n < num && num < Fp.ORDER;
    }
    $(isValidFieldElement);
    function modN(a) {
      return mod(a, CURVE_ORDER);
    }
    $(modN);
    function invN(a) {
      return invert(a, CURVE_ORDER);
    }
    $(invN);
    const {
      ProjectivePoint: Point,
      normPrivateKeyToScalar: normPrivateKeyToScalar,
      weierstrassEquation: weierstrassEquation,
      isWithinCurveOrder: isWithinCurveOrder
    } = weierstrassPoints($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, CURVE);
      result.toBytes = $(function (_c, point, isCompressed) {
        const a = point.toAffine();
        const x = Fp.toBytes(a.x);
        const cat = concatBytes;
        if (isCompressed) {
          return cat(Uint8Array.from($Array.of(point.hasEvenY() ? 2 : 3)), x);
        } else {
          return cat(Uint8Array.from($Array.of(4)), x, Fp.toBytes(a.y));
        }
      });
      result.fromBytes = $(function (bytes) {
        const len = bytes.length;
        const head = bytes[0];
        const tail = bytes.subarray(1);
        if (len === compressedLen && (head === 2 || head === 3)) {
          const x = bytesToNumberBE(tail);
          if (!isValidFieldElement(x)) throw new Error("Point is not on curve");
          const y2 = weierstrassEquation(x);
          let y;
          try {
            y = Fp.sqrt(y2);
          } catch (sqrtError) {
            const suffix = sqrtError instanceof Error ? ": " + sqrtError.message : "";
            throw new Error("Point is not on curve" + suffix);
          }
          const isYOdd = (y & _1n$1) === _1n$1;
          const isHeadOdd = (head & 1) === 1;
          if (isHeadOdd !== isYOdd) y = Fp.neg(y);
          return $(function () {
            let result = $Object.create(null, undefined);
            result.x = x;
            result.y = y;
            return result;
          })();
        } else if (len === uncompressedLen && head === 4) {
          const x = Fp.fromBytes(tail.subarray(0, Fp.BYTES));
          const y = Fp.fromBytes(tail.subarray(Fp.BYTES, 2 * Fp.BYTES));
          return $(function () {
            let result = $Object.create(null, undefined);
            result.x = x;
            result.y = y;
            return result;
          })();
        } else {
          throw new Error(`Point of length ${len} was invalid. Expected ${compressedLen} compressed bytes or ${uncompressedLen} uncompressed bytes`);
        }
      });
      return result;
    })());
    const numToNByteStr = num => bytesToHex(numberToBytesBE(num, CURVE.nByteLength));
    $(numToNByteStr);
    function isBiggerThanHalfOrder(number) {
      const HALF = CURVE_ORDER >> _1n$1;
      return number > HALF;
    }
    $(isBiggerThanHalfOrder);
    function normalizeS(s) {
      return isBiggerThanHalfOrder(s) ? modN(-s) : s;
    }
    $(normalizeS);
    const slcNum = (b, from, to) => bytesToNumberBE(b.slice(from, to));
    $(slcNum);
    class Signature {
      constructor(r, s, recovery) {
        this.r = r;
        this.s = s;
        this.recovery = recovery;
        this.assertValidity();
      }
      static fromCompact(hex) {
        const l = CURVE.nByteLength;
        hex = ensureBytes("compactSignature", hex, l * 2);
        return new Signature(slcNum(hex, 0, l), slcNum(hex, l, 2 * l));
      }
      static fromDER(hex) {
        const {
          r: r,
          s: s
        } = DER.toSig(ensureBytes("DER", hex));
        return new Signature(r, s);
      }
      assertValidity() {
        if (!isWithinCurveOrder(this.r)) throw new Error("r must be 0 < r < CURVE.n");
        if (!isWithinCurveOrder(this.s)) throw new Error("s must be 0 < s < CURVE.n");
      }
      addRecoveryBit(recovery) {
        return new Signature(this.r, this.s, recovery);
      }
      recoverPublicKey(msgHash) {
        const {
          r: r,
          s: s,
          recovery: rec
        } = this;
        const h = bits2int_modN(ensureBytes("msgHash", msgHash));
        if (rec == null || !$Array.of(0, 1, 2, 3).includes(rec)) throw new Error("recovery id invalid");
        const radj = rec === 2 || rec === 3 ? r + CURVE.n : r;
        if (radj >= Fp.ORDER) throw new Error("recovery id 2 or 3 invalid");
        const prefix = (rec & 1) === 0 ? "02" : "03";
        const R = Point.fromHex(prefix + numToNByteStr(radj));
        const ir = invN(radj);
        const u1 = modN(-h * ir);
        const u2 = modN(s * ir);
        const Q = Point.BASE.multiplyAndAddUnsafe(R, u1, u2);
        if (!Q) throw new Error("point at infinify");
        Q.assertValidity();
        return Q;
      }
      hasHighS() {
        return isBiggerThanHalfOrder(this.s);
      }
      normalizeS() {
        return this.hasHighS() ? new Signature(this.r, modN(-this.s), this.recovery) : this;
      }
      toDERRawBytes() {
        return hexToBytes(this.toDERHex());
      }
      toDERHex() {
        return DER.hexFromSig($(function () {
          let result = $Object.create(null, undefined);
          result.r = this.r;
          result.s = this.s;
          return result;
        }).bind(this)());
      }
      toCompactRawBytes() {
        return hexToBytes(this.toCompactHex());
      }
      toCompactHex() {
        return numToNByteStr(this.r) + numToNByteStr(this.s);
      }
    }
    const utils = $(function () {
      let result = $Object.create(null, undefined);
      result.isValidPrivateKey = $(function (privateKey) {
        try {
          normPrivateKeyToScalar(privateKey);
          return true;
        } catch (error) {
          return false;
        }
      });
      result.normPrivateKeyToScalar = normPrivateKeyToScalar;
      result.randomPrivateKey = $(() => {
        const length = getMinHashLength(CURVE.n);
        return mapHashToField(CURVE.randomBytes(length), CURVE.n);
      });
      result.precompute = $(function (windowSize = 8, point = Point.BASE) {
        point._setWindowSize(windowSize);
        point.multiply(BigInt(3));
        return point;
      });
      return result;
    })();
    function getPublicKey(privateKey, isCompressed = true) {
      return Point.fromPrivateKey(privateKey).toRawBytes(isCompressed);
    }
    $(getPublicKey);
    function isProbPub(item) {
      const arr = isBytes$2(item);
      const str = typeof item === "string";
      const len = (arr || str) && item.length;
      if (arr) return len === compressedLen || len === uncompressedLen;
      if (str) return len === 2 * compressedLen || len === 2 * uncompressedLen;
      if (item instanceof Point) return true;
      return false;
    }
    $(isProbPub);
    function getSharedSecret(privateA, publicB, isCompressed = true) {
      if (isProbPub(privateA)) throw new Error("first arg must be private key");
      if (!isProbPub(publicB)) throw new Error("second arg must be public key");
      const b = Point.fromHex(publicB);
      return b.multiply(normPrivateKeyToScalar(privateA)).toRawBytes(isCompressed);
    }
    $(getSharedSecret);
    const bits2int = CURVE.bits2int || $(function (bytes) {
      const num = bytesToNumberBE(bytes);
      const delta = bytes.length * 8 - CURVE.nBitLength;
      return delta > 0 ? num >> BigInt(delta) : num;
    });
    const bits2int_modN = CURVE.bits2int_modN || $(function (bytes) {
      return modN(bits2int(bytes));
    });
    const ORDER_MASK = bitMask(CURVE.nBitLength);
    function int2octets(num) {
      if (typeof num !== "bigint") throw new Error("bigint expected");
      if (!(_0n <= num && num < ORDER_MASK)) throw new Error(`bigint expected < 2^${CURVE.nBitLength}`);
      return numberToBytesBE(num, CURVE.nByteLength);
    }
    $(int2octets);
    function prepSig(msgHash, privateKey, opts = defaultSigOpts) {
      if ($Array.of("recovered", "canonical").some($(k => k in opts))) throw new Error("sign() legacy options not supported");
      const {
        hash: hash,
        randomBytes: randomBytes
      } = CURVE;
      let {
        lowS: lowS,
        prehash: prehash,
        extraEntropy: ent
      } = opts;
      if (lowS == null) lowS = true;
      msgHash = ensureBytes("msgHash", msgHash);
      if (prehash) msgHash = ensureBytes("prehashed msgHash", hash(msgHash));
      const h1int = bits2int_modN(msgHash);
      const d = normPrivateKeyToScalar(privateKey);
      const seedArgs = $Array.of(int2octets(d), int2octets(h1int));
      if (ent != null && ent !== false) {
        const e = ent === true ? randomBytes(Fp.BYTES) : ent;
        seedArgs.push(ensureBytes("extraEntropy", e));
      }
      const seed = concatBytes(...seedArgs);
      const m = h1int;
      function k2sig(kBytes) {
        const k = bits2int(kBytes);
        if (!isWithinCurveOrder(k)) return;
        const ik = invN(k);
        const q = Point.BASE.multiply(k).toAffine();
        const r = modN(q.x);
        if (r === _0n) return;
        const s = modN(ik * modN(m + r * d));
        if (s === _0n) return;
        let recovery = (q.x === r ? 0 : 2) | Number(q.y & _1n$1);
        let normS = s;
        if (lowS && isBiggerThanHalfOrder(s)) {
          normS = normalizeS(s);
          recovery ^= 1;
        }
        return new Signature(r, normS, recovery);
      }
      $(k2sig);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.seed = seed;
        result.k2sig = k2sig;
        return result;
      })();
    }
    $(prepSig);
    const defaultSigOpts = $(function () {
      let result = $Object.create(null, undefined);
      result.lowS = CURVE.lowS;
      result.prehash = false;
      return result;
    })();
    const defaultVerOpts = $(function () {
      let result = $Object.create(null, undefined);
      result.lowS = CURVE.lowS;
      result.prehash = false;
      return result;
    })();
    function sign(msgHash, privKey, opts = defaultSigOpts) {
      const {
        seed: seed,
        k2sig: k2sig
      } = prepSig(msgHash, privKey, opts);
      const C = CURVE;
      const drbg = createHmacDrbg(C.hash.outputLen, C.nByteLength, C.hmac);
      return drbg(seed, k2sig);
    }
    $(sign);
    Point.BASE._setWindowSize(8);
    function verify(signature, msgHash, publicKey, opts = defaultVerOpts) {
      const sg = signature;
      msgHash = ensureBytes("msgHash", msgHash);
      publicKey = ensureBytes("publicKey", publicKey);
      if ("strict" in opts) throw new Error("options.strict was renamed to lowS");
      const {
        lowS: lowS,
        prehash: prehash
      } = opts;
      let _sig = undefined;
      let P;
      try {
        if (typeof sg === "string" || isBytes$2(sg)) {
          try {
            _sig = Signature.fromDER(sg);
          } catch (derError) {
            if (!(derError instanceof DER.Err)) throw derError;
            _sig = Signature.fromCompact(sg);
          }
        } else if (typeof sg === "object" && typeof sg.r === "bigint" && typeof sg.s === "bigint") {
          const {
            r: r,
            s: s
          } = sg;
          _sig = new Signature(r, s);
        } else {
          throw new Error("PARSE");
        }
        P = Point.fromHex(publicKey);
      } catch (error) {
        if (error.message === "PARSE") throw new Error(`signature must be Signature instance, Uint8Array or hex string`);
        return false;
      }
      if (lowS && _sig.hasHighS()) return false;
      if (prehash) msgHash = CURVE.hash(msgHash);
      const {
        r: r,
        s: s
      } = _sig;
      const h = bits2int_modN(msgHash);
      const is = invN(s);
      const u1 = modN(h * is);
      const u2 = modN(r * is);
      const R = Point.BASE.multiplyAndAddUnsafe(P, u1, u2)?.toAffine();
      if (!R) return false;
      const v = modN(R.x);
      return v === r;
    }
    $(verify);
    return $(function () {
      let result = $Object.create(null, undefined);
      result.CURVE = CURVE;
      result.getPublicKey = getPublicKey;
      result.getSharedSecret = getSharedSecret;
      result.sign = sign;
      result.verify = verify;
      result.ProjectivePoint = Point;
      result.Signature = Signature;
      result.utils = utils;
      return result;
    })();
  }
  $(weierstrass);
  class HMAC extends Hash$1 {
    constructor(hash$1, _key) {
      super();
      this.finished = false;
      this.destroyed = false;
      hash(hash$1);
      const key = toBytes$1(_key);
      this.iHash = hash$1.create();
      if (typeof this.iHash.update !== "function") throw new Error("Expected instance of class which extends utils.Hash");
      this.blockLen = this.iHash.blockLen;
      this.outputLen = this.iHash.outputLen;
      const blockLen = this.blockLen;
      const pad = new Uint8Array(blockLen);
      pad.set(key.length > blockLen ? hash$1.create().update(key).digest() : key);
      for (let i = 0; i < pad.length; i++) pad[i] ^= 54;
      this.iHash.update(pad);
      this.oHash = hash$1.create();
      for (let i = 0; i < pad.length; i++) pad[i] ^= 54 ^ 92;
      this.oHash.update(pad);
      pad.fill(0);
    }
    update(buf) {
      exists$1(this);
      this.iHash.update(buf);
      return this;
    }
    digestInto(out) {
      exists$1(this);
      bytes$1(out, this.outputLen);
      this.finished = true;
      this.iHash.digestInto(out);
      this.oHash.update(out);
      this.oHash.digestInto(out);
      this.destroy();
    }
    digest() {
      const out = new Uint8Array(this.oHash.outputLen);
      this.digestInto(out);
      return out;
    }
    _cloneInto(to) {
      to || (to = $Object.create($Object.getPrototypeOf(this), $Object.create(null, undefined)));
      const {
        oHash: oHash,
        iHash: iHash,
        finished: finished,
        destroyed: destroyed,
        blockLen: blockLen,
        outputLen: outputLen
      } = this;
      to = to;
      to.finished = finished;
      to.destroyed = destroyed;
      to.blockLen = blockLen;
      to.outputLen = outputLen;
      to.oHash = oHash._cloneInto(to.oHash);
      to.iHash = iHash._cloneInto(to.iHash);
      return to;
    }
    destroy() {
      this.destroyed = true;
      this.oHash.destroy();
      this.iHash.destroy();
    }
  }
  const hmac = (hash, key, message) => new HMAC(hash, key).update(message).digest();
  $(hmac);
  hmac.create = $((hash, key) => new HMAC(hash, key))
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */;
  function getHash(hash) {
    return $(function () {
      let result = $Object.create(null, undefined);
      result.hash = hash;
      result.hmac = $((key, ...msgs) => hmac(hash, key, concatBytes$1(...msgs)));
      result.randomBytes = randomBytes;
      return result;
    })();
  }
  $(getHash);
  function createCurve(curveDef, defHash) {
    const create = hash => weierstrass($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, curveDef);
      $Object.assign(result, getHash(hash));
      return result;
    })());
    $(create);
    return $Object.freeze($(function () {
      let result = $Object.create(null, undefined);
      $Object.assign(result, create(defHash));
      result.create = create;
      return result;
    })());
  }
  /*! noble-curves - MIT License (c) 2022 Paul Miller (paulmillr.com) */
  $(createCurve);
  const secp256k1P = BigInt("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f");
  const secp256k1N = BigInt("0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
  const _1n = BigInt(1);
  const _2n = BigInt(2);
  const divNearest = (a, b) => (a + b / _2n) / b;
  $(divNearest);
  function sqrtMod(y) {
    const P = secp256k1P;
    const _3n = BigInt(3),
      _6n = BigInt(6),
      _11n = BigInt(11),
      _22n = BigInt(22);
    const _23n = BigInt(23),
      _44n = BigInt(44),
      _88n = BigInt(88);
    const b2 = y * y * y % P;
    const b3 = b2 * b2 * y % P;
    const b6 = pow2(b3, _3n, P) * b3 % P;
    const b9 = pow2(b6, _3n, P) * b3 % P;
    const b11 = pow2(b9, _2n, P) * b2 % P;
    const b22 = pow2(b11, _11n, P) * b11 % P;
    const b44 = pow2(b22, _22n, P) * b22 % P;
    const b88 = pow2(b44, _44n, P) * b44 % P;
    const b176 = pow2(b88, _88n, P) * b88 % P;
    const b220 = pow2(b176, _44n, P) * b44 % P;
    const b223 = pow2(b220, _3n, P) * b3 % P;
    const t1 = pow2(b223, _23n, P) * b22 % P;
    const t2 = pow2(t1, _6n, P) * b2 % P;
    const root = pow2(t2, _2n, P);
    if (!Fp.eql(Fp.sqr(root), y)) throw new Error("Cannot find square root");
    return root;
  }
  $(sqrtMod);
  const Fp = Field(secp256k1P, undefined, undefined, $(function () {
    let result = $Object.create(null, undefined);
    result.sqrt = sqrtMod;
    return result;
  })());
  const secp256k1 = createCurve($(function () {
    let result = $Object.create(null, undefined);
    result.a = BigInt(0);
    result.b = BigInt(7);
    result.Fp = Fp;
    result.n = secp256k1N;
    result.Gx = BigInt("55066263022277343669578718895168534326250603453777594175500187360389116729240");
    result.Gy = BigInt("32670510020758816978083085130507043184471273380659243275938904335757337482424");
    result.h = BigInt(1);
    result.lowS = true;
    result.endo = $(function () {
      let result = $Object.create(null, undefined);
      result.beta = BigInt("0x7ae96a2b657c07106e64479eac3434e99cf0497512f58995c1396c28719501ee");
      result.splitScalar = $(k => {
        const n = secp256k1N;
        const a1 = BigInt("0x3086d221a7d46bcde86c90e49284eb15");
        const b1 = -_1n * BigInt("0xe4437ed6010e88286f547fa90abfe4c3");
        const a2 = BigInt("0x114ca50f7a8e2f3f657c1108d9d44cfd8");
        const b2 = a1;
        const POW_2_128 = BigInt("0x100000000000000000000000000000000");
        const c1 = divNearest(b2 * k, n);
        const c2 = divNearest(-b1 * k, n);
        let k1 = mod(k - c1 * a1 - c2 * a2, n);
        let k2 = mod(-c1 * b1 - c2 * b2, n);
        const k1neg = k1 > POW_2_128;
        const k2neg = k2 > POW_2_128;
        if (k1neg) k1 = n - k1;
        if (k2neg) k2 = n - k2;
        if (k1 > POW_2_128 || k2 > POW_2_128) {
          throw new Error("splitScalar: Endomorphism failed, k=" + k);
        }
        return $(function () {
          let result = $Object.create(null, undefined);
          result.k1neg = k1neg;
          result.k1 = k1;
          result.k2neg = k2neg;
          result.k2 = k2;
          return result;
        })();
      });
      return result;
    })();
    return result;
  })(), sha256);
  BigInt(0);
  secp256k1.ProjectivePoint;
  const ecdsaSign = (msgHash, privKey) => {
    const signature = secp256k1.sign(msgHash, privKey);
    return $Array.of(signature.toCompactRawBytes(), signature.recovery);
  };
  $(ecdsaSign);
  secp256k1.utils.isValidPrivateKey;
  const publicKeyCreate = secp256k1.getPublicKey;
  const PRIVATE_KEY_BYTES = 32;
  const ETHEREUM_ADDRESS_BYTES = 20;
  const PUBLIC_KEY_BYTES = 64;
  const SIGNATURE_OFFSETS_SERIALIZED_SIZE = 11;
  const SECP256K1_INSTRUCTION_LAYOUT = struct($Array.of(u8("numSignatures"), u16("signatureOffset"), u8("signatureInstructionIndex"), u16("ethAddressOffset"), u8("ethAddressInstructionIndex"), u16("messageDataOffset"), u16("messageDataSize"), u8("messageInstructionIndex"), blob(20, "ethAddress"), blob(64, "signature"), u8("recoveryId")));
  class Secp256k1Program {
    constructor() {}
    static publicKeyToEthAddress(publicKey) {
      assert$1(publicKey.length === PUBLIC_KEY_BYTES, `Public key must be ${PUBLIC_KEY_BYTES} bytes but received ${publicKey.length} bytes`);
      try {
        return buffer.Buffer.from(keccak_256(toBuffer(publicKey))).slice(-ETHEREUM_ADDRESS_BYTES);
      } catch (error) {
        throw new Error(`Error constructing Ethereum address: ${error}`);
      }
    }
    static createInstructionWithPublicKey(params) {
      const {
        publicKey: publicKey,
        message: message,
        signature: signature,
        recoveryId: recoveryId,
        instructionIndex: instructionIndex
      } = params;
      return Secp256k1Program.createInstructionWithEthAddress($(function () {
        let result = $Object.create(null, undefined);
        result.ethAddress = Secp256k1Program.publicKeyToEthAddress(publicKey);
        result.message = message;
        result.signature = signature;
        result.recoveryId = recoveryId;
        result.instructionIndex = instructionIndex;
        return result;
      })());
    }
    static createInstructionWithEthAddress(params) {
      const {
        ethAddress: rawAddress,
        message: message,
        signature: signature,
        recoveryId: recoveryId,
        instructionIndex = 0
      } = params;
      let ethAddress;
      if (typeof rawAddress === "string") {
        if (rawAddress.startsWith("0x")) {
          ethAddress = buffer.Buffer.from(rawAddress.substr(2), "hex");
        } else {
          ethAddress = buffer.Buffer.from(rawAddress, "hex");
        }
      } else {
        ethAddress = rawAddress;
      }
      assert$1(ethAddress.length === ETHEREUM_ADDRESS_BYTES, `Address must be ${ETHEREUM_ADDRESS_BYTES} bytes but received ${ethAddress.length} bytes`);
      const dataStart = 1 + SIGNATURE_OFFSETS_SERIALIZED_SIZE;
      const ethAddressOffset = dataStart;
      const signatureOffset = dataStart + ethAddress.length;
      const messageDataOffset = signatureOffset + signature.length + 1;
      const numSignatures = 1;
      const instructionData = buffer.Buffer.alloc(SECP256K1_INSTRUCTION_LAYOUT.span + message.length);
      SECP256K1_INSTRUCTION_LAYOUT.encode($(function () {
        let result = $Object.create(null, undefined);
        result.numSignatures = numSignatures;
        result.signatureOffset = signatureOffset;
        result.signatureInstructionIndex = instructionIndex;
        result.ethAddressOffset = ethAddressOffset;
        result.ethAddressInstructionIndex = instructionIndex;
        result.messageDataOffset = messageDataOffset;
        result.messageDataSize = message.length;
        result.messageInstructionIndex = instructionIndex;
        result.signature = toBuffer(signature);
        result.ethAddress = toBuffer(ethAddress);
        result.recoveryId = recoveryId;
        return result;
      })(), instructionData);
      instructionData.fill(toBuffer(message), SECP256K1_INSTRUCTION_LAYOUT.span);
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of();
        result.programId = Secp256k1Program.programId;
        result.data = instructionData;
        return result;
      })());
    }
    static createInstructionWithPrivateKey(params) {
      const {
        privateKey: pkey,
        message: message,
        instructionIndex: instructionIndex
      } = params;
      assert$1(pkey.length === PRIVATE_KEY_BYTES, `Private key must be ${PRIVATE_KEY_BYTES} bytes but received ${pkey.length} bytes`);
      try {
        const privateKey = toBuffer(pkey);
        const publicKey = publicKeyCreate(privateKey, false).slice(1);
        const messageHash = buffer.Buffer.from(keccak_256(toBuffer(message)));
        const [signature, recoveryId] = ecdsaSign(messageHash, privateKey);
        return this.createInstructionWithPublicKey($(function () {
          let result = $Object.create(null, undefined);
          result.publicKey = publicKey;
          result.message = message;
          result.signature = signature;
          result.recoveryId = recoveryId;
          result.instructionIndex = instructionIndex;
          return result;
        })());
      } catch (error) {
        throw new Error(`Error creating instruction; ${error}`);
      }
    }
  }
  Secp256k1Program.programId = new PublicKey("KeccakSecp256k11111111111111111111111111111");
  var _class2;
  const STAKE_CONFIG_ID = new PublicKey("StakeConfig11111111111111111111111111111111");
  class Authorized {
    constructor(staker, withdrawer) {
      this.staker = void 0;
      this.withdrawer = void 0;
      this.staker = staker;
      this.withdrawer = withdrawer;
    }
  }
  class Lockup {
    constructor(unixTimestamp, epoch, custodian) {
      this.unixTimestamp = void 0;
      this.epoch = void 0;
      this.custodian = void 0;
      this.unixTimestamp = unixTimestamp;
      this.epoch = epoch;
      this.custodian = custodian;
    }
  }
  _class2 = Lockup;
  Lockup.default = new _class2(0, 0, PublicKey.default);
  class StakeInstruction {
    constructor() {}
    static decodeInstructionType(instruction) {
      this.checkProgramId(instruction.programId);
      const instructionTypeLayout = u32("instruction");
      const typeIndex = instructionTypeLayout.decode(instruction.data);
      let type;
      for (const [ixType, layout] of $Object.entries(STAKE_INSTRUCTION_LAYOUTS)) {
        if (layout.index == typeIndex) {
          type = ixType;
          break;
        }
      }
      if (!type) {
        throw new Error("Instruction type incorrect; not a StakeInstruction");
      }
      return type;
    }
    static decodeInitialize(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        authorized: authorized,
        lockup: lockup
      } = decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Initialize, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.authorized = new Authorized(new PublicKey(authorized.staker), new PublicKey(authorized.withdrawer));
        result.lockup = new Lockup(lockup.unixTimestamp, lockup.epoch, new PublicKey(lockup.custodian));
        return result;
      })();
    }
    static decodeDelegate(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 6);
      decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Delegate, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.votePubkey = instruction.keys[1].pubkey;
        result.authorizedPubkey = instruction.keys[5].pubkey;
        return result;
      })();
    }
    static decodeAuthorize(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        newAuthorized: newAuthorized,
        stakeAuthorizationType: stakeAuthorizationType
      } = decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Authorize, instruction.data);
      const o = $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = instruction.keys[2].pubkey;
        result.newAuthorizedPubkey = new PublicKey(newAuthorized);
        result.stakeAuthorizationType = $(function () {
          let result = $Object.create(null, undefined);
          result.index = stakeAuthorizationType;
          return result;
        })();
        return result;
      })();
      if (instruction.keys.length > 3) {
        o.custodianPubkey = instruction.keys[3].pubkey;
      }
      return o;
    }
    static decodeAuthorizeWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 2);
      const {
        newAuthorized: newAuthorized,
        stakeAuthorizationType: stakeAuthorizationType,
        authoritySeed: authoritySeed,
        authorityOwner: authorityOwner
      } = decodeData$1(STAKE_INSTRUCTION_LAYOUTS.AuthorizeWithSeed, instruction.data);
      const o = $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.authorityBase = instruction.keys[1].pubkey;
        result.authoritySeed = authoritySeed;
        result.authorityOwner = new PublicKey(authorityOwner);
        result.newAuthorizedPubkey = new PublicKey(newAuthorized);
        result.stakeAuthorizationType = $(function () {
          let result = $Object.create(null, undefined);
          result.index = stakeAuthorizationType;
          return result;
        })();
        return result;
      })();
      if (instruction.keys.length > 3) {
        o.custodianPubkey = instruction.keys[3].pubkey;
      }
      return o;
    }
    static decodeSplit(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        lamports: lamports
      } = decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Split, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.splitStakePubkey = instruction.keys[1].pubkey;
        result.authorizedPubkey = instruction.keys[2].pubkey;
        result.lamports = lamports;
        return result;
      })();
    }
    static decodeMerge(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Merge, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.sourceStakePubKey = instruction.keys[1].pubkey;
        result.authorizedPubkey = instruction.keys[4].pubkey;
        return result;
      })();
    }
    static decodeWithdraw(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 5);
      const {
        lamports: lamports
      } = decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Withdraw, instruction.data);
      const o = $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.toPubkey = instruction.keys[1].pubkey;
        result.authorizedPubkey = instruction.keys[4].pubkey;
        result.lamports = lamports;
        return result;
      })();
      if (instruction.keys.length > 5) {
        o.custodianPubkey = instruction.keys[5].pubkey;
      }
      return o;
    }
    static decodeDeactivate(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      decodeData$1(STAKE_INSTRUCTION_LAYOUTS.Deactivate, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = instruction.keys[2].pubkey;
        return result;
      })();
    }
    static checkProgramId(programId) {
      if (!programId.equals(StakeProgram.programId)) {
        throw new Error("invalid instruction; programId is not StakeProgram");
      }
    }
    static checkKeyLength(keys, expectedLength) {
      if (keys.length < expectedLength) {
        throw new Error(`invalid instruction; found ${keys.length} keys, expected at least ${expectedLength}`);
      }
    }
  }
  const STAKE_INSTRUCTION_LAYOUTS = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.Initialize = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      result.layout = struct($Array.of(u32("instruction"), authorized(), lockup()));
      return result;
    })();
    result.Authorize = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      result.layout = struct($Array.of(u32("instruction"), publicKey("newAuthorized"), u32("stakeAuthorizationType")));
      return result;
    })();
    result.Delegate = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 2;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.Split = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 3;
      result.layout = struct($Array.of(u32("instruction"), ns64("lamports")));
      return result;
    })();
    result.Withdraw = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 4;
      result.layout = struct($Array.of(u32("instruction"), ns64("lamports")));
      return result;
    })();
    result.Deactivate = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 5;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.Merge = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 7;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.AuthorizeWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 8;
      result.layout = struct($Array.of(u32("instruction"), publicKey("newAuthorized"), u32("stakeAuthorizationType"), rustString("authoritySeed"), publicKey("authorityOwner")));
      return result;
    })();
    return result;
  })());
  const StakeAuthorizationLayout = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.Staker = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      return result;
    })();
    result.Withdrawer = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      return result;
    })();
    return result;
  })());
  class StakeProgram {
    constructor() {}
    static initialize(params) {
      const {
        stakePubkey: stakePubkey,
        authorized: authorized,
        lockup: maybeLockup
      } = params;
      const lockup = maybeLockup || Lockup.default;
      const type = STAKE_INSTRUCTION_LAYOUTS.Initialize;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.authorized = $(function () {
          let result = $Object.create(null, undefined);
          result.staker = toBuffer(authorized.staker.toBuffer());
          result.withdrawer = toBuffer(authorized.withdrawer.toBuffer());
          return result;
        })();
        result.lockup = $(function () {
          let result = $Object.create(null, undefined);
          result.unixTimestamp = lockup.unixTimestamp;
          result.epoch = lockup.epoch;
          result.custodian = toBuffer(lockup.custodian.toBuffer());
          return result;
        })();
        return result;
      })());
      const instructionData = $(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = stakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RENT_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)();
      return new TransactionInstruction(instructionData);
    }
    static createAccountWithSeed(params) {
      const transaction = new Transaction();
      transaction.add(SystemProgram.createAccountWithSeed($(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = params.fromPubkey;
        result.newAccountPubkey = params.stakePubkey;
        result.basePubkey = params.basePubkey;
        result.seed = params.seed;
        result.lamports = params.lamports;
        result.space = this.space;
        result.programId = this.programId;
        return result;
      }).bind(this)()));
      const {
        stakePubkey: stakePubkey,
        authorized: authorized,
        lockup: lockup
      } = params;
      return transaction.add(this.initialize($(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = stakePubkey;
        result.authorized = authorized;
        result.lockup = lockup;
        return result;
      })()));
    }
    static createAccount(params) {
      const transaction = new Transaction();
      transaction.add(SystemProgram.createAccount($(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = params.fromPubkey;
        result.newAccountPubkey = params.stakePubkey;
        result.lamports = params.lamports;
        result.space = this.space;
        result.programId = this.programId;
        return result;
      }).bind(this)()));
      const {
        stakePubkey: stakePubkey,
        authorized: authorized,
        lockup: lockup
      } = params;
      return transaction.add(this.initialize($(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = stakePubkey;
        result.authorized = authorized;
        result.lockup = lockup;
        return result;
      })()));
    }
    static delegate(params) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey,
        votePubkey: votePubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Delegate;
      const data = encodeData(type);
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = stakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = votePubkey;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_CLOCK_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_STAKE_HISTORY_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = STAKE_CONFIG_ID;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static authorize(params) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey,
        newAuthorizedPubkey: newAuthorizedPubkey,
        stakeAuthorizationType: stakeAuthorizationType,
        custodianPubkey: custodianPubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Authorize;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.newAuthorized = toBuffer(newAuthorizedPubkey.toBuffer());
        result.stakeAuthorizationType = stakeAuthorizationType.index;
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = stakePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_CLOCK_PUBKEY;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorizedPubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      if (custodianPubkey) {
        keys.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = custodianPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      }
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static authorizeWithSeed(params) {
      const {
        stakePubkey: stakePubkey,
        authorityBase: authorityBase,
        authoritySeed: authoritySeed,
        authorityOwner: authorityOwner,
        newAuthorizedPubkey: newAuthorizedPubkey,
        stakeAuthorizationType: stakeAuthorizationType,
        custodianPubkey: custodianPubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.AuthorizeWithSeed;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.newAuthorized = toBuffer(newAuthorizedPubkey.toBuffer());
        result.stakeAuthorizationType = stakeAuthorizationType.index;
        result.authoritySeed = authoritySeed;
        result.authorityOwner = toBuffer(authorityOwner.toBuffer());
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = stakePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorityBase;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_CLOCK_PUBKEY;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })());
      if (custodianPubkey) {
        keys.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = custodianPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      }
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static splitInstruction(params) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey,
        splitStakePubkey: splitStakePubkey,
        lamports: lamports
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Split;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.lamports = lamports;
        return result;
      })());
      return new TransactionInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = stakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = splitStakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static split(params, rentExemptReserve) {
      const transaction = new Transaction();
      transaction.add(SystemProgram.createAccount($(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = params.authorizedPubkey;
        result.newAccountPubkey = params.splitStakePubkey;
        result.lamports = rentExemptReserve;
        result.space = this.space;
        result.programId = this.programId;
        return result;
      }).bind(this)()));
      return transaction.add(this.splitInstruction(params));
    }
    static splitWithSeed(params, rentExemptReserve) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey,
        splitStakePubkey: splitStakePubkey,
        basePubkey: basePubkey,
        seed: seed,
        lamports: lamports
      } = params;
      const transaction = new Transaction();
      transaction.add(SystemProgram.allocate($(function () {
        let result = $Object.create(null, undefined);
        result.accountPubkey = splitStakePubkey;
        result.basePubkey = basePubkey;
        result.seed = seed;
        result.space = this.space;
        result.programId = this.programId;
        return result;
      }).bind(this)()));
      if (rentExemptReserve && rentExemptReserve > 0) {
        transaction.add(SystemProgram.transfer($(function () {
          let result = $Object.create(null, undefined);
          result.fromPubkey = params.authorizedPubkey;
          result.toPubkey = splitStakePubkey;
          result.lamports = rentExemptReserve;
          return result;
        })()));
      }
      return transaction.add(this.splitInstruction($(function () {
        let result = $Object.create(null, undefined);
        result.stakePubkey = stakePubkey;
        result.authorizedPubkey = authorizedPubkey;
        result.splitStakePubkey = splitStakePubkey;
        result.lamports = lamports;
        return result;
      })()));
    }
    static merge(params) {
      const {
        stakePubkey: stakePubkey,
        sourceStakePubKey: sourceStakePubKey,
        authorizedPubkey: authorizedPubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Merge;
      const data = encodeData(type);
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = stakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = sourceStakePubKey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_CLOCK_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_STAKE_HISTORY_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static withdraw(params) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey,
        toPubkey: toPubkey,
        lamports: lamports,
        custodianPubkey: custodianPubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Withdraw;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.lamports = lamports;
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = stakePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = toPubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_CLOCK_PUBKEY;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_STAKE_HISTORY_PUBKEY;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorizedPubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      if (custodianPubkey) {
        keys.push($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = custodianPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
      }
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static deactivate(params) {
      const {
        stakePubkey: stakePubkey,
        authorizedPubkey: authorizedPubkey
      } = params;
      const type = STAKE_INSTRUCTION_LAYOUTS.Deactivate;
      const data = encodeData(type);
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = stakePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_CLOCK_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = authorizedPubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
  }
  StakeProgram.programId = new PublicKey("Stake11111111111111111111111111111111111111");
  StakeProgram.space = 200;
  class VoteInit {
    constructor(nodePubkey, authorizedVoter, authorizedWithdrawer, commission) {
      this.nodePubkey = void 0;
      this.authorizedVoter = void 0;
      this.authorizedWithdrawer = void 0;
      this.commission = void 0;
      this.nodePubkey = nodePubkey;
      this.authorizedVoter = authorizedVoter;
      this.authorizedWithdrawer = authorizedWithdrawer;
      this.commission = commission;
    }
  }
  class VoteInstruction {
    constructor() {}
    static decodeInstructionType(instruction) {
      this.checkProgramId(instruction.programId);
      const instructionTypeLayout = u32("instruction");
      const typeIndex = instructionTypeLayout.decode(instruction.data);
      let type;
      for (const [ixType, layout] of $Object.entries(VOTE_INSTRUCTION_LAYOUTS)) {
        if (layout.index == typeIndex) {
          type = ixType;
          break;
        }
      }
      if (!type) {
        throw new Error("Instruction type incorrect; not a VoteInstruction");
      }
      return type;
    }
    static decodeInitializeAccount(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 4);
      const {
        voteInit: voteInit
      } = decodeData$1(VOTE_INSTRUCTION_LAYOUTS.InitializeAccount, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.votePubkey = instruction.keys[0].pubkey;
        result.nodePubkey = instruction.keys[3].pubkey;
        result.voteInit = new VoteInit(new PublicKey(voteInit.nodePubkey), new PublicKey(voteInit.authorizedVoter), new PublicKey(voteInit.authorizedWithdrawer), voteInit.commission);
        return result;
      })();
    }
    static decodeAuthorize(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        newAuthorized: newAuthorized,
        voteAuthorizationType: voteAuthorizationType
      } = decodeData$1(VOTE_INSTRUCTION_LAYOUTS.Authorize, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.votePubkey = instruction.keys[0].pubkey;
        result.authorizedPubkey = instruction.keys[2].pubkey;
        result.newAuthorizedPubkey = new PublicKey(newAuthorized);
        result.voteAuthorizationType = $(function () {
          let result = $Object.create(null, undefined);
          result.index = voteAuthorizationType;
          return result;
        })();
        return result;
      })();
    }
    static decodeAuthorizeWithSeed(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        voteAuthorizeWithSeedArgs: {
          currentAuthorityDerivedKeyOwnerPubkey: currentAuthorityDerivedKeyOwnerPubkey,
          currentAuthorityDerivedKeySeed: currentAuthorityDerivedKeySeed,
          newAuthorized: newAuthorized,
          voteAuthorizationType: voteAuthorizationType
        }
      } = decodeData$1(VOTE_INSTRUCTION_LAYOUTS.AuthorizeWithSeed, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.currentAuthorityDerivedKeyBasePubkey = instruction.keys[2].pubkey;
        result.currentAuthorityDerivedKeyOwnerPubkey = new PublicKey(currentAuthorityDerivedKeyOwnerPubkey);
        result.currentAuthorityDerivedKeySeed = currentAuthorityDerivedKeySeed;
        result.newAuthorizedPubkey = new PublicKey(newAuthorized);
        result.voteAuthorizationType = $(function () {
          let result = $Object.create(null, undefined);
          result.index = voteAuthorizationType;
          return result;
        })();
        result.votePubkey = instruction.keys[0].pubkey;
        return result;
      })();
    }
    static decodeWithdraw(instruction) {
      this.checkProgramId(instruction.programId);
      this.checkKeyLength(instruction.keys, 3);
      const {
        lamports: lamports
      } = decodeData$1(VOTE_INSTRUCTION_LAYOUTS.Withdraw, instruction.data);
      return $(function () {
        let result = $Object.create(null, undefined);
        result.votePubkey = instruction.keys[0].pubkey;
        result.authorizedWithdrawerPubkey = instruction.keys[2].pubkey;
        result.lamports = lamports;
        result.toPubkey = instruction.keys[1].pubkey;
        return result;
      })();
    }
    static checkProgramId(programId) {
      if (!programId.equals(VoteProgram.programId)) {
        throw new Error("invalid instruction; programId is not VoteProgram");
      }
    }
    static checkKeyLength(keys, expectedLength) {
      if (keys.length < expectedLength) {
        throw new Error(`invalid instruction; found ${keys.length} keys, expected at least ${expectedLength}`);
      }
    }
  }
  const VOTE_INSTRUCTION_LAYOUTS = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.InitializeAccount = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      result.layout = struct($Array.of(u32("instruction"), voteInit()));
      return result;
    })();
    result.Authorize = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      result.layout = struct($Array.of(u32("instruction"), publicKey("newAuthorized"), u32("voteAuthorizationType")));
      return result;
    })();
    result.Withdraw = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 3;
      result.layout = struct($Array.of(u32("instruction"), ns64("lamports")));
      return result;
    })();
    result.UpdateValidatorIdentity = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 4;
      result.layout = struct($Array.of(u32("instruction")));
      return result;
    })();
    result.AuthorizeWithSeed = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 10;
      result.layout = struct($Array.of(u32("instruction"), voteAuthorizeWithSeedArgs()));
      return result;
    })();
    return result;
  })());
  const VoteAuthorizationLayout = $Object.freeze($(function () {
    let result = $Object.create(null, undefined);
    result.Voter = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 0;
      return result;
    })();
    result.Withdrawer = $(function () {
      let result = $Object.create(null, undefined);
      result.index = 1;
      return result;
    })();
    return result;
  })());
  class VoteProgram {
    constructor() {}
    static initializeAccount(params) {
      const {
        votePubkey: votePubkey,
        nodePubkey: nodePubkey,
        voteInit: voteInit
      } = params;
      const type = VOTE_INSTRUCTION_LAYOUTS.InitializeAccount;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.voteInit = $(function () {
          let result = $Object.create(null, undefined);
          result.nodePubkey = toBuffer(voteInit.nodePubkey.toBuffer());
          result.authorizedVoter = toBuffer(voteInit.authorizedVoter.toBuffer());
          result.authorizedWithdrawer = toBuffer(voteInit.authorizedWithdrawer.toBuffer());
          result.commission = voteInit.commission;
          return result;
        })();
        return result;
      })());
      const instructionData = $(function () {
        let result = $Object.create(null, undefined);
        result.keys = $Array.of($(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = votePubkey;
          result.isSigner = false;
          result.isWritable = true;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_RENT_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = SYSVAR_CLOCK_PUBKEY;
          result.isSigner = false;
          result.isWritable = false;
          return result;
        })(), $(function () {
          let result = $Object.create(null, undefined);
          result.pubkey = nodePubkey;
          result.isSigner = true;
          result.isWritable = false;
          return result;
        })());
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)();
      return new TransactionInstruction(instructionData);
    }
    static createAccount(params) {
      const transaction = new Transaction();
      transaction.add(SystemProgram.createAccount($(function () {
        let result = $Object.create(null, undefined);
        result.fromPubkey = params.fromPubkey;
        result.newAccountPubkey = params.votePubkey;
        result.lamports = params.lamports;
        result.space = this.space;
        result.programId = this.programId;
        return result;
      }).bind(this)()));
      return transaction.add(this.initializeAccount($(function () {
        let result = $Object.create(null, undefined);
        result.votePubkey = params.votePubkey;
        result.nodePubkey = params.voteInit.nodePubkey;
        result.voteInit = params.voteInit;
        return result;
      })()));
    }
    static authorize(params) {
      const {
        votePubkey: votePubkey,
        authorizedPubkey: authorizedPubkey,
        newAuthorizedPubkey: newAuthorizedPubkey,
        voteAuthorizationType: voteAuthorizationType
      } = params;
      const type = VOTE_INSTRUCTION_LAYOUTS.Authorize;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.newAuthorized = toBuffer(newAuthorizedPubkey.toBuffer());
        result.voteAuthorizationType = voteAuthorizationType.index;
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = votePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_CLOCK_PUBKEY;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorizedPubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static authorizeWithSeed(params) {
      const {
        currentAuthorityDerivedKeyBasePubkey: currentAuthorityDerivedKeyBasePubkey,
        currentAuthorityDerivedKeyOwnerPubkey: currentAuthorityDerivedKeyOwnerPubkey,
        currentAuthorityDerivedKeySeed: currentAuthorityDerivedKeySeed,
        newAuthorizedPubkey: newAuthorizedPubkey,
        voteAuthorizationType: voteAuthorizationType,
        votePubkey: votePubkey
      } = params;
      const type = VOTE_INSTRUCTION_LAYOUTS.AuthorizeWithSeed;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.voteAuthorizeWithSeedArgs = $(function () {
          let result = $Object.create(null, undefined);
          result.currentAuthorityDerivedKeyOwnerPubkey = toBuffer(currentAuthorityDerivedKeyOwnerPubkey.toBuffer());
          result.currentAuthorityDerivedKeySeed = currentAuthorityDerivedKeySeed;
          result.newAuthorized = toBuffer(newAuthorizedPubkey.toBuffer());
          result.voteAuthorizationType = voteAuthorizationType.index;
          return result;
        })();
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = votePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = SYSVAR_CLOCK_PUBKEY;
        result.isSigner = false;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = currentAuthorityDerivedKeyBasePubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static withdraw(params) {
      const {
        votePubkey: votePubkey,
        authorizedWithdrawerPubkey: authorizedWithdrawerPubkey,
        lamports: lamports,
        toPubkey: toPubkey
      } = params;
      const type = VOTE_INSTRUCTION_LAYOUTS.Withdraw;
      const data = encodeData(type, $(function () {
        let result = $Object.create(null, undefined);
        result.lamports = lamports;
        return result;
      })());
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = votePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = toPubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorizedWithdrawerPubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
    static safeWithdraw(params, currentVoteAccountBalance, rentExemptMinimum) {
      if (params.lamports > currentVoteAccountBalance - rentExemptMinimum) {
        throw new Error("Withdraw will leave vote account with insufficient funds.");
      }
      return VoteProgram.withdraw(params);
    }
    static updateValidatorIdentity(params) {
      const {
        votePubkey: votePubkey,
        authorizedWithdrawerPubkey: authorizedWithdrawerPubkey,
        nodePubkey: nodePubkey
      } = params;
      const type = VOTE_INSTRUCTION_LAYOUTS.UpdateValidatorIdentity;
      const data = encodeData(type);
      const keys = $Array.of($(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = votePubkey;
        result.isSigner = false;
        result.isWritable = true;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = nodePubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })(), $(function () {
        let result = $Object.create(null, undefined);
        result.pubkey = authorizedWithdrawerPubkey;
        result.isSigner = true;
        result.isWritable = false;
        return result;
      })());
      return new Transaction().add($(function () {
        let result = $Object.create(null, undefined);
        result.keys = keys;
        result.programId = this.programId;
        result.data = data;
        return result;
      }).bind(this)());
    }
  }
  VoteProgram.programId = new PublicKey("Vote111111111111111111111111111111111111111");
  VoteProgram.space = 3762;
  const VALIDATOR_INFO_KEY = new PublicKey("Va1idator1nfo111111111111111111111111111111");
  const InfoString = type($(function () {
    let result = $Object.create(null, undefined);
    result.name = string();
    result.website = optional(string());
    result.details = optional(string());
    result.keybaseUsername = optional(string());
    return result;
  })());
  class ValidatorInfo {
    constructor(key, info) {
      this.key = void 0;
      this.info = void 0;
      this.key = key;
      this.info = info;
    }
    static fromConfigData(buffer$1) {
      let byteArray = $Array.of(...buffer$1);
      const configKeyCount = decodeLength(byteArray);
      if (configKeyCount !== 2) return null;
      const configKeys = $Array.of();
      for (let i = 0; i < 2; i++) {
        const publicKey = new PublicKey(guardedSplice(byteArray, 0, PUBLIC_KEY_LENGTH));
        const isSigner = guardedShift(byteArray) === 1;
        configKeys.push($(function () {
          let result = $Object.create(null, undefined);
          result.publicKey = publicKey;
          result.isSigner = isSigner;
          return result;
        })());
      }
      if (configKeys[0].publicKey.equals(VALIDATOR_INFO_KEY)) {
        if (configKeys[1].isSigner) {
          const rawInfo = rustString().decode(buffer.Buffer.from(byteArray));
          const info = JSON.parse(rawInfo);
          assert(info, InfoString);
          return new ValidatorInfo(configKeys[1].publicKey, info);
        }
      }
      return null;
    }
  }
  const VOTE_PROGRAM_ID = new PublicKey("Vote111111111111111111111111111111111111111");
  const VoteAccountLayout = struct($Array.of(publicKey("nodePubkey"), publicKey("authorizedWithdrawer"), u8("commission"), nu64(), seq(struct($Array.of(nu64("slot"), u32("confirmationCount"))), offset(u32(), -8), "votes"), u8("rootSlotValid"), nu64("rootSlot"), nu64(), seq(struct($Array.of(nu64("epoch"), publicKey("authorizedVoter"))), offset(u32(), -8), "authorizedVoters"), struct($Array.of(seq(struct($Array.of(publicKey("authorizedPubkey"), nu64("epochOfLastAuthorizedSwitch"), nu64("targetEpoch"))), 32, "buf"), nu64("idx"), u8("isEmpty")), "priorVoters"), nu64(), seq(struct($Array.of(nu64("epoch"), nu64("credits"), nu64("prevCredits"))), offset(u32(), -8), "epochCredits"), struct($Array.of(nu64("slot"), nu64("timestamp")), "lastTimestamp")));
  class VoteAccount {
    constructor(args) {
      this.nodePubkey = void 0;
      this.authorizedWithdrawer = void 0;
      this.commission = void 0;
      this.rootSlot = void 0;
      this.votes = void 0;
      this.authorizedVoters = void 0;
      this.priorVoters = void 0;
      this.epochCredits = void 0;
      this.lastTimestamp = void 0;
      this.nodePubkey = args.nodePubkey;
      this.authorizedWithdrawer = args.authorizedWithdrawer;
      this.commission = args.commission;
      this.rootSlot = args.rootSlot;
      this.votes = args.votes;
      this.authorizedVoters = args.authorizedVoters;
      this.priorVoters = args.priorVoters;
      this.epochCredits = args.epochCredits;
      this.lastTimestamp = args.lastTimestamp;
    }
    static fromAccountData(buffer) {
      const versionOffset = 4;
      const va = VoteAccountLayout.decode(toBuffer(buffer), versionOffset);
      let rootSlot = va.rootSlot;
      if (!va.rootSlotValid) {
        rootSlot = null;
      }
      return new VoteAccount($(function () {
        let result = $Object.create(null, undefined);
        result.nodePubkey = new PublicKey(va.nodePubkey);
        result.authorizedWithdrawer = new PublicKey(va.authorizedWithdrawer);
        result.commission = va.commission;
        result.votes = va.votes;
        result.rootSlot = rootSlot;
        result.authorizedVoters = va.authorizedVoters.map(parseAuthorizedVoter);
        result.priorVoters = getPriorVoters(va.priorVoters);
        result.epochCredits = va.epochCredits;
        result.lastTimestamp = va.lastTimestamp;
        return result;
      })());
    }
  }
  function parseAuthorizedVoter({
    authorizedVoter: authorizedVoter,
    epoch: epoch
  }) {
    return $(function () {
      let result = $Object.create(null, undefined);
      result.epoch = epoch;
      result.authorizedVoter = new PublicKey(authorizedVoter);
      return result;
    })();
  }
  $(parseAuthorizedVoter);
  function parsePriorVoters({
    authorizedPubkey: authorizedPubkey,
    epochOfLastAuthorizedSwitch: epochOfLastAuthorizedSwitch,
    targetEpoch: targetEpoch
  }) {
    return $(function () {
      let result = $Object.create(null, undefined);
      result.authorizedPubkey = new PublicKey(authorizedPubkey);
      result.epochOfLastAuthorizedSwitch = epochOfLastAuthorizedSwitch;
      result.targetEpoch = targetEpoch;
      return result;
    })();
  }
  $(parsePriorVoters);
  function getPriorVoters({
    buf: buf,
    idx: idx,
    isEmpty: isEmpty
  }) {
    if (isEmpty) {
      return $Array.of();
    }
    return $Array.of(...buf.slice(idx + 1).map(parsePriorVoters), ...buf.slice(0, idx).map(parsePriorVoters));
  }
  $(getPriorVoters);
  const endpoint = $(function () {
    let result = $Object.create(null, undefined);
    result.http = $(function () {
      let result = $Object.create(null, undefined);
      result.devnet = "http://api.devnet.solana.com";
      result.testnet = "http://api.testnet.solana.com";
      result["mainnet-beta"] = "http://api.mainnet-beta.solana.com/";
      return result;
    })();
    result.https = $(function () {
      let result = $Object.create(null, undefined);
      result.devnet = "https://api.devnet.solana.com";
      result.testnet = "https://api.testnet.solana.com";
      result["mainnet-beta"] = "https://api.mainnet-beta.solana.com/";
      return result;
    })();
    return result;
  })();
  function clusterApiUrl(cluster, tls) {
    const key = tls === false ? "http" : "https";
    if (!cluster) {
      return endpoint[key]["devnet"];
    }
    const url = endpoint[key][cluster];
    if (!url) {
      throw new Error(`Unknown ${key} cluster: ${cluster}`);
    }
    return url;
  }
  $(clusterApiUrl);
  async function sendAndConfirmRawTransaction(connection, rawTransaction, confirmationStrategyOrConfirmOptions, maybeConfirmOptions) {
    let confirmationStrategy;
    let options;
    if (confirmationStrategyOrConfirmOptions && $Object.prototype.hasOwnProperty.call(confirmationStrategyOrConfirmOptions, "lastValidBlockHeight")) {
      confirmationStrategy = confirmationStrategyOrConfirmOptions;
      options = maybeConfirmOptions;
    } else if (confirmationStrategyOrConfirmOptions && $Object.prototype.hasOwnProperty.call(confirmationStrategyOrConfirmOptions, "nonceValue")) {
      confirmationStrategy = confirmationStrategyOrConfirmOptions;
      options = maybeConfirmOptions;
    } else {
      options = confirmationStrategyOrConfirmOptions;
    }
    const sendOptions = options && $(function () {
      let result = $Object.create(null, undefined);
      result.skipPreflight = options.skipPreflight;
      result.preflightCommitment = options.preflightCommitment || options.commitment;
      result.minContextSlot = options.minContextSlot;
      return result;
    })();
    const signature = await connection.sendRawTransaction(rawTransaction, sendOptions);
    const commitment = options && options.commitment;
    const confirmationPromise = confirmationStrategy ? connection.confirmTransaction(confirmationStrategy, commitment) : connection.confirmTransaction(signature, commitment);
    const status = (await confirmationPromise).value;
    if (status.err) {
      throw new Error(`Raw transaction ${signature} failed (${JSON.stringify(status)})`);
    }
    return signature;
  }
  $(sendAndConfirmRawTransaction);
  const LAMPORTS_PER_SOL = 1e9;
  exports.Account = Account;
  exports.AddressLookupTableAccount = AddressLookupTableAccount;
  exports.AddressLookupTableInstruction = AddressLookupTableInstruction;
  exports.AddressLookupTableProgram = AddressLookupTableProgram;
  exports.Authorized = Authorized;
  exports.BLOCKHASH_CACHE_TIMEOUT_MS = BLOCKHASH_CACHE_TIMEOUT_MS;
  exports.BPF_LOADER_DEPRECATED_PROGRAM_ID = BPF_LOADER_DEPRECATED_PROGRAM_ID;
  exports.BPF_LOADER_PROGRAM_ID = BPF_LOADER_PROGRAM_ID;
  exports.BpfLoader = BpfLoader;
  exports.COMPUTE_BUDGET_INSTRUCTION_LAYOUTS = COMPUTE_BUDGET_INSTRUCTION_LAYOUTS;
  exports.ComputeBudgetInstruction = ComputeBudgetInstruction;
  exports.ComputeBudgetProgram = ComputeBudgetProgram;
  exports.Connection = Connection;
  exports.Ed25519Program = Ed25519Program;
  exports.Enum = Enum;
  exports.EpochSchedule = EpochSchedule;
  exports.FeeCalculatorLayout = FeeCalculatorLayout;
  exports.Keypair = Keypair;
  exports.LAMPORTS_PER_SOL = LAMPORTS_PER_SOL;
  exports.LOOKUP_TABLE_INSTRUCTION_LAYOUTS = LOOKUP_TABLE_INSTRUCTION_LAYOUTS;
  exports.Loader = Loader;
  exports.Lockup = Lockup;
  exports.MAX_SEED_LENGTH = MAX_SEED_LENGTH;
  exports.Message = Message;
  exports.MessageAccountKeys = MessageAccountKeys;
  exports.MessageV0 = MessageV0;
  exports.NONCE_ACCOUNT_LENGTH = NONCE_ACCOUNT_LENGTH;
  exports.NonceAccount = NonceAccount;
  exports.PACKET_DATA_SIZE = PACKET_DATA_SIZE;
  exports.PUBLIC_KEY_LENGTH = PUBLIC_KEY_LENGTH;
  exports.PublicKey = PublicKey;
  exports.SIGNATURE_LENGTH_IN_BYTES = SIGNATURE_LENGTH_IN_BYTES;
  exports.SOLANA_SCHEMA = SOLANA_SCHEMA;
  exports.STAKE_CONFIG_ID = STAKE_CONFIG_ID;
  exports.STAKE_INSTRUCTION_LAYOUTS = STAKE_INSTRUCTION_LAYOUTS;
  exports.SYSTEM_INSTRUCTION_LAYOUTS = SYSTEM_INSTRUCTION_LAYOUTS;
  exports.SYSVAR_CLOCK_PUBKEY = SYSVAR_CLOCK_PUBKEY;
  exports.SYSVAR_EPOCH_SCHEDULE_PUBKEY = SYSVAR_EPOCH_SCHEDULE_PUBKEY;
  exports.SYSVAR_INSTRUCTIONS_PUBKEY = SYSVAR_INSTRUCTIONS_PUBKEY;
  exports.SYSVAR_RECENT_BLOCKHASHES_PUBKEY = SYSVAR_RECENT_BLOCKHASHES_PUBKEY;
  exports.SYSVAR_RENT_PUBKEY = SYSVAR_RENT_PUBKEY;
  exports.SYSVAR_REWARDS_PUBKEY = SYSVAR_REWARDS_PUBKEY;
  exports.SYSVAR_SLOT_HASHES_PUBKEY = SYSVAR_SLOT_HASHES_PUBKEY;
  exports.SYSVAR_SLOT_HISTORY_PUBKEY = SYSVAR_SLOT_HISTORY_PUBKEY;
  exports.SYSVAR_STAKE_HISTORY_PUBKEY = SYSVAR_STAKE_HISTORY_PUBKEY;
  exports.Secp256k1Program = Secp256k1Program;
  exports.SendTransactionError = SendTransactionError;
  exports.SolanaJSONRPCError = SolanaJSONRPCError;
  exports.SolanaJSONRPCErrorCode = SolanaJSONRPCErrorCode;
  exports.StakeAuthorizationLayout = StakeAuthorizationLayout;
  exports.StakeInstruction = StakeInstruction;
  exports.StakeProgram = StakeProgram;
  exports.Struct = Struct$1;
  exports.SystemInstruction = SystemInstruction;
  exports.SystemProgram = SystemProgram;
  exports.Transaction = Transaction;
  exports.TransactionExpiredBlockheightExceededError = TransactionExpiredBlockheightExceededError;
  exports.TransactionExpiredNonceInvalidError = TransactionExpiredNonceInvalidError;
  exports.TransactionExpiredTimeoutError = TransactionExpiredTimeoutError;
  exports.TransactionInstruction = TransactionInstruction;
  exports.TransactionMessage = TransactionMessage;
  exports.TransactionStatus = TransactionStatus;
  exports.VALIDATOR_INFO_KEY = VALIDATOR_INFO_KEY;
  exports.VERSION_PREFIX_MASK = VERSION_PREFIX_MASK;
  exports.VOTE_PROGRAM_ID = VOTE_PROGRAM_ID;
  exports.ValidatorInfo = ValidatorInfo;
  exports.VersionedMessage = VersionedMessage;
  exports.VersionedTransaction = VersionedTransaction;
  exports.VoteAccount = VoteAccount;
  exports.VoteAuthorizationLayout = VoteAuthorizationLayout;
  exports.VoteInit = VoteInit;
  exports.VoteInstruction = VoteInstruction;
  exports.VoteProgram = VoteProgram;
  exports.clusterApiUrl = clusterApiUrl;
  exports.sendAndConfirmRawTransaction = sendAndConfirmRawTransaction;
  exports.sendAndConfirmTransaction = sendAndConfirmTransaction;
  return exports;
})($Object.create(null, undefined));

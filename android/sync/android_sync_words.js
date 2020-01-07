/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var sync = 'cfa95630-88fb-4753-8b47-f1ef1819ab58'
function getBytesFromWords(words) {
    try {
      injectedObject.cryptoOutput(JSON.stringify(module.exports.passphrase.toBytes32(words)))
    } catch(e) {
      injectedObject.cryptoOutputError("JS error: " + e)
    }
}

function getCodeWordsFromSeed(seed) {
    try {
      var buffer = new Uint8Array(seed)
      injectedObject.cryptoOutputCodeWords(JSON.stringify(module.exports.passphrase.fromBytesOrHex(buffer)))
    } catch(e) {
      injectedObject.cryptoOutputError("JS error: " + e)
    }
}

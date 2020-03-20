/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 export const getUSDPrice = (accountBTCBalance: string, btcUSDPrice: string) => {
  if (!accountBTCBalance || !btcUSDPrice) {
    return '0.00'
  }

  const btcUSDPriceNumber = parseFloat(btcUSDPrice)
  const btcBalanceNumber = parseFloat(accountBTCBalance)

  if (isNaN(btcUSDPriceNumber) || isNaN(btcBalanceNumber)) {
    return '0.00'
  }

  return (btcUSDPriceNumber * btcBalanceNumber).toFixed(2)
}

const generateRandomString = () => {
  const array = new Uint32Array(28)
  window.crypto.getRandomValues(array)
  return Array.from(array, dec => ('0' + dec.toString(16)).substr(-2)).join('')
}

const sha256 = (verifier: string) => {
  const encoder = new TextEncoder()
  const data = encoder.encode(verifier)
  return window.crypto.subtle.digest('SHA-256', data)
}

const base64encode = (hashed: ArrayBuffer) => {
  let binary = '';
  const bytes = new Uint8Array(hashed);
  for (let i = 0; i < bytes.byteLength; i++) {
    binary += String.fromCharCode(bytes[i]);
  }
  return window.btoa(binary);
}

const generateCodeChallenge = async (verifier: string) => {
  const hashed = await sha256(verifier)
  return base64encode(hashed)
}

export const getCodeChallenge = async () => {
  const challenge = await generateCodeChallenge(generateRandomString())
  return challenge
}

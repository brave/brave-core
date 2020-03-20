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

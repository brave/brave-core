/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const qr = require('qr-image')
import BigNumber from 'bignumber.js'
import { Address } from 'brave-ui/features/rewards/modalAddFunds'

export let actions: any = null

export const getActions = () => actions
export const setActions = (newActions: any) => actions = newActions

export const convertBalance = (tokens: string, rates: Record<string, number> | undefined, currency: string = 'USD'): string => {
  const tokensNum = parseFloat(tokens)
  if (tokensNum === 0 || !rates || !rates[currency]) {
    return '0.00'
  }

  const converted = tokensNum * rates[currency]

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const generateContributionMonthly = (list: number[], rates: Record<string, number> | undefined) => {
  if (!list) {
    return []
  }

  return list.map((item: number) => {
    return {
      tokens: item.toFixed(1),
      converted: convertBalance(item.toString(), rates)
    }
  })
}

export const generateQR = (addresses: Record<Rewards.AddressesType, string>) => {
  let url = null

  const generate = (type: Rewards.AddressesType, address: string) => {
    switch (type) {
      case 'BAT':
      case 'ETH':
        url = `ethereum:${address}`
        break
      case 'BTC':
        url = `bitcoin:${address}`
        break
      case 'LTC':
        url = `litecoin:${address}`
        break
      default:
        return
    }

    try {
      let chunks: Uint8Array[] = []
      qr.image(url, { type: 'png' })
        .on('data', (chunk: Uint8Array) => {
          chunks.push(chunk)
        })
        .on('end', () => {
          const qrImage = 'data:image/png;base64,' + Buffer.concat(chunks).toString('base64')
          if (actions) {
            actions.onQRGenerated(type, qrImage)
          }
        })
    } catch (ex) {
      console.error('qr.imageSync (for url ' + url + ') error: ' + ex.toString())
    }
  }

  for (let type in addresses) {
    generate(type as Rewards.AddressesType, addresses[type])
  }
}

export const getAddresses = (addresses?: Record<Rewards.AddressesType, Rewards.Address>) => {
  let result: Address[] = []

  if (!addresses) {
    return result
  }

  const sortedArray = [ 'BTC', 'ETH', 'BAT', 'LTC' ]

  sortedArray.forEach((type: Rewards.AddressesType) => {
    const item: Rewards.Address = addresses[type]
    if (item) {
      result.push({
        type,
        qr: item.qr,
        address: item.address
      })
    }
  })

  return result
}

export const convertProbiToFixed = (probi: string, places: number = 1) => {
  const result = new BigNumber(probi).dividedBy('1e18').toFixed(places, BigNumber.ROUND_DOWN)

  if (result === 'NaN') {
    return '0.0'
  }

  return result
}

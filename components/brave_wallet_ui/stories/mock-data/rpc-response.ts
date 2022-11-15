// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { RPCResponseType } from '../../constants/types'

export const mockRPCResponse: RPCResponseType[] = [
  {
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    assets: [
      {
        id: '1',
        name: 'Ethereum',
        symbol: 'ETH',
        balance: 3.24
      },
      {
        id: '2',
        name: 'Basic Attention Token',
        symbol: 'BAT',
        balance: 4685.36
      },
      {
        id: '3',
        name: 'Binance Coin',
        symbol: 'BNB',
        balance: 58.54
      },
      {
        id: '4',
        name: 'Bitcoin',
        symbol: 'BTC',
        balance: 0.00269
      },
      {
        id: '7',
        name: 'AcclimatedMoonCats',
        symbol: 'AMC',
        balance: 1
      }
    ],
    transactions: [
      {
        assetId: '1',
        amount: 0.6,
        to: '0xf6s5e8f4AED3115d93Bd1790332f3Cd0h5v6r5g8',
        from: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
        hash: 'f65a4sd6f546sd6f4'
      },
      {
        assetId: '2',
        amount: 102,
        to: '0xf6s5e8f4AED3115d93Bd1790332f3Cd0h5v6r5g8',
        from: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
        hash: 'r54y6h5f4h6sfg54d6f5g4s'
      },
      {
        assetId: '3',
        amount: 5.3,
        to: '0xf6s5e8f4AED3115d93Bd1790332f3Cd0h5v6r5g8',
        from: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
        hash: 'rey645sfg4s65fgsafg454fg'
      },
      {
        assetId: '4',
        amount: 0.00269,
        to: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
        from: '0xf6s5e8f4AED3115d93Bd1790332f3Cd0h5v6r5g8',
        hash: 't8j46d8fgjh3sd5f4tg65f'
      }
    ]
  },
  {
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    assets: [
      {
        id: '2',
        name: 'Basic Attention Token',
        symbol: 'BAT',
        balance: 254.36
      },
      {
        id: '3',
        name: 'Binance Coin',
        symbol: 'BNB',
        balance: 1.33
      },
      {
        id: '4',
        name: 'Bitcoin',
        symbol: 'BTC',
        balance: 0
      }
    ],
    transactions: [
      {
        assetId: '1',
        amount: 0.3,
        to: '0xf6s5e8f63dD3115d93Bd1790332f3Cd0h5lsog5v',
        from: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
        hash: '564hkj65dghs65fdgs6fd'
      }
    ]
  },
  {
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    assets: [
      {
        id: '1',
        name: 'Ethereum',
        symbol: 'ETH',
        balance: 1.2
      },
      {
        id: '2',
        name: 'Basic Attention Token',
        symbol: 'BAT',
        balance: 0
      },
      {
        id: '3',
        name: 'Binance Coin',
        symbol: 'BNB',
        balance: 0
      },
      {
        id: '4',
        name: 'Bitcoin',
        symbol: 'BTC',
        balance: 0
      }
    ],
    transactions: []
  }
]

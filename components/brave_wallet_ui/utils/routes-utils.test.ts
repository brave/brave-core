// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountPageTabs, WalletRoutes } from '../constants/types'
import {
  mockBitcoinAccount,
  mockEthAccount,
} from '../stories/mock-data/mock-wallet-accounts'
import { mockEthToken } from '../stories/mock-data/mock-asset-options'
import {
  makeAccountRoute,
  makeAccountTransactionRoute,
  makePortfolioNftCollectionRoute,
  makeSendRoute,
} from './routes-utils'

describe('makeAccountRoute', () => {
  it('routes for eth account', () => {
    expect(
      makeAccountRoute(mockEthAccount, AccountPageTabs.AccountAssetsSub),
    ).toBe('/crypto/accounts/0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14/assets')
    expect(
      makeAccountRoute(mockEthAccount, AccountPageTabs.AccountNFTsSub),
    ).toBe('/crypto/accounts/0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14/nfts')
    expect(
      makeAccountRoute(mockEthAccount, AccountPageTabs.AccountTransactionsSub),
    ).toBe(
      '/crypto/accounts/0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14/transactions',
    )
  })

  it('routes for bitcoin account', () => {
    expect(
      makeAccountRoute(mockBitcoinAccount, AccountPageTabs.AccountAssetsSub),
    ).toBe('/crypto/accounts/mockBitcoinAccount_uniqueKey/assets')
    expect(
      makeAccountRoute(mockBitcoinAccount, AccountPageTabs.AccountNFTsSub),
    ).toBe('/crypto/accounts/mockBitcoinAccount_uniqueKey/nfts')
    expect(
      makeAccountRoute(
        mockBitcoinAccount,
        AccountPageTabs.AccountTransactionsSub,
      ),
    ).toBe('/crypto/accounts/mockBitcoinAccount_uniqueKey/transactions')
  })
})

describe('makeAccountTransactionRoute', () => {
  it('transaction for eth account', () => {
    expect(makeAccountTransactionRoute(mockEthAccount, '#transactionId')).toBe(
      '/crypto/accounts/0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14/'
        + 'transactions#transactionId',
    )
  })

  it('transaction for bitcoin account', () => {
    expect(
      makeAccountTransactionRoute(mockBitcoinAccount, '#transactionId'),
    ).toBe(
      '/crypto/accounts/mockBitcoinAccount_uniqueKey/transactions#transactionId',
    )
  })
})

describe('makePortfolioNftCollectionRoute', () => {
  it('uses the correct router params', () => {
    expect(WalletRoutes.PortfolioNFTCollection).toBe(
      '/crypto/portfolio/collections/:collectionName',
    )

    const routeWithoutPage = makePortfolioNftCollectionRoute('MoonCatsRescue')
    expect(routeWithoutPage).toBe(
      '/crypto/portfolio/collections/MoonCatsRescue',
    )

    const routeWithPage = makePortfolioNftCollectionRoute('MoonCatsRescue', 2)
    expect(routeWithPage).toBe(
      '/crypto/portfolio/collections/MoonCatsRescue?page=2',
    )
  })
})

describe('makeSendRoute', () => {
  it('should return a route with chainId and token params', () => {
    expect(makeSendRoute(mockEthToken)).toBe(
      '/send?chainId=0x1&token=ETH#token',
    )
  })

  it('should return a route with chainId, token and account params', () => {
    expect(makeSendRoute(mockEthToken, mockEthAccount)).toBe(
      '/send?chainId=0x1&token=ETH&account=mockEthAccount_uniqueKey#token',
    )
  })

  it(
    'should return a route with chainId, token, '
      + 'account and recipient params',
    () => {
      expect(makeSendRoute(mockEthToken, mockEthAccount, '0x1234567890')).toBe(
        '/send?chainId=0x1&token=ETH&account=mockEthAccount_uniqueKey'
          + '&recipient=0x1234567890#token',
      )
    },
  )

  it('should return a route with chainId, token and recipient params', () => {
    expect(makeSendRoute(mockEthToken, undefined, '0x1234567890')).toBe(
      '/send?chainId=0x1&token=ETH&recipient=0x1234567890#token',
    )
  })
})

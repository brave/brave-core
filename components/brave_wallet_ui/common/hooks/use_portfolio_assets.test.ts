// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { renderHook, waitFor } from '@testing-library/react'

// utils
import {
  createMockStore,
  renderHookOptionsWithMockStore,
} from '../../utils/test-utils'
import {
  createEmptyTokenBalancesRegistry,
  setBalance,
} from '../../utils/balance-utils'
import Amount from '../../utils/amount'

// hooks
import { usePortfolioAssets } from './use_portfolio_assets'

// mocks
import { mockAccount } from '../constants/mocks'
import { mockEthMainnet } from '../../stories/mock-data/mock-networks'
import { mockBasicAttentionToken } from '../../stories/mock-data/mock-asset-options'
import { BraveWallet, WalletState } from '../../constants/types'
import { WalletApiDataOverrides } from '../../constants/testing_types'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'

const mockBatBalance = '1000000000000000000' // 1 BAT
const mockSpotPrice = '3873.78'

const createTokenBalanceRegistry = () => {
  const tokenBalancesRegistry = createEmptyTokenBalancesRegistry()
  setBalance({
    accountId: mockAccount.accountId,
    balance: mockBatBalance,
    chainId: mockEthMainnet.chainId,
    coinType: mockEthMainnet.coin,
    contractAddress: mockBasicAttentionToken.contractAddress,
    tokenId: '',
    tokenBalancesRegistry,
    zcashTokenType: BraveWallet.ZCashTokenType.kNone,
  })
  return tokenBalancesRegistry
}

const createPortfolioStore = (
  apiOverrides?: WalletApiDataOverrides,
  walletStateOverride?: Partial<WalletState>,
) => {
  return createMockStore(
    {
      walletStateOverride: {
        isWalletCreated: apiOverrides?.isWalletCreated ?? true,
        isWalletLocked: false,
        hasInitialized: true,
        ...walletStateOverride,
      },
    },
    {
      isWalletCreated: true,
      accountInfos: [mockAccount],
      networks: [mockEthMainnet],
      userAssets: [mockBasicAttentionToken],
      blockchainTokens: [mockBasicAttentionToken],
      tokenBalanceRegistry: createTokenBalanceRegistry(),
      defaultBaseCurrency: 'usd',
      ...apiOverrides,
    },
  )
}

describe('usePortfolioAssets hook', () => {
  beforeEach(() => {
    window.localStorage.clear()
  })

  it('starts with an empty formatted fiat balance while loading', () => {
    const store = createPortfolioStore()
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    expect(result.current.formattedFullPortfolioFiatBalance).toBe('')
    expect(result.current.fullPortfolioFiatBalance.isUndefined()).toBe(true)
  })

  it('computes the portfolio fiat total from balances and spot prices', async () => {
    const store = createPortfolioStore()
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.formattedFullPortfolioFiatBalance).not.toBe('')
    })

    const expectedFiat = new Amount(mockBatBalance)
      .divideByDecimals(mockBasicAttentionToken.decimals)
      .times(mockSpotPrice)

    expect(result.current.defaultFiat).toBe('usd')
    expect(result.current.fullPortfolioFiatBalance.toNumber()).toBeCloseTo(
      expectedFiat.toNumber(),
    )
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      expectedFiat.compactAsFiat('usd'),
    )
    expect(result.current.visibleAssetOptions).toHaveLength(1)
    expect(result.current.visibleAssetOptions[0].asset.symbol).toBe('BAT')
    expect(result.current.visibleAssetOptions[0].assetBalance).toBe(
      mockBatBalance,
    )
    expect(result.current.usersFilteredAccounts).toHaveLength(1)
    expect(result.current.visiblePortfolioNetworks).toHaveLength(1)
  })

  it('returns a zero fiat total when there are no user assets', async () => {
    const store = createPortfolioStore({
      userAssets: [],
      blockchainTokens: [],
      tokenBalanceRegistry: createEmptyTokenBalancesRegistry(),
    })
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.formattedFullPortfolioFiatBalance).not.toBe('')
    })

    expect(result.current.visibleAssetOptions).toHaveLength(0)
    expect(result.current.fullPortfolioFiatBalance.isZero()).toBe(true)
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      Amount.zero().compactAsFiat('usd'),
    )
  })

  it('keeps an empty fiat total when the wallet is not created', async () => {
    const store = createPortfolioStore({
      isWalletCreated: false,
    })
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.defaultFiat).toBe('usd')
      expect(result.current.usersFilteredAccounts.length).toBeGreaterThan(0)
    })

    expect(result.current.tokenBalancesRegistry).toBeFalsy()
    expect(result.current.formattedFullPortfolioFiatBalance).toBe('')
    expect(result.current.fullPortfolioFiatBalance.isUndefined()).toBe(true)
  })

  it('returns a zero fiat total when all accounts are filtered out', async () => {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.FILTERED_OUT_PORTFOLIO_ACCOUNT_IDS,
      JSON.stringify([mockAccount.accountId.uniqueKey]),
    )
    const store = createPortfolioStore({
      tokenBalanceRegistry: createEmptyTokenBalancesRegistry(),
    })
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.formattedFullPortfolioFiatBalance).not.toBe('')
    })

    expect(result.current.usersFilteredAccounts).toHaveLength(0)
    expect(result.current.tokenBalancesRegistry).toBeFalsy()
    expect(result.current.fullPortfolioFiatBalance.isZero()).toBe(true)
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      Amount.zero().compactAsFiat('usd'),
    )
  })

  it('returns a zero fiat total when there are no visible networks', async () => {
    const store = createPortfolioStore({
      networks: [],
      tokenBalanceRegistry: createEmptyTokenBalancesRegistry(),
    })
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.formattedFullPortfolioFiatBalance).not.toBe('')
    })

    expect(result.current.visiblePortfolioNetworks).toHaveLength(0)
    expect(result.current.tokenBalancesRegistry).toBeFalsy()
    expect(result.current.fullPortfolioFiatBalance.isZero()).toBe(true)
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      Amount.zero().compactAsFiat('usd'),
    )
  })

  it('returns a zero fiat total when the wallet is locked and balances are skipped', async () => {
    const store = createPortfolioStore({}, { isWalletLocked: true })
    const { result } = renderHook(
      () => usePortfolioAssets(),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.formattedFullPortfolioFiatBalance).not.toBe('')
    })

    expect(result.current.tokenBalancesRegistry).toBeFalsy()
    expect(result.current.fullPortfolioFiatBalance.isZero()).toBe(true)
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      Amount.zero().compactAsFiat('usd'),
    )
  })

  it('skips spot prices when skipSpotPrices is set', async () => {
    const store = createPortfolioStore()
    const { result } = renderHook(
      () => usePortfolioAssets({ skipSpotPrices: true }),
      renderHookOptionsWithMockStore(store),
    )

    await waitFor(() => {
      expect(result.current.tokenBalancesRegistry).toBeTruthy()
      expect(result.current.visibleAssetOptions.length).toBeGreaterThan(0)
    })

    expect(result.current.spotPrices).toEqual([])
    expect(result.current.fullPortfolioFiatBalance.isZero()).toBe(true)
    expect(result.current.formattedFullPortfolioFiatBalance).toBe(
      Amount.zero().compactAsFiat('usd'),
    )
  })
})

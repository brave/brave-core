// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mapLimit } from 'async'

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  SetUserAssetVisiblePayloadType,
  UpdateUsetAssetType
} from '../constants/action_types'
import { BraveWallet, WalletState, RefreshOpts } from '../../constants/types'

// Utils
import getAPIProxy from './bridge'
import {
  refreshVisibleTokenInfo,
  refreshPortfolioFilterOptions,
  getNFTMetadata
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import { walletApi } from '../slices/api.slice'
import { getVisibleNetworksList } from '../../utils/api-utils'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

function getWalletState(store: Store): WalletState {
  return store.getState().wallet
}

async function refreshBalancesPricesAndHistory(store: Store) {
  await store.dispatch(refreshVisibleTokenInfo())
}

async function refreshWalletInfo(store: Store, payload: RefreshOpts = {}) {
  const apiProxy = getAPIProxy()

  const { walletInfo } = await apiProxy.walletHandler.getWalletInfo()
  const { allAccounts } = await apiProxy.keyringService.getAllAccounts()
  store.dispatch(WalletActions.initialized({ walletInfo, allAccounts }))
  store.dispatch(WalletActions.refreshAll(payload))

  // refresh networks registry & selected network
  await store
    .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
    .unwrap()

  // Populate tokens from blockchain registry.
  store.dispatch(WalletActions.getAllTokensList())

  store.dispatch(
    walletApi.util.invalidateTags([
      'ConnectedAccounts',
      'DefaultEthWallet',
      'DefaultSolWallet',
      'IsMetaMaskInstalled'
    ])
  )
}

handler.on(
  WalletActions.refreshNetworksAndTokens.type,
  async (store: Store, payload: RefreshOpts) => {
    // refresh networks registry & selected network
    store.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(true))
    await store
      .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
      .unwrap()
    await store.dispatch(refreshVisibleTokenInfo())
    await store.dispatch(refreshPortfolioFilterOptions())
    store.dispatch(WalletActions.setIsRefreshingNetworksAndTokens(false))
  }
)

handler.on(
  WalletActions.initialize.type,
  async (store, payload: RefreshOpts) => {
    // Initialize active origin state.
    const braveWalletService = getAPIProxy().braveWalletService
    const { originInfo } = await braveWalletService.getActiveOrigin()
    store.dispatch(WalletActions.activeOriginChanged(originInfo))
    await refreshWalletInfo(store, payload)
  }
)

handler.on(WalletActions.walletCreated.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.walletRestored.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.walletReset.type, async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.type, async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.type, async (store) => {
  await refreshWalletInfo(store, {
    skipBalancesRefresh: true
  })
})

handler.on(WalletActions.backedUp.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(
  WalletActions.defaultBaseCryptocurrencyChanged.type,
  async (store) => {
    await refreshWalletInfo(store)
  }
)

handler.on(
  WalletActions.refreshAll.type,
  async (store: Store, payload: RefreshOpts) => {
    const keyringService = getAPIProxy().keyringService
    const state = getWalletState(store)
    if (!state.isWalletLocked) {
      keyringService.notifyUserInteraction()
    }
    interactionNotifier.beginWatchingForInteraction(
      50000,
      state.isWalletLocked,
      async () => {
        keyringService.notifyUserInteraction()
      }
    )
    const braveWalletService = getAPIProxy().braveWalletService
    store.dispatch(walletApi.util.invalidateTags(['DefaultFiatCurrency']))
    // Fetch Balances and Prices
    if (!state.isWalletLocked && state.isWalletCreated) {
      // refresh networks registry & selected network
      await store
        .dispatch(walletApi.endpoints.refreshNetworkInfo.initiate())
        .unwrap()
      await store.dispatch(refreshVisibleTokenInfo())
      await store.dispatch(refreshPortfolioFilterOptions())
      await braveWalletService.discoverAssetsOnAllSupportedChains()
    }
  }
)

handler.on(WalletActions.getAllTokensList.type, async (store) => {
  const api = getAPIProxy()
  const networkList = await getVisibleNetworksList(api)
  const { blockchainRegistry } = api
  const getAllTokensList = await mapLimit(
    networkList,
    10,
    async (network: BraveWallet.NetworkInfo) => {
      const list = await blockchainRegistry.getAllTokens(
        network.chainId,
        network.coin
      )
      return list.tokens.map((token) => {
        return {
          ...token,
          chainId: network.chainId,
          logo: `chrome://erc-token-images/${token.logo}`
        }
      })
    }
  )
  const allTokensList = getAllTokensList.flat(1)
  store.dispatch(WalletActions.setAllTokensList(allTokensList))
})

handler.on(
  WalletActions.addUserAsset.type,
  async (store: Store, payload: BraveWallet.BlockchainToken) => {
    const { braveWalletService } = getAPIProxy()

    if (payload.isErc721 || payload.isNft) {
      const result = await getNFTMetadata(payload)
      if (!result?.error) {
        const response = result?.response && JSON.parse(result.response)
        payload.logo = response.image || payload.logo
      }
    }

    const result = await braveWalletService.addUserAsset(payload)

    // Refresh balances here for adding ERC721 tokens if result is successful
    if ((payload.isErc721 || payload.isNft) && result.success) {
      refreshBalancesPricesAndHistory(store)
    }
    store.dispatch(WalletActions.addUserAssetError(!result.success))
  }
)

handler.on(
  WalletActions.updateUserAsset.type,
  async (store: Store, payload: UpdateUsetAssetType) => {
    const { braveWalletService } = getAPIProxy()
    const { existing, updated } = payload
    // fetch NFT metadata if tokenId or contract address has changed
    if (
      (updated.isNft || updated.isErc721) &&
      (updated.tokenId !== existing.tokenId ||
        updated.contractAddress !== existing.contractAddress)
    ) {
      const result = await getNFTMetadata(updated)
      if (!result?.error) {
        try {
          const nftMetadata = result?.response && JSON.parse(result.response)
          updated.logo = nftMetadata?.image || ''
        } catch (error) {
          console.error(error)
        }
      }
    }

    const deleteResult = await braveWalletService.removeUserAsset(existing)
    if (deleteResult.success) {
      const addResult = await braveWalletService.addUserAsset(updated)
      if (addResult.success) {
        refreshBalancesPricesAndHistory(store)
        await store.dispatch(refreshVisibleTokenInfo())
      }
    }
  }
)

handler.on(
  WalletActions.removeUserAsset.type,
  async (store: Store, payload: BraveWallet.BlockchainToken) => {
    const { braveWalletService } = getAPIProxy()
    await braveWalletService.removeUserAsset(payload)
  }
)

handler.on(
  WalletActions.setUserAssetVisible.type,
  async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
    const { braveWalletService } = getAPIProxy()

    const { success } = await braveWalletService.setUserAssetVisible(
      payload.token,
      payload.isVisible
    )

    if (!success) {
      // token is probably not in the core-side assets list
      // try adding it to the user tokens list
      store.dispatch(WalletActions.addUserAsset(payload.token))
    }
  }
)

handler.on(
  WalletActions.refreshBalancesAndPriceHistory.type,
  async (store: Store) => {
    await refreshBalancesPricesAndHistory(store)
  }
)

export default handler.middleware

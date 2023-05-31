// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  ChainChangedEventPayloadType,
  AddSitePermissionPayloadType,
  RemoveSitePermissionPayloadType,
  SetUserAssetVisiblePayloadType,
  UnlockWalletPayloadType,
  SelectedAccountChangedPayloadType,
  GetCoinMarketPayload,
  UpdateUsetAssetType
} from '../constants/action_types'
import {
  BraveWallet,
  WalletState,
  WalletInfo,
  NetworkFilterType
} from '../../constants/types'
import {
  AddAccountPayloadType,
  AddBitcoinAccountPayloadType
} from '../../page/constants/action_types'

// Utils

import getAPIProxy from './bridge'
import {
  refreshKeyringInfo,
  refreshTokenPriceHistory,
  refreshSitePermissions,
  refreshBalances,
  refreshVisibleTokenInfo,
  refreshPrices,
  refreshPortfolioFilterOptions,
  getNFTMetadata
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import {
  getVisibleNetworksList,
  walletApi
} from '../slices/api.slice'
import { deserializeOrigin, makeSerializableOriginInfo } from '../../utils/model-serialization-utils'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshBalancesPricesAndHistory (store: Store) {
  const state = getWalletState(store)
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
  await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
}

async function refreshWalletInfo (store: Store) {
  const apiProxy = getAPIProxy()

  await store.dispatch(refreshKeyringInfo())

  // refresh networks registry & selected network
  await store.dispatch(
    walletApi.endpoints.refreshNetworkInfo.initiate()
  ).unwrap()

  // Populate tokens from blockchain registry.
  store.dispatch(WalletActions.getAllTokensList())

  const braveWalletService = apiProxy.braveWalletService
  const defaultEthereumResult = await braveWalletService.getDefaultEthereumWallet()
  store.dispatch(WalletActions.defaultEthereumWalletUpdated(defaultEthereumResult.defaultWallet))
  const defaultSolanaResult = await braveWalletService.getDefaultSolanaWallet()
  store.dispatch(WalletActions.defaultSolanaWalletUpdated(defaultSolanaResult.defaultWallet))

  const mmResult =
    await braveWalletService.isExternalWalletInstalled(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletActions.setMetaMaskInstalled(mmResult.installed))

  await store.dispatch(refreshSitePermissions())
  store.dispatch(WalletActions.getOnRampCurrencies())
}

async function updateAccountInfo (store: Store) {
  const state = getWalletState(store)
  const proxy = getAPIProxy()
  const { allAccounts } = (await proxy.keyringService.getAllAccounts())
  if (state.accounts.length === allAccounts.accounts.length) {
    await store.dispatch(WalletActions.refreshAccountInfo(allAccounts))
  } else {
    await refreshWalletInfo(store)
  }
}

async function updateCoinAccountNetworkInfo (store: Store, coin: BraveWallet.CoinType) {
  const { accounts } = getWalletState(store)
  if (accounts.length === 0) {
    return
  }
  const { braveWalletService, keyringService } = getAPIProxy()

  // Update Selected Coin & cached selected network
  await store
    .dispatch(walletApi.endpoints.setSelectedCoin.initiate(coin))
    .unwrap()

  // Updated Selected Account
  const { address: selectedAccountAddress } =
    coin === BraveWallet.CoinType.FIL
      ? await keyringService.getFilecoinSelectedAccount(
          (
            await braveWalletService.getChainIdForActiveOrigin(coin)
          ).chainId
        )
      : await keyringService.getSelectedAccount(coin)

  const defaultAccount =
    accounts.find(
      (account) => account.address === selectedAccountAddress
    ) || accounts[0]
  await store.dispatch(WalletActions.setSelectedAccount(defaultAccount))
}

handler.on(WalletActions.refreshBalancesAndPrices.type, async (store: Store) => {
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
})

handler.on(WalletActions.refreshNetworksAndTokens.type, async (store: Store) => {
  // refresh networks registry & selected network
  await store.dispatch(
    walletApi.endpoints.refreshNetworkInfo.initiate()
  ).unwrap()
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
  await store.dispatch(refreshPortfolioFilterOptions())
})

handler.on(WalletActions.initialize.type, async (store) => {
  // Initialize active origin state.
  const braveWalletService = getAPIProxy().braveWalletService
  const { originInfo } = await braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(
    makeSerializableOriginInfo(originInfo)
  ))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringCreated.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringRestored.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringReset.type, async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.type, async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.backedUp.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.accountsChanged.type, async (store) => {
  await updateAccountInfo(store)
})

handler.on(WalletActions.chainChangedEvent.type, async (store: Store, payload: ChainChangedEventPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(
  WalletActions.selectAccount.type,
  async (store: Store, accountId: BraveWallet.AccountId) => {
    await store.dispatch(
      walletApi.endpoints.setSelectedAccount.initiate(accountId)
    )
  }
)


handler.on(WalletActions.selectedAccountChanged.type, async (store, payload: SelectedAccountChangedPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(WalletActions.defaultEthereumWalletChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultSolanaWalletChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCryptocurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.lockWallet.type, async (store) => {
  const keyringService = getAPIProxy().keyringService
  keyringService.lock()
})

handler.on(WalletActions.unlockWallet.type, async (store: Store, payload: UnlockWalletPayloadType) => {
  const keyringService = getAPIProxy().keyringService
  const result = await keyringService.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

// TODO(apaymyshev): remove apps ui
handler.on(WalletActions.addFavoriteApp.type, async (store: Store, appItem: BraveWallet.AppItem) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.type, async (store: Store, appItem: BraveWallet.AppItem) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.initialized.type, async (store: Store, payload: WalletInfo) => {
  const keyringService = getAPIProxy().keyringService
  const state = getWalletState(store)
  if (!state.isWalletLocked) {
    keyringService.notifyUserInteraction()
  }
  interactionNotifier.beginWatchingForInteraction(50000, state.isWalletLocked, async () => {
    keyringService.notifyUserInteraction()
  })
  const braveWalletService = getAPIProxy().braveWalletService
  const defaultFiat = await braveWalletService.getDefaultBaseCurrency()
  const defaultCrypto = await braveWalletService.getDefaultBaseCryptocurrency()
  const defaultCurrencies = {
    fiat: defaultFiat.currency,
    crypto: defaultCrypto.cryptocurrency
  }
  store.dispatch(WalletActions.defaultCurrenciesUpdated(defaultCurrencies))
  // Fetch Balances and Prices
  if (!state.isWalletLocked && state.isWalletCreated) {
    // refresh networks registry & selected network
    await store.dispatch(
      walletApi.endpoints.refreshNetworkInfo.initiate()
    ).unwrap()
    await store.dispatch(refreshVisibleTokenInfo())
    await store.dispatch(refreshBalances())
    await store.dispatch(refreshPortfolioFilterOptions())
    await store.dispatch(refreshPrices())
    await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
    await braveWalletService.discoverAssetsOnAllSupportedChains()
  }
})

handler.on(WalletActions.getAllTokensList.type, async (store) => {
  const api = getAPIProxy()
  const networkList = await getVisibleNetworksList(api)
  const { blockchainRegistry } = api
  const getAllTokensList = await Promise.all(networkList.map(async (network) => {
    const list = await blockchainRegistry.getAllTokens(network.chainId, network.coin)
    return list.tokens.map((token) => {
      return {
        ...token,
        chainId: network.chainId,
        logo: `chrome://erc-token-images/${token.logo}`
      }
    })
  }))
  const allTokensList = getAllTokensList.flat(1)
  store.dispatch(WalletActions.setAllTokensList(allTokensList))
})

handler.on(WalletActions.addUserAsset.type, async (store: Store, payload: BraveWallet.BlockchainToken) => {
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
})

handler.on(WalletActions.updateUserAsset.type, async (store: Store, payload: UpdateUsetAssetType) => {
  const { braveWalletService } = getAPIProxy()
  const { existing, updated } = payload
  // fetch NFT metadata if tokenId or contract address has changed
  if ((updated.isNft || updated.isErc721) && (updated.tokenId !== existing.tokenId || updated.contractAddress !== existing.contractAddress)) {
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
      refreshVisibleTokenInfo()
    }
  }
})

handler.on(WalletActions.removeUserAsset.type, async (store: Store, payload: BraveWallet.BlockchainToken) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.removeUserAsset(payload)
})

handler.on(WalletActions.setUserAssetVisible.type, async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.setUserAssetVisible(payload.token, payload.isVisible)
})

handler.on(WalletActions.refreshBalancesAndPriceHistory.type, async (store: Store) => {
  refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.selectPortfolioTimeline.type, async (store: Store, payload: BraveWallet.AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await store.dispatch(refreshTokenPriceHistory(payload))
})

handler.on(WalletActions.removeSitePermission.type, async (store: Store, payload: RemoveSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.resetPermission(payload.accountId, deserializeOrigin(payload.origin))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.addSitePermission.type, async (store: Store, payload: AddSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.addPermission(payload.accountId, deserializeOrigin(payload.origin))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.expandWalletNetworks.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet/networks' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(WalletActions.getCoinMarkets.type, async (store: Store, payload: GetCoinMarketPayload) => {
  const assetRatioService = getAPIProxy().assetRatioService
  const result = await assetRatioService.getCoinMarkets(payload.vsAsset, payload.limit)
  store.dispatch(WalletActions.setCoinMarkets(result))
})

handler.on(WalletActions.setSelectedNetworkFilter.type, async (store: Store, payload: NetworkFilterType) => {
  const state = getWalletState(store)
  const { selectedPortfolioTimeline } = state
  await store.dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
})

handler.on(WalletActions.setSelectedAccountFilterItem.type, async (store: Store, payload: string) => {
  const state = getWalletState(store)
  const { selectedPortfolioTimeline } = state
  await store.dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
})

handler.on(WalletActions.addAccount.type, async (_store: Store, payload: AddAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addAccount(payload.coin, payload.keyringId, payload.accountName)
  return !!result.accountInfo
})

handler.on(WalletActions.addBitcoinAccount.type, async (_store: Store, payload: AddBitcoinAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addBitcoinAccount(payload.accountName, payload.networkId, payload.keyringId)
  return !!result.accountInfo
})

handler.on(WalletActions.getOnRampCurrencies.type, async (store: Store) => {
  const { blockchainRegistry } = getAPIProxy()
  const currencies = (await blockchainRegistry.getOnRampCurrencies()).currencies
  await store.dispatch(WalletActions.setOnRampCurrencies(currencies))
})

export default handler.middleware

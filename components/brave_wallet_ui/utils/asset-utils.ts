/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Types
import {
  AssetIdsByCollectionNameRegistry,
  BraveWallet
} from '../constants/types'
import {
  TokenBalancesRegistry //
} from '../common/slices/entities/token-balance.entity'

// utils
import Amount from './amount'
import { getRampNetworkPrefix } from './string-utils'
import { getNetworkLogo, makeNativeAssetLogo } from '../options/asset-options'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import { getBalance } from './balance-utils'

export const getUniqueAssets = (assets: BraveWallet.BlockchainToken[]) => {
  return assets.filter((asset, index) => {
    return (
      index ===
      assets.findIndex((item) => {
        return (
          item.contractAddress.toLowerCase() ===
            asset.contractAddress.toLowerCase() &&
          item.chainId === asset.chainId
        )
      })
    )
  })
}

export const isSelectedAssetInAssetOptions = (
  selectedAsset: BraveWallet.BlockchainToken,
  assetOptions: BraveWallet.BlockchainToken[]
) => {
  return (
    assetOptions.findIndex((asset) => {
      return (
        asset.contractAddress.toLowerCase() ===
          selectedAsset?.contractAddress.toLowerCase() &&
        asset.chainId === selectedAsset.chainId &&
        asset.symbol.toLowerCase() === selectedAsset.symbol.toLowerCase()
      )
    }) !== -1
  )
}

export const getRampAssetSymbol = (
  asset: BraveWallet.BlockchainToken,
  isOfframp?: boolean
) => {
  if (
    asset.symbol.toUpperCase() === 'BAT' &&
    asset.chainId === BraveWallet.MAINNET_CHAIN_ID
  ) {
    // BAT is the only token on Ethereum Mainnet with a prefix on Ramp.Network
    return 'ETH_BAT'
  }

  if (
    asset.chainId === BraveWallet.AVALANCHE_MAINNET_CHAIN_ID &&
    asset.contractAddress === ''
  ) {
    return isOfframp ? 'AVAX_AVAX' : asset.symbol // AVAX native token has no prefix for buy
  }

  const rampNetworkPrefix = getRampNetworkPrefix(asset.chainId, isOfframp)
  return rampNetworkPrefix !== ''
    ? `${rampNetworkPrefix}_${asset.symbol.toUpperCase()}`
    : asset.symbol
}

export const auroraSupportedContractAddresses = [
  '0x7fc66500c84a76ad7e9c93437bfc5ac33e2ddae9', // AAVE
  '0xaaaaaa20d9e0e2461697782ef11675f668207961', // AURORA
  '0xba100000625a3754423978a60c9317c58a424e3d', // BAL
  '0x0d8775f648430679a709e98d2b0cb6250d2887ef', // BAT
  '0xc00e94cb662c3520282e6f5717214004a7f26888', // COMP
  '0x2ba592f78db6436527729929aaf6c908497cb200', // CREAM
  '0x6b175474e89094c44da98b954eedeac495271d0f', // DAI
  '0x43dfc4159d86f3a37a5a4b3d4580b888ad7d4ddd', // DODO
  '0x3ea8ea4237344c9931214796d9417af1a1180770', // FLX
  '0x853d955acef822db058eb8505911ed77f175b99e', // FRAX
  '0x3432b6a60d23ca0dfca7761b7ab56459d9c964d0', // FXS
  '0xd9c2d319cd7e6177336b0a9c93c21cb48d84fb54', // HAPI
  '0x514910771af9ca656af840dff83e8264ecf986ca', // LINK
  '0x9f8f72aa9304c8b593d555f12ef6589cc3a579a2', // MKR
  '0x1117ac6ad6cdf1a3bc543bad3b133724620522d5', // MODA
  '0xf5cfbc74057c610c8ef151a439252680ac68c6dc', // OCT
  '0x9aeb50f542050172359a0e1a25a9933bc8c01259', // OIN
  '0xea7cc765ebc94c4805e3bff28d7e4ae48d06468a', // PAD
  '0x429881672b9ae42b8eba0e26cd9c73711b891ca5', // PICKLE
  '0x408e41876cccdc0f92210600ef50372656052a38', // REN
  '0xc011a73ee8576fb46f5e1c5751ca3b9fe0af2a6f', // SNX
  '0x6b3595068778dd592e39a122f4f5a5cf09c90fe2', // SUSHI
  '0x1f9840a85d5af5bf1d1762f925bdaddc4201f984', // UNI
  '0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48', // USDC
  '0xdac17f958d2ee523a2206206994597c13d831ec7', // USDT
  '0x2260fac5e5542a773aa44fbcfedf7c193bc2c599', // WBTC
  '0x4691937a7508860f876c9c0a2a617e7d9e945d4b', // WOO
  '0x0bc529c00c6401aef6d220be8c6ea1667f6ad93e' // YFI
].map((contractAddress) => contractAddress.toLowerCase())

export const addChainIdToToken = (
  token: BraveWallet.BlockchainToken,
  chainId: string
) => {
  try {
    token.chainId = chainId
    return token
  } catch {
    // the token object was immutable, return a new token object
    return {
      ...token,
      chainId: chainId
    }
  }
}

export const addLogoToToken = (
  token: BraveWallet.BlockchainToken,
  logo: string
) => {
  try {
    token.logo = logo
    return token
  } catch {
    // the token object was immutable, return a new token object
    return {
      ...token,
      logo: logo
    }
  }
}

export const batSymbols = ['bat', 'wbat', 'bat.e'] as const
export type BatSymbols = (typeof batSymbols)[number]

export const isBat = ({
  symbol
}: Pick<BraveWallet.BlockchainToken, 'symbol'>) => {
  return batSymbols.includes(symbol.toLowerCase() as BatSymbols)
}

/**
 * alphabetically sorts tokens in this order:
 *  1. Gas
 *  2. BAT
 *  3. non-gas, non-BAT
 */
export const sortNativeAndAndBatAssetsToTop = (
  tokenList: BraveWallet.BlockchainToken[]
) => {
  return [...tokenList].sort((a, b) => {
    // check if Gas/Fee token
    const nativeSort = Number(isNativeAsset(b)) - Number(isNativeAsset(a))
    if (nativeSort !== 0) {
      return nativeSort
    }

    // check if BAT
    const batSort = Number(isBat(b)) - Number(isBat(a))
    if (batSort !== 0) {
      return batSort
    }

    // sort alphabetically
    return a.name.localeCompare(b.name)
  })
}

export type GetBlockchainTokenIdArg = Pick<
  BraveWallet.BlockchainToken,
  'coin' | 'chainId' | 'contractAddress' | 'isErc721' |
  'tokenId' | 'isNft' | 'isShielded'
>

/**
 * @param asset The token to get an id for
 * @returns an id that can be used as a react element key
 */
export const getAssetIdKey = (
  asset: Pick<
    GetBlockchainTokenIdArg,
    'contractAddress' | 'chainId' | 'tokenId' | 'coin' | 'isShielded'
  >
) => {
  return asset.tokenId
    ? `${asset.coin}-${asset.contractAddress.toLowerCase()}-${asset.tokenId}-${
      asset.chainId}-${asset.isShielded}`
    : `${asset.coin}-${asset.contractAddress.toLowerCase()}-${asset.chainId}-${
      asset.isShielded}`
}

export const findTokenByContractAddress = <
  T extends Pick<BraveWallet.BlockchainToken, 'contractAddress'>
>(
  contractAddress: string,
  tokensList: T[]
): T | undefined => {
  return tokensList.find(
    (token) =>
      token.contractAddress.toLowerCase() === contractAddress.toLowerCase()
  )
}

export const findTokenBySymbol = (
  tokenSymbol: string,
  tokensList: BraveWallet.BlockchainToken[]
) => {
  return tokensList.find(
    (token) => token.symbol.toLowerCase() === tokenSymbol.toLowerCase()
  )
}

export const findTokenByAssetId = <
  T extends Pick<
    BraveWallet.BlockchainToken,
    'contractAddress' | 'chainId' | 'tokenId' | 'coin' | 'isShielded'
  >
>(
  assetId: string,
  tokensList: T[]
) => {
  return tokensList.find((t) => getAssetIdKey(t) === assetId)
}

export const isNativeAsset = (
  token: Pick<BraveWallet.BlockchainToken, 'contractAddress' | 'isShielded'>
) => token.contractAddress === ''

export const formatTokenBalance = (
  tokenBalanceString: string | undefined,
  selectedAsset:
    | Pick<BraveWallet.BlockchainToken, 'decimals' | 'symbol'>
    | undefined,
  decimalPlaces?: number
): string => {
  return tokenBalanceString
    ? new Amount(tokenBalanceString ?? '')
        .divideByDecimals(selectedAsset?.decimals ?? 18)
        .formatAsAsset(decimalPlaces ?? 6, selectedAsset?.symbol ?? '')
    : ''
}

export const checkIfTokensMatch = (
  tokenOne: BraveWallet.BlockchainToken,
  tokenTwo: BraveWallet.BlockchainToken
): boolean => {
  return (
    tokenOne.symbol.toLowerCase() === tokenTwo.symbol.toLowerCase() &&
    tokenOne.contractAddress.toLowerCase() ===
      tokenTwo.contractAddress.toLowerCase() &&
    tokenOne.chainId === tokenTwo.chainId &&
    tokenOne.tokenId === tokenTwo.tokenId
  )
}

export function filterTokensByNetworks(
  assets: BraveWallet.BlockchainToken[],
  networks: BraveWallet.NetworkInfo[]
) {
  return assets.filter((asset) =>
    networks.some(
      (network) =>
        asset.chainId === network.chainId && asset.coin === network.coin
    )
  )
}

export const checkIfTokenNeedsNetworkIcon = (
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'symbol'>,
  contractAddress: string
) => {
  return (
    contractAddress !== '' || // non-native asset
    // Checks if the network is not the official Ethereum network,
    // but uses ETH as gas.
    getNetworkLogo(network.chainId, network.symbol) !==
      makeNativeAssetLogo(network.symbol, network.chainId)
  )
}

/**
 * Evaluates support for stripe
 * @returns Boolean indicating stripe support
 */
export const isStripeSupported = () =>
  navigator.language.toLowerCase() === 'en-us'

const idWithHashRegexp = new RegExp(/#(\d+)$/)
const idWithSpaceRegexp = new RegExp(/ (\d+)$/)

/** Attempts to remove the token-Id from the NFT name. Useful fro grouping NFTS
 * into like-kinds */
export function tokenNameToNftCollectionName(
  token: BraveWallet.BlockchainToken
) {
  if (token.name.match(idWithHashRegexp)) {
    return token.name.replace(idWithHashRegexp, '')
  }

  if (token.name.match(idWithSpaceRegexp)) {
    return token.name.replace(idWithSpaceRegexp, '')
  }

  return token.name || token.symbol
}

export const getHiddenTokenIds = (): string[] => {
  return JSON.parse(
    localStorage.getItem(LOCAL_STORAGE_KEYS.USER_HIDDEN_TOKEN_IDS) || '[]'
  )
}

export const getDeletedTokenIds = (): string[] => {
  return JSON.parse(
    localStorage.getItem(LOCAL_STORAGE_KEYS.USER_DELETED_TOKEN_IDS) || '[]'
  )
}

export const getHiddenOrDeletedTokenIdsList = () => {
  return getDeletedTokenIds().concat(getHiddenTokenIds())
}

export const isTokenIdRemoved = (tokenId: string, removedIds: string[]) => {
  return removedIds.includes(tokenId)
}

export function makeCountCollectionAssetsInRegistry(
  collectionAssetsRegistry: Record<
    string, // collection name
    BraveWallet.BlockchainToken[]
  >
): (
  total: number,
  currentCollectionToken: BraveWallet.BlockchainToken,
  currentIndex: number,
  array: BraveWallet.BlockchainToken[]
) => number {
  return (acc, collection) => {
    return acc + (collectionAssetsRegistry[collection.name]?.length ?? 0)
  }
}

export function groupSpamAndNonSpamNfts(nfts: BraveWallet.BlockchainToken[]) {
  const results: {
    visibleUserNonSpamNfts: BraveWallet.BlockchainToken[]
    visibleUserMarkedSpamNfts: BraveWallet.BlockchainToken[]
  } = {
    visibleUserNonSpamNfts: [],
    visibleUserMarkedSpamNfts: []
  }
  for (const nft of nfts) {
    if (nft.isSpam) {
      results.visibleUserMarkedSpamNfts.push(nft)
    } else {
      if (nft.visible) {
        results.visibleUserNonSpamNfts.push(nft)
      }
    }
  }
  return results
}

export function getAllSpamNftsAndIds(
  userNonSpamNftIds: string[],
  hiddenNftsIds: string[],
  deletedTokenIds: string[],
  simpleHashSpamNfts: BraveWallet.BlockchainToken[],
  visibleUserMarkedSpamNfts: BraveWallet.BlockchainToken[]
) {
  // filter out NFTs user has marked not spam
  // hidden NFTs, and deleted NFTs
  const excludedNftIds = userNonSpamNftIds
    .concat(hiddenNftsIds)
    .concat(deletedTokenIds)
  const simpleHashList = simpleHashSpamNfts.filter(
    (nft) => !excludedNftIds.includes(getAssetIdKey(nft))
  )
  const simpleHashListIds = simpleHashList.map((nft) => getAssetIdKey(nft))
  // add NFTs user has marked as NFT if they are not in the list
  // to avoid duplicates
  const fullSpamList = simpleHashList.concat(
    visibleUserMarkedSpamNfts.filter(
      (nft) => !simpleHashListIds.includes(getAssetIdKey(nft))
    )
  )

  return [fullSpamList, fullSpamList.map((nft) => getAssetIdKey(nft))] as const
}

export const compareTokensByName = (
  a: Pick<BraveWallet.BlockchainToken, 'name'>,
  b: Pick<BraveWallet.BlockchainToken, 'name'>
) => a.name.localeCompare(b.name)

export function isTokenWatchOnly(
  token: BraveWallet.BlockchainToken,
  allAccounts: BraveWallet.AccountInfo[],
  tokenBalancesRegistry: TokenBalancesRegistry | null | undefined,
  spamTokenBalancesRegistry: TokenBalancesRegistry | null | undefined
) {
  return !allAccounts.some((account) => {
    const balance = getBalance(account.accountId, token, tokenBalancesRegistry)
    const spamBalance = getBalance(
      account.accountId,
      token,
      spamTokenBalancesRegistry
    )
    return (balance && balance !== '0') || (spamBalance && spamBalance !== '0')
  })
}

export function getTokensWithBalanceForAccounts(
  tokens: BraveWallet.BlockchainToken[],
  filteredAccounts: BraveWallet.AccountInfo[],
  allAccounts: BraveWallet.AccountInfo[],
  tokenBalancesRegistry: TokenBalancesRegistry | null | undefined,
  spamTokenBalancesRegistry: TokenBalancesRegistry | null | undefined,
  hideUnowned?: boolean
) {
  if (hideUnowned) {
    return tokens.filter((token) => {
      return filteredAccounts.some((account) => {
        const balance = getBalance(
          account.accountId,
          token,
          tokenBalancesRegistry
        )
        const spamBalance = getBalance(
          account.accountId,
          token,
          spamTokenBalancesRegistry
        )
        return (
          (balance && balance !== '0') || (spamBalance && spamBalance !== '0')
        )
      })
    })
  }

  // skip balance checks if all accounts are selected
  if (filteredAccounts.length === allAccounts.length) {
    return tokens
  }

  return tokens.filter((token) => {
    return (
      filteredAccounts.some((account) => {
        const balance = getBalance(
          account.accountId,
          token,
          tokenBalancesRegistry
        )
        const spamBalance = getBalance(
          account.accountId,
          token,
          spamTokenBalancesRegistry
        )
        return (
          (balance && balance !== '0') || (spamBalance && spamBalance !== '0')
        )
      }) ||
      // not owned by any account
      !allAccounts.some((account) => {
        const balance = getBalance(
          account.accountId,
          token,
          tokenBalancesRegistry
        )
        const spamBalance = getBalance(
          account.accountId,
          token,
          spamTokenBalancesRegistry
        )
        return (
          (balance && balance !== '0') || (spamBalance && spamBalance !== '0')
        )
      })
    )
  })
}

export const searchNfts = (
  searchValue: string,
  items: BraveWallet.BlockchainToken[]
) => {
  if (searchValue === '') {
    return items
  }

  return items.filter((item) => {
    const tokenId = new Amount(item.tokenId).toNumber().toString()
    const searchValueLower = searchValue.toLowerCase()
    return (
      item.name.toLocaleLowerCase().includes(searchValueLower) ||
      item.symbol.toLocaleLowerCase().includes(searchValueLower) ||
      tokenId.includes(searchValueLower)
    )
  })
}

export const searchNftCollectionsAndGetTotalNftsFound = (
  searchValue: string,
  collections: BraveWallet.BlockchainToken[],
  collectionAssetsRegistry: Record<
    string, // collection name
    BraveWallet.BlockchainToken[]
  >
): {
  foundCollections: BraveWallet.BlockchainToken[]
  totalNftsFound: number
} => {
  const searchValueLower = searchValue.toLowerCase().trim()

  const countCollectionAssets = makeCountCollectionAssetsInRegistry(
    collectionAssetsRegistry
  )

  if (searchValueLower === '') {
    return {
      foundCollections: collections,
      // count all nfts in categories
      totalNftsFound: collections.reduce(countCollectionAssets, 0)
    }
  }

  const foundCollections = collections.filter((collection) => {
    // search collection name first
    if (collection.name.toLocaleLowerCase().includes(searchValueLower)) {
      return true
    }

    // search collection assets and count how many NFTs were found
    return collectionAssetsRegistry[collection.name]?.some((asset) => {
      const tokenId = new Amount(asset.tokenId).toNumber().toString()
      return (
        asset.name.toLocaleLowerCase().includes(searchValueLower) ||
        asset.symbol.toLocaleLowerCase().includes(searchValueLower) ||
        tokenId.includes(searchValueLower) ||
        asset.contractAddress.toLocaleLowerCase().startsWith(searchValueLower)
      )
    })
  })

  return {
    foundCollections,
    totalNftsFound: foundCollections.reduce(countCollectionAssets, 0)
  }
}

export function getTokenCollectionName(
  collectionNames: string[],
  assetIdsByCollectionNameRegistry:
    | AssetIdsByCollectionNameRegistry
    | undefined,
  token: BraveWallet.BlockchainToken
) {
  if (!assetIdsByCollectionNameRegistry) {
    return tokenNameToNftCollectionName(token)
  }

  return (
    collectionNames.find((collectionName) => {
      return assetIdsByCollectionNameRegistry[collectionName].includes(
        // token id is not used in the collection name registry to reduce size
        getAssetIdKey({ ...token, tokenId: '' })
      )
    }) || tokenNameToNftCollectionName(token)
  )
}

export function getCoinTypeName(coin: BraveWallet.CoinType) {
  switch (coin) {
    case BraveWallet.CoinType.FIL:
      return 'Filecoin'
    case BraveWallet.CoinType.ETH:
      return 'Ethereum'
    case BraveWallet.CoinType.SOL:
      return 'Solana'
    case BraveWallet.CoinType.BTC:
      return 'Bitcoin'
    case BraveWallet.CoinType.ZEC:
      return 'ZCash'
  }
  return ''
}

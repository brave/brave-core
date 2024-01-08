// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'
import { getAssetIdKey } from '../../utils/asset-utils'

// Icons
import MoonCatIcon from '../../assets/png-icons/mooncat.png'
import { ALGOIconUrl, BATIconUrl, USDCIconUrl, ZRXIconUrl } from './asset-icons'
import {
  ETHIconUrl,
  SOLIconUrl,
  FILECOINIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ZECIconUrl
} from '../../assets/network_token_icons/network_token_icons'

export const mockEthToken = {
  contractAddress: '',
  name: 'Ethereum',
  symbol: 'ETH',
  logo: ETHIconUrl,
  isErc20: false,
  isErc721: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: 'ethereum',
  chainId: '0x1',
  coin: BraveWallet.CoinType.ETH
} as BraveWallet.BlockchainToken

export const mockBtcToken = {
  contractAddress: '',
  name: 'Bitcoin',
  symbol: 'BTC',
  logo: BTCIconUrl,
  isErc20: false,
  isErc721: false,
  isNft: false,
  isSpam: false,
  decimals: 8,
  visible: true,
  tokenId: '',
  coingeckoId: 'bitcoin',
  chainId: BraveWallet.BITCOIN_MAINNET,
  coin: BraveWallet.CoinType.BTC
} as BraveWallet.BlockchainToken

export const mockZecToken = {
  contractAddress: '',
  name: 'ZCash',
  symbol: 'ZEC',
  logo: ZECIconUrl,
  isErc20: false,
  isErc721: false,
  isNft: false,
  isSpam: false,
  decimals: 8,
  visible: true,
  tokenId: '',
  coingeckoId: 'zcash',
  chainId: BraveWallet.Z_CASH_MAINNET,
  coin: BraveWallet.CoinType.ZEC
} as BraveWallet.BlockchainToken

export const mockSolToken = {
  contractAddress: '',
  name: 'Solana',
  symbol: 'SOL',
  logo: SOLIconUrl,
  isErc20: false,
  isErc721: false,
  isNft: false,
  isSpam: false,
  decimals: 9,
  visible: true,
  tokenId: '',
  coingeckoId: 'solana',
  chainId: BraveWallet.SOLANA_MAINNET,
  coin: BraveWallet.CoinType.SOL
} as BraveWallet.BlockchainToken

export const mockFilToken = {
  contractAddress: '',
  name: 'Filecoin',
  symbol: 'FIL',
  logo: FILECOINIconUrl,
  isErc20: false,
  isErc721: false,
  isNft: false,
  isSpam: false,
  decimals: 9,
  visible: true,
  tokenId: '',
  coingeckoId: 'filecoin',
  chainId: BraveWallet.FILECOIN_MAINNET,
  coin: BraveWallet.CoinType.FIL
} as BraveWallet.BlockchainToken

export const mockBasicAttentionToken = {
  contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
  name: 'Basic Attention Token',
  symbol: 'BAT',
  logo: BATIconUrl,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockBinanceCoinErc20Token = {
  contractAddress: '0xB8c77482e45F1F44dE1745F52C74426C631bDD52',
  name: 'Binance Coin',
  symbol: 'BNB',
  logo: BNBIconUrl,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockBitcoinErc20Token = {
  contractAddress: '0x2260fac5e5542a773aa44fbcfedf7c193bc2c599',
  name: 'Bitcoin',
  symbol: 'BTC',
  logo: BTCIconUrl,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 8,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockAlgorandErc20Token = {
  contractAddress: '5',
  name: 'Algorand',
  symbol: 'ALGO',
  logo: ALGOIconUrl,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 8,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockZrxErc20Token = {
  contractAddress: '0xE41d2489571d322189246DaFA5ebDe1F4699F498',
  name: '0x',
  symbol: 'ZRX',
  logo: ZRXIconUrl,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockDaiToken = {
  coingeckoId: 'dai',
  contractAddress: '0xad6d458402f60fd3bd25163575031acdce07538d',
  decimals: 18,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  logo: '',
  name: 'DAI Stablecoin',
  symbol: 'DAI',
  tokenId: '',
  visible: true,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockUSDCoin = {
  coingeckoId: 'usd-coin',
  contractAddress: '0x07865c6e87b9f70255377e024ace6630c1eaa37f',
  decimals: 6,
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  logo: USDCIconUrl,
  name: 'USD Coin',
  symbol: 'USDC',
  tokenId: '',
  visible: true,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockMoonCatNFT = {
  contractAddress: '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69',
  name: 'MoonCats',
  symbol: 'AMC',
  logo: MoonCatIcon,
  isErc20: false,
  isErc721: true,
  isErc1155: false,
  isNft: true,
  isSpam: false,
  decimals: 0,
  visible: true,
  tokenId: '0x42a5',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockERC20Token: BraveWallet.BlockchainToken = {
  contractAddress: 'mockContractAddress',
  name: 'Dog Coin',
  symbol: 'DOG',
  logo: '',
  isErc20: true,
  isErc721: false,
  isErc1155: false,
  isNft: false,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockErc721Token: BraveWallet.BlockchainToken = {
  contractAddress: '0x59468516a8259058bad1ca5f8f4bff190d30e066',
  name: 'Invisible Friends',
  symbol: 'INVSBLE',
  logo: 'https://ipfs.io/ipfs/QmX4nfgA35MiW5APoc4P815hMcH8hAt7edi5H3wXkFm485/2D/2585.gif',
  isErc20: false,
  isErc721: true,
  isErc1155: false,
  isNft: true,
  isSpam: false,
  decimals: 18,
  visible: true,
  tokenId: '0x0a19',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

export const mockSplNft: BraveWallet.BlockchainToken = {
  contractAddress: 'wt1PhURTzRSgmWKHBEJgSX8hN9TdkdNoKhPAnwCmnZE',
  name: 'The Degen #2314',
  symbol: 'BNFT',
  logo: 'https://shdw-drive.genesysgo.net/FR3sEzyAmQMooUYhcPPnN4TmVLSZWi3cEwAWpB4nJvYJ/image-2.png',
  isErc20: false,
  isErc721: false,
  isErc1155: false,
  isNft: true,
  isSpam: false,
  decimals: 1,
  visible: true,
  tokenId: 'wt1PhURTzRSgmWKHBEJgSX8hN9TdkdNoKhPAnwCmnZE',
  coingeckoId: '',
  coin: BraveWallet.CoinType.SOL,
  chainId: BraveWallet.SOLANA_MAINNET
}

export const mockSplBat = {
  ...mockBasicAttentionToken,
  contractAddress: 'splBat498tu349u498j',
  chainId: BraveWallet.SOLANA_MAINNET
}

const mockSplUSDC = {
  ...mockUSDCoin,
  contractAddress: 'splusd09856080378450y75',
  chainId: BraveWallet.SOLANA_MAINNET
}

export const mockAccountAssetOptions: BraveWallet.BlockchainToken[] = [
  mockEthToken,
  mockSolToken,
  mockFilToken,
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockBitcoinErc20Token,
  mockAlgorandErc20Token,
  mockZrxErc20Token,
  mockDaiToken,
  mockUSDCoin,
  mockSplBat,
  mockSplNft,
  mockSplUSDC
]

export const mockErc20TokensList = [
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockBitcoinErc20Token,
  mockAlgorandErc20Token,
  mockZrxErc20Token,
  mockDaiToken,
  mockUSDCoin
]

export const mockNewAssetOptions: BraveWallet.BlockchainToken[] = [
  mockEthToken,
  {
    contractAddress: '2',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: BATIconUrl,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1'
  },
  {
    contractAddress: '3',
    name: 'Binance Coin',
    symbol: 'BNB',
    logo: BNBIconUrl,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x5'
  },
  {
    contractAddress: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    logo: BTCIconUrl,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1'
  },
  {
    contractAddress: '5',
    name: 'Algorand',
    symbol: 'ALGO',
    logo: ALGOIconUrl,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1'
  },
  {
    contractAddress: '6',
    name: '0x',
    symbol: 'ZRX',
    logo: ZRXIconUrl,
    isErc20: true,
    isErc721: false,
    isErc1155: false,
    isNft: false,
    isSpam: false,
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x1'
  },
  mockMoonCatNFT,
  { ...mockMoonCatNFT, tokenId: '0x52a5' },
  { ...mockMoonCatNFT, tokenId: '0x62a5' }
]

export const mockBinanceCoinErc20TokenId = getAssetIdKey(
  mockBinanceCoinErc20Token
)
export const mockBitcoinErc20TokenId = getAssetIdKey(mockBitcoinErc20Token)
export const mockAlgorandErc20TokenId = getAssetIdKey(mockAlgorandErc20Token)
export const mockZrxErc20TokenId = getAssetIdKey(mockZrxErc20Token)
export const mockDaiTokenId = getAssetIdKey(mockDaiToken)
export const mockSplNftId = getAssetIdKey(mockSplNft)

export const mockBasicAttentionTokenId = getAssetIdKey(mockBasicAttentionToken)
export const mockUSDCoinId = getAssetIdKey(mockUSDCoin)

export const mockSplBasicAttentionTokenId = getAssetIdKey(mockSplBat)
export const mockSplUSDCoinId = getAssetIdKey(mockSplUSDC)

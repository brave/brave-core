// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BraveWallet } from '../../constants/types'
import {
  ALGOIconUrl,
  BATIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ETHIconUrl,
  USDCIconUrl,
  ZRXIconUrl
} from './asset-icons'
import MoonCatIcon from '../../assets/png-icons/mooncat.png'

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
  coingeckoId: '',
  chainId: '0x1',
  coin: BraveWallet.CoinType.ETH
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
  logo: 'chrome://erc-token-images/dai.png',
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

export const mockAccountAssetOptions: BraveWallet.BlockchainToken[] = [
  mockEthToken,
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockBitcoinErc20Token,
  mockAlgorandErc20Token,
  mockZrxErc20Token
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

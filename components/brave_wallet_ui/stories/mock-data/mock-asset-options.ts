import { BraveWallet } from '../../constants/types'
import { ETH } from '../../options/asset-options'
import {
  ALGOIconUrl,
  BATIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ZRXIconUrl
} from '../../assets/asset-icons'
import MoonCatIcon from '../../assets/png-icons/mooncat.png'

export const mockEthToken = ETH

export const mockBasicAttentionToken = {
  contractAddress: '0x0D8775F648430679A709E98d2b0Cb6250d2887EF',
  name: 'Basic Attention Token',
  symbol: 'BAT',
  logo: BATIconUrl,
  isErc20: true,
  isErc721: false,
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
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockBitcoinErc20Token = {
  contractAddress: '4',
  name: 'Bitcoin',
  symbol: 'BTC',
  logo: BTCIconUrl,
  isErc20: true,
  isErc721: false,
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
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockMoonCatNFT = {
  contractAddress: '0xc3f733ca98E0daD0386979Eb96fb1722A1A05E69',
  name: 'MoonCats',
  symbol: 'AMC',
  logo: MoonCatIcon,
  isErc20: false,
  isErc721: true,
  decimals: 0,
  visible: true,
  tokenId: '0x42a5',
  coingeckoId: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: '0x1'
}

export const mockAccountAssetOptions: BraveWallet.BlockchainToken[] = [
  ETH,
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token,
  mockBitcoinErc20Token,
  mockAlgorandErc20Token,
  mockZrxErc20Token
]

export const mockNewAssetOptions: BraveWallet.BlockchainToken[] = [
  ETH,
  {
    contractAddress: '2',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: BATIconUrl,
    isErc20: true,
    isErc721: false,
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
    decimals: 18,
    visible: true,
    tokenId: '',
    coingeckoId: '',
    coin: BraveWallet.CoinType.ETH,
    chainId: '0x3'
  },
  {
    contractAddress: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    logo: BTCIconUrl,
    isErc20: true,
    isErc721: false,
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

import { AccountAssetOptionType, AssetOptionType } from '../constants/types'
import {
  ETHIconUrl
} from '../assets/asset-icons'
import {
  CrvIconURL,
  MusdcIconURL,
  PaxIconURL,
  PdaiIconURL
} from '../assets/wyre-asset-icons'

export const WyreAssetOptions: AssetOptionType[] = [
  {
    id: '1',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    logo: 'chrome://erc-token-images/bat.png'
  },
  {
    id: '2',
    name: 'Ethereum',
    symbol: 'ETH',
    logo: ETHIconUrl
  },
  {
    id: '3',
    name: 'USD Coin',
    symbol: 'USDC',
    logo: 'chrome://erc-token-images/usdc.png'
  },
  {
    id: '4',
    name: 'DAI',
    symbol: 'DAI',
    logo: 'chrome://erc-token-images/dai.png'
  },
  {
    id: '5',
    name: 'AAVE',
    symbol: 'AAVE',
    logo: 'chrome://erc-token-images/AAVE.png'
  },
  {
    id: '6',
    name: 'Binance USD',
    symbol: 'BUSD',
    logo: 'chrome://erc-token-images/busd.png'
  },
  {
    id: '7',
    name: 'Compound',
    symbol: 'COMP',
    logo: 'chrome://erc-token-images/comp.png'
  },
  {
    id: '8',
    name: 'Curve',
    symbol: 'CRV',
    logo: CrvIconURL
  },
  {
    id: '9',
    name: 'Gemini Dollar',
    symbol: 'GUSD',
    logo: 'chrome://erc-token-images/gusd.png'
  },
  {
    id: '10',
    name: 'Chainlink',
    symbol: 'LINK',
    logo: 'chrome://erc-token-images/chainlink.png'
  },
  {
    id: '11',
    name: 'Maker',
    symbol: 'MKR',
    logo: 'chrome://erc-token-images/mkr.png'
  },
  {
    id: '12',
    name: 'Paxos Standard',
    symbol: 'PAX',
    logo: PaxIconURL
  },
  {
    id: '13',
    name: 'Synthetix',
    symbol: 'SNX',
    logo: 'chrome://erc-token-images/synthetix.png'
  },
  {
    id: '14',
    name: 'UMA',
    symbol: 'UMA',
    logo: 'chrome://erc-token-images/UMA.png'
  },
  {
    id: '15',
    name: 'Uniswap',
    symbol: 'UNI',
    logo: 'chrome://erc-token-images/uni.png'
  },
  {
    id: '16',
    name: 'Stably Dollar',
    symbol: 'USDS',
    logo: 'chrome://erc-token-images/usds.png'
  },
  {
    id: '17',
    name: 'Tether',
    symbol: 'USDT',
    logo: 'chrome://erc-token-images/usdt.png'
  },
  {
    id: '18',
    name: 'Wrapped Bitcoin',
    symbol: 'WBTC',
    logo: 'chrome://erc-token-images/wbtc.png'
  },
  {
    id: '19',
    name: 'Yearn.Fianance',
    symbol: 'YFI',
    logo: 'chrome://erc-token-images/yfi.png'
  },
  {
    id: '20',
    name: 'Palm DAI',
    symbol: 'PDAI',
    logo: PdaiIconURL
  },
  {
    id: '21',
    name: 'Matic USDC',
    symbol: 'MUSDC',
    logo: MusdcIconURL
  }
]

export const WyreAccountAssetOptions: AccountAssetOptionType[] = [
  {
    asset: {
      contractAddress: '1',
      name: 'Basic Attention Token',
      symbol: 'BAT',
      logo: 'chrome://erc-token-images/bat.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '2',
      name: 'Ethereum',
      symbol: 'ETH',
      logo: ETHIconUrl,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '3',
      name: 'USD Coin',
      symbol: 'USDC',
      logo: 'chrome://erc-token-images/usdc.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '4',
      name: 'DAI',
      symbol: 'DAI',
      logo: 'chrome://erc-token-images/dai.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '5',
      name: 'AAVE',
      symbol: 'AAVE',
      logo: 'chrome://erc-token-images/AAVE.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '6',
      name: 'Binance USD',
      symbol: 'BUSD',
      logo: 'chrome://erc-token-images/busd.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '7',
      name: 'Compound',
      symbol: 'COMP',
      logo: 'chrome://erc-token-images/comp.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '8',
      name: 'Curve',
      symbol: 'CRV',
      logo: CrvIconURL,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '9',
      name: 'Gemini Dollar',
      symbol: 'GUSD',
      logo: 'chrome://erc-token-images/gusd.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '10',
      name: 'Chainlink',
      symbol: 'LINK',
      logo: 'chrome://erc-token-images/chainlink.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '11',
      name: 'Maker',
      symbol: 'MKR',
      logo: 'chrome://erc-token-images/mkr.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '12',
      name: 'Paxos Standard',
      symbol: 'PAX',
      logo: PaxIconURL,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '13',
      name: 'Synthetix',
      symbol: 'SNX',
      logo: 'chrome://erc-token-images/synthetix.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '14',
      name: 'UMA',
      symbol: 'UMA',
      logo: 'chrome://erc-token-images/UMA.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '15',
      name: 'Uniswap',
      symbol: 'UNI',
      logo: 'chrome://erc-token-images/uni.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '16',
      name: 'Stably Dollar',
      symbol: 'USDS',
      logo: 'chrome://erc-token-images/usds.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '17',
      name: 'Tether',
      symbol: 'USDT',
      logo: 'chrome://erc-token-images/usdt.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '18',
      name: 'Wrapped Bitcoin',
      symbol: 'WBTC',
      logo: 'chrome://erc-token-images/wbtc.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '19',
      name: 'Yearn.Fianance',
      symbol: 'YFI',
      logo: 'chrome://erc-token-images/yfi.png',
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '20',
      name: 'Palm DAI',
      symbol: 'PDAI',
      logo: PdaiIconURL,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  },
  {
    asset: {
      contractAddress: '21',
      name: 'Matic USDC',
      symbol: 'MUSDC',
      logo: MusdcIconURL,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  }
]

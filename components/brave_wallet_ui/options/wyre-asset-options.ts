import { AccountAssetOptionType, AssetOptionType } from '../constants/types'
import {
  ETHIconUrl,
  BATIconUrl
} from '../assets/asset-icons'
import {
  AaveIconURL,
  BusdIconURL,
  CompIconURL,
  CrvIconURL,
  DaiIconURL,
  GusdIconURL,
  LinkIconURL,
  MkrIconURL,
  MusdcIconURL,
  PaxIconURL,
  PdaiIconURL,
  SnxIconURL,
  UmaIconURL,
  UniIconURL,
  UsdcIconURL,
  UsdsIconURL,
  UsdtIconURL,
  WbtcIconURL,
  YfiIconURL
} from '../assets/wyre-asset-icons'

export const WyreAssetOptions: AssetOptionType[] = [
  {
    id: '1',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    icon: BATIconUrl
  },
  {
    id: '2',
    name: 'Ethereum',
    symbol: 'ETH',
    icon: ETHIconUrl
  },
  {
    id: '3',
    name: 'USD Coin',
    symbol: 'USDC',
    icon: UsdcIconURL
  },
  {
    id: '4',
    name: 'DAI',
    symbol: 'DAI',
    icon: DaiIconURL
  },
  {
    id: '5',
    name: 'AAVE',
    symbol: 'AAVE',
    icon: AaveIconURL
  },
  {
    id: '6',
    name: 'Binance USD',
    symbol: 'BUSD',
    icon: BusdIconURL
  },
  {
    id: '7',
    name: 'Compound',
    symbol: 'COMP',
    icon: CompIconURL
  },
  {
    id: '8',
    name: 'Curve',
    symbol: 'CRV',
    icon: CrvIconURL
  },
  {
    id: '9',
    name: 'Gemini Dollar',
    symbol: 'GUSD',
    icon: GusdIconURL
  },
  {
    id: '10',
    name: 'Chainlink',
    symbol: 'LINK',
    icon: LinkIconURL
  },
  {
    id: '11',
    name: 'Maker',
    symbol: 'MKR',
    icon: MkrIconURL
  },
  {
    id: '12',
    name: 'Paxos Standard',
    symbol: 'PAX',
    icon: PaxIconURL
  },
  {
    id: '13',
    name: 'Synthetix',
    symbol: 'SNX',
    icon: SnxIconURL
  },
  {
    id: '14',
    name: 'UMA',
    symbol: 'UMA',
    icon: UmaIconURL
  },
  {
    id: '15',
    name: 'Uniswap',
    symbol: 'UNI',
    icon: UniIconURL
  },
  {
    id: '16',
    name: 'Stably Dollar',
    symbol: 'USDS',
    icon: UsdsIconURL
  },
  {
    id: '17',
    name: 'Tether',
    symbol: 'USDT',
    icon: UsdtIconURL
  },
  {
    id: '18',
    name: 'Wrapped Bitcoin',
    symbol: 'WBTC',
    icon: WbtcIconURL
  },
  {
    id: '19',
    name: 'Yearn.Fianance',
    symbol: 'YFI',
    icon: YfiIconURL
  },
  {
    id: '20',
    name: 'Palm DAI',
    symbol: 'PDAI',
    icon: PdaiIconURL
  },
  {
    id: '21',
    name: 'Matic USDC',
    symbol: 'MUSDC',
    icon: MusdcIconURL
  }
]

export const WyreAccountAssetOptions: AccountAssetOptionType[] = [
  {
    asset: {
      contractAddress: '1',
      name: 'Basic Attention Token',
      symbol: 'BAT',
      icon: BATIconUrl,
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
      icon: ETHIconUrl,
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
      icon: UsdcIconURL,
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
      icon: DaiIconURL,
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
      icon: AaveIconURL,
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
      icon: BusdIconURL,
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
      icon: CompIconURL,
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
      icon: CrvIconURL,
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
      icon: GusdIconURL,
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
      icon: LinkIconURL,
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
      icon: MkrIconURL,
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
      icon: PaxIconURL,
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
      icon: SnxIconURL,
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
      icon: UmaIconURL,
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
      icon: UniIconURL,
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
      icon: UsdsIconURL,
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
      icon: UsdtIconURL,
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
      icon: WbtcIconURL,
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
      icon: YfiIconURL,
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
      icon: PdaiIconURL,
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
      icon: MusdcIconURL,
      isErc20: true,
      isErc721: false,
      decimals: 8
    },
    assetBalance: '0',
    fiatBalance: '0'
  }
]

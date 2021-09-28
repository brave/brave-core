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
    logo: BATIconUrl
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
    logo: UsdcIconURL
  },
  {
    id: '4',
    name: 'DAI',
    symbol: 'DAI',
    logo: DaiIconURL
  },
  {
    id: '5',
    name: 'AAVE',
    symbol: 'AAVE',
    logo: AaveIconURL
  },
  {
    id: '6',
    name: 'Binance USD',
    symbol: 'BUSD',
    logo: BusdIconURL
  },
  {
    id: '7',
    name: 'Compound',
    symbol: 'COMP',
    logo: CompIconURL
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
    logo: GusdIconURL
  },
  {
    id: '10',
    name: 'Chainlink',
    symbol: 'LINK',
    logo: LinkIconURL
  },
  {
    id: '11',
    name: 'Maker',
    symbol: 'MKR',
    logo: MkrIconURL
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
    logo: SnxIconURL
  },
  {
    id: '14',
    name: 'UMA',
    symbol: 'UMA',
    logo: UmaIconURL
  },
  {
    id: '15',
    name: 'Uniswap',
    symbol: 'UNI',
    logo: UniIconURL
  },
  {
    id: '16',
    name: 'Stably Dollar',
    symbol: 'USDS',
    logo: UsdsIconURL
  },
  {
    id: '17',
    name: 'Tether',
    symbol: 'USDT',
    logo: UsdtIconURL
  },
  {
    id: '18',
    name: 'Wrapped Bitcoin',
    symbol: 'WBTC',
    logo: WbtcIconURL
  },
  {
    id: '19',
    name: 'Yearn.Fianance',
    symbol: 'YFI',
    logo: YfiIconURL
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
      logo: BATIconUrl,
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
      logo: UsdcIconURL,
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
      logo: DaiIconURL,
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
      logo: AaveIconURL,
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
      logo: BusdIconURL,
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
      logo: CompIconURL,
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
      logo: GusdIconURL,
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
      logo: LinkIconURL,
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
      logo: MkrIconURL,
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
      logo: SnxIconURL,
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
      logo: UmaIconURL,
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
      logo: UniIconURL,
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
      logo: UsdsIconURL,
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
      logo: UsdtIconURL,
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
      logo: WbtcIconURL,
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
      logo: YfiIconURL,
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

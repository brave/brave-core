import { AssetOptionType } from '../constants/types'
import {
  ALGOIconUrl,
  BATIconUrl,
  BNBIconUrl,
  BTCIconUrl,
  ETHIconUrl,
  ZRXIconUrl
} from '../assets/asset-icons'

export const AssetOptions: AssetOptionType[] = [
  {
    id: '1',
    name: 'Ethereum',
    symbol: 'ETH',
    icon: ETHIconUrl
  },
  {
    id: '2',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    icon: BATIconUrl
  },
  {
    id: '3',
    name: 'Binance Coin',
    symbol: 'BNB',
    icon: BNBIconUrl
  },
  {
    id: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    icon: BTCIconUrl
  },
  {
    id: '5',
    name: 'Algorand',
    symbol: 'ALGO',
    icon: ALGOIconUrl
  },
  {
    id: '6',
    name: '0x',
    symbol: 'ZRX',
    icon: ZRXIconUrl
  }
]

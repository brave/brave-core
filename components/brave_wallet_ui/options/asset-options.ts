import { AssetOptionType } from '../constants/types'
import {
  ALGOIcon,
  BATIcon,
  BNBIcon,
  BTCIcon,
  ETHIcon,
  ZRXIcon
} from '../assets/asset-icons'

export const AssetOptions: AssetOptionType[] = [
  {
    id: '1',
    name: 'Ethereum',
    symbol: 'ETH',
    icon: ETHIcon
  },
  {
    id: '2',
    name: 'Basic Attention Token',
    symbol: 'BAT',
    icon: BATIcon
  },
  {
    id: '3',
    name: 'Binance Coin',
    symbol: 'BNB',
    icon: BNBIcon
  },
  {
    id: '4',
    name: 'Bitcoin',
    symbol: 'BTC',
    icon: BTCIcon
  },
  {
    id: '5',
    name: 'Algorand',
    symbol: 'ALGO',
    icon: ALGOIcon
  },
  {
    id: '6',
    name: '0x',
    symbol: 'ZRX',
    icon: ZRXIcon
  }
]

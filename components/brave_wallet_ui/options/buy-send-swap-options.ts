import { BuySendSwapObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const BuySendSwapOptions = (): BuySendSwapObjectType[] => [
  {
    id: 'buy',
    name: getLocale('braveWalletBuy')
  },
  {
    id: 'send',
    name: getLocale('braveWalletSend')
  },
  {
    id: 'swap',
    name: getLocale('braveWalletSwap')
  }
]

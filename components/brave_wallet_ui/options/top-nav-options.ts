import { TopTabNavObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const TopNavOptions = (): TopTabNavObjectType[] => [
  {
    id: 'portfolio',
    name: getLocale('braveWalletTopNavPortfolio')
  },
  {
    id: 'market',
    name: getLocale('braveWalletTopNavMarket')
  },
  {
    id: 'accounts',
    name: getLocale('braveWalletTopNavAccounts')
  }
  // Temp commented out for MVP
  // {
  //   id: 'apps',
  //   name: getLocale('braveWalletTopTabApps')
  // }
]

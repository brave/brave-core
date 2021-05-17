import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const TopNavOptions: TopTabNavObjectType[] = [
  {
    id: 'portfolio',
    name: locale.topNavPortfolio
  },
  {
    id: 'prices',
    name: locale.topTabPrices
  },
  {
    id: 'defi',
    name: locale.topTabDefi
  },
  {
    id: 'nfts',
    name: locale.topNavNFTS
  },
  {
    id: 'accounts',
    name: locale.topNavAccounts
  }
]

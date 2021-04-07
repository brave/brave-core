import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const TopNavOptions: TopTabNavObjectType[] = [
  {
    id: 'portfolio',
    name: locale.topNavPortfolio
  },
  {
    id: 'nfts',
    name: locale.topNavNFTS
  },
  {
    id: 'invest',
    name: locale.topNavInvest
  },
  {
    id: 'lending',
    name: locale.topNavLending
  },
  {
    id: 'apps',
    name: locale.topNavApps
  },
  {
    id: 'accounts',
    name: locale.topNavAccounts
  }
]

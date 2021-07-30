import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const TopNavOptions: TopTabNavObjectType[] = [
  {
    id: 'portfolio',
    name: locale.topNavPortfolio
  },
  {
    id: 'apps',
    name: locale.topTabApps
  },
  {
    id: 'accounts',
    name: locale.topNavAccounts
  }
]

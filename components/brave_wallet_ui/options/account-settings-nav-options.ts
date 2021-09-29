import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const AccountSettingsNavOptions: TopTabNavObjectType[] = [
  {
    id: 'details',
    name: locale.accountSettingsDetails
  },
  {
    id: 'privateKey',
    name: locale.accountSettingsPrivateKey
  }
]

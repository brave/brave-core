import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const AccountSettingsNavOptions: TopTabNavObjectType[] = [
  {
    id: 'details',
    name: locale.accountSettingsDetails
  }
]

export const ImportedAccountSettingsNavOptions: TopTabNavObjectType[] = [
  ...AccountSettingsNavOptions,
  {
    id: 'privateKey',
    name: locale.accountSettingsPrivateKey
  }
]

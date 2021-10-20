import { TopTabNavObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AccountSettingsNavOptions = (): TopTabNavObjectType[] => [
  {
    id: 'details',
    name: getLocale('braveWalletAccountSettingsDetails')
  },
  {
    id: 'privateKey',
    name: getLocale('braveWalletAccountSettingsPrivateKey')
  }
]

export const HardwareAccountSettingsNavOptions = (): TopTabNavObjectType[] => [
  {
    id: 'details',
    name: getLocale('braveWalletAccountSettingsDetails')
  }
]

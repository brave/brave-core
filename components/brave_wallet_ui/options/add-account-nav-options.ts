import { TopTabNavObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AddAccountNavOptions = (): TopTabNavObjectType[] => [
  {
    id: 'create',
    name: getLocale('braveWalletAddAccountCreate')
  },
  {
    id: 'import',
    name: getLocale('braveWalletAddAccountImport')
  },
  {
    id: 'hardware',
    name: getLocale('braveWalletAddAccountHardware')
  }
]

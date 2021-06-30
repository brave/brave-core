import { TopTabNavObjectType } from '../constants/types'
import locale from '../constants/locale'

export const AddAccountNavOptions: TopTabNavObjectType[] = [
  {
    id: 'create',
    name: locale.addAccountCreate
  },
  {
    id: 'import',
    name: locale.addAccountImport
  },
  {
    id: 'hardware',
    name: locale.addAccountHardware
  }
]

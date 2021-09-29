import { AmountPresetObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AmountPresetOptions = (): AmountPresetObjectType[] => [
  {
    name: getLocale('braveWalletPreset25'),
    id: 0.25
  },
  {
    name: getLocale('braveWalletPreset50'),
    id: 0.5
  },
  {
    name: getLocale('braveWalletPreset75'),
    id: 0.75
  },
  {
    name: getLocale('braveWalletPreset100'),
    id: 1
  }
]

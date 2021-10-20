import { AmountPresetObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AmountPresetOptions = (): AmountPresetObjectType[] => [
  {
    name: getLocale('braveWalletPreset25'),
    value: 0.25
  },
  {
    name: getLocale('braveWalletPreset50'),
    value: 0.5
  },
  {
    name: getLocale('braveWalletPreset75'),
    value: 0.75
  },
  {
    name: getLocale('braveWalletPreset100'),
    value: 1
  }
]

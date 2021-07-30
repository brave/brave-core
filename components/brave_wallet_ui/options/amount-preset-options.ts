import { AmountPresetObjectType } from '../constants/types'
import locale from '../constants/locale'

export const AmountPresetOptions: AmountPresetObjectType[] = [
  {
    name: locale.preset25,
    id: 0.25
  },
  {
    name: locale.preset50,
    id: 0.5
  },
  {
    name: locale.preset75,
    id: 0.75
  },
  {
    name: locale.preset100,
    id: 1
  }
]

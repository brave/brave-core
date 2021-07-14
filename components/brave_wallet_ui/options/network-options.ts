import { NetworkOptionsType } from '../constants/types'
import locale from '../constants/locale'

export const NetworkOptions: NetworkOptionsType[] = [
  {
    id: 0,
    name: `${locale.networkETH} ${locale.networkMain}`,
    abbr: locale.networkMain
  },
  {
    id: 1,
    name: `${locale.networkRopsten} ${locale.networkTest}`,
    abbr: locale.networkRopsten
  },
  {
    id: 2,
    name: `${locale.networkKavan} ${locale.networkTest}`,
    abbr: locale.networkKavan
  },
  {
    id: 3,
    name: `${locale.networkRinkeby} ${locale.networkTest}`,
    abbr: locale.networkRinkeby
  },
  {
    id: 4,
    name: `${locale.networkGoerli} ${locale.networkTest}`,
    abbr: locale.networkGoerli
  },
  {
    id: 5,
    name: locale.networkBinance,
    abbr: locale.networkBinanceAbbr
  }
]

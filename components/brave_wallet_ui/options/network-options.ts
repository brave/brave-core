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
    name: `${locale.networkRinkeby} ${locale.networkTest}`,
    abbr: locale.networkRinkeby
  },
  {
    id: 2,
    name: `${locale.networkRopsten} ${locale.networkTest}`,
    abbr: locale.networkRopsten
  },
  {
    id: 3,
    name: `${locale.networkGoerli} ${locale.networkTest}`,
    abbr: locale.networkGoerli
  },
  {
    id: 4,
    name: `${locale.networkKovan} ${locale.networkTest}`,
    abbr: locale.networkKovan
  },
  {
    id: 5,
    name: locale.networkLocalhost,
    abbr: locale.networkLocalhost
  }
]

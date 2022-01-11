import { CreateAccountOptionsType } from '../constants/types'
import {
  ETHIconUrl
  // SOLIconUrl,
  // FILECOINIconUrl
} from '../assets/asset-icons'
import { getLocale } from '../../common/locale'

export const CreateAccountOptions = (): CreateAccountOptionsType[] => [
  {
    description: getLocale('braveWalletCreateAccountEthereumDescription'),
    name: 'Ethereum',
    network: 'ethereum',
    icon: ETHIconUrl
  }
  // Commented out until we have support for these networks
  // {
  //   description: getLocale('braveWalletCreateAccountSolanaDescription'),
  //   name: 'Solana',
  //   network: 'solana',
  //   icon: SOLIconUrl
  // },
  // {
  //   description: getLocale('braveWalletCreateAccountFilecoinDescription'),
  //   name: 'Filecoin',
  //   network: 'filecoin',
  //   icon: FILECOINIconUrl
  // }
]

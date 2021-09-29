import { NavObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'
import {
  WalletIconD,
  WalletIconL,
  RewardsIconL,
  RewardsIconD,
  CreditCardIconL,
  CreditCardIconD
} from '../assets/svg-icons/nav-button-icons'

export const NavOptions = (): NavObjectType[] => [
  {
    name: getLocale('braveWalletSideNavCrypto'),
    primaryIcon: WalletIconL,
    secondaryIcon: WalletIconD,
    id: 'crypto'
  },
  {
    name: getLocale('braveWalletSideNavRewards'),
    primaryIcon: RewardsIconL,
    secondaryIcon: RewardsIconD,
    id: 'rewards'
  },
  {
    name: getLocale('braveWalletSideNavCards'),
    primaryIcon: CreditCardIconL,
    secondaryIcon: CreditCardIconD,
    id: 'cards'
  }
]

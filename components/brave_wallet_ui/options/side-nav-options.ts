import { NavObjectType } from '../constants/types'
import locale from '../constants/locale'
import {
  WalletIconD,
  WalletIconL,
  RewardsIconL,
  RewardsIconD,
  CreditCardIconL,
  CreditCardIconD
} from '../assets/svg-icons/nav-button-icons'

export const NavOptions: NavObjectType[] = [
  {
    name: locale.sideNavCrypto,
    primaryIcon: WalletIconL,
    secondaryIcon: WalletIconD,
    id: 'crypto'
  },
  {
    name: locale.sideNavRewards,
    primaryIcon: RewardsIconL,
    secondaryIcon: RewardsIconD,
    id: 'rewards'
  },
  {
    name: locale.sideNavCards,
    primaryIcon: CreditCardIconL,
    secondaryIcon: CreditCardIconD,
    id: 'cards'
  }
]

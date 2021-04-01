import { NavObjectType } from '../constants/types'
import locale from './mock-locale'
import {
  WalletIconD,
  WalletIconL,
  RewardsIconL,
  RewardsIconD,
  CreditCardIconL,
  CreditCardIconD,
  PuzzleIconL,
  PuzzleIconD,
  LockIconL,
  LockIconD,
  GeminiIcon
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
  },
  {
    name: locale.sideNavLinkedAccounts,
    primaryIcon: PuzzleIconL,
    secondaryIcon: PuzzleIconD,
    id: 'linked_accounts'
  }
]

export const LinkedAccountsOptions: NavObjectType[] = [
  {
    name: locale.sideNavGemini,
    primaryIcon: GeminiIcon,
    secondaryIcon: GeminiIcon,
    id: 'gemini'
  },
  {
    name: locale.sideNavCreators,
    primaryIcon: RewardsIconL,
    secondaryIcon: RewardsIconD,
    id: 'creators'
  }
]

export const StaticOptions: NavObjectType[] = [
  {
    name: locale.sideNavLockWallet,
    primaryIcon: LockIconL,
    secondaryIcon: LockIconD,
    id: 'lock_wallet'
  }
]

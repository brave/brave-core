import { BraveWallet, CreateAccountOptionsType } from '../constants/types'
import {
  ETHIconUrl,
  SOLIconUrl,
  FILECOINIconUrl
} from '../assets/asset-icons'
import { getLocale } from '../../common/locale'

export const CreateAccountOptions = (isFilecoinEnabled: boolean, isSolanaEnabled: boolean): CreateAccountOptionsType[] => {
  let accounts = [
    {
      description: getLocale('braveWalletCreateAccountEthereumDescription'),
      name: 'Ethereum',
      coin: BraveWallet.CoinType.ETH,
      icon: ETHIconUrl
    }
  ]
  if (isSolanaEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountSolanaDescription'),
      name: 'Solana',
      coin: BraveWallet.CoinType.SOL,
      icon: SOLIconUrl
    })
  }
  if (isFilecoinEnabled) {
    accounts.push({
      description: getLocale('braveWalletCreateAccountFilecoinDescription'),
      name: 'Filecoin',
      coin: BraveWallet.CoinType.FIL,
      icon: FILECOINIconUrl
    })
  }
  return accounts
}

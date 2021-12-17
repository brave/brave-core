import {
  AccountAssetOptionType,
  BraveWallet,
  UserAccountType
} from '../constants/types'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (networkChainId: string, asset: AccountAssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (networkChainId) {
    case BraveWallet.MAINNET_CHAIN_ID:
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case BraveWallet.ROPSTEN_CHAIN_ID:
      return 'https://faucet.ropsten.be/'
    case BraveWallet.KOVAN_CHAIN_ID:
      return 'https://github.com/kovan-testnet/faucet'
    case BraveWallet.RINKEBY_CHAIN_ID:
      return 'https://www.rinkeby.io/'
    case BraveWallet.GOERLI_CHAIN_ID:
      return 'https://goerli-faucet.slock.it/'
    default:
      return ''
  }
}

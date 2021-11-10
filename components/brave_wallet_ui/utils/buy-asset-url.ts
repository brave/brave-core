import { AccountAssetOptionType,
         UserAccountType,
         MAINNET_CHAIN_ID,
         ROPSTEN_CHAIN_ID,
         KOVAN_CHAIN_ID,
         RINKEBY_CHAIN_ID,
         GOERLI_CHAIN_ID } from '../constants/types'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (networkChainId: string, asset: AccountAssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (networkChainId) {
    case MAINNET_CHAIN_ID:
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case ROPSTEN_CHAIN_ID:
      return 'https://faucet.ropsten.be/'
    case KOVAN_CHAIN_ID:
      return 'https://github.com/kovan-testnet/faucet'
    case RINKEBY_CHAIN_ID:
      return 'https://www.rinkeby.io/'
    case GOERLI_CHAIN_ID:
      return 'https://goerli-faucet.slock.it/'
    default:
      return ''
  }
}

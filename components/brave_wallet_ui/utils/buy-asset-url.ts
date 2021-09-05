import { AccountAssetOptionType,
         UserAccountType,
         kMainnetChainId,
         kRopstenChainId,
         kKovanChainId,
         kRinkebyChainId,
         kGoerliChainId } from '../constants/types'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (networkChainId: string, asset: AccountAssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (networkChainId) {
    case kMainnetChainId:
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case kRopstenChainId:
      return 'https://faucet.metamask.io/'
    case kKovanChainId:
      return 'https://github.com/kovan-testnet/faucet'
    case kRinkebyChainId:
      return 'https://www.rinkeby.io/'
    case kGoerliChainId:
      return 'https://goerli-faucet.slock.it/'
    default:
      return ''
  }
}

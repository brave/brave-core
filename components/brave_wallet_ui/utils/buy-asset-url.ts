import { AccountAssetOptionType, UserAccountType } from '../constants/types'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (networkChainId: string, asset: AccountAssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (networkChainId) {
    case '0x1':
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case '0x3':
      return 'https://faucet.metamask.io/'
    case '0x2a':
      return 'https://github.com/kovan-testnet/faucet'
    case '0x4':
      return 'https://www.rinkeby.io/'
    case '0x5':
      return 'https://goerli-faucet.slock.it/'
    default:
      throw new Error(`Unknown cryptocurrency exchange or faucet: "${networkChainId}"`)
  }
}

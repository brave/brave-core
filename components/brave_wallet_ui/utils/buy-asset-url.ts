import { NetworkOptionsType, AssetOptionType, UserAccountType } from '../constants/types'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (network: NetworkOptionsType, asset: AssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (network.id) {
    case 0:
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case 1:
      return 'https://faucet.metamask.io/'
    case 2:
      return 'https://github.com/kovan-testnet/faucet'
    case 3:
      return 'https://www.rinkeby.io/'
    case 4:
      return 'https://goerli-faucet.slock.it/'
    default:
      throw new Error(`Unknown cryptocurrency exchange or faucet: "${network.name}"`)
  }
}

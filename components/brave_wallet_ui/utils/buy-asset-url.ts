import { Network, AccountAssetOptionType, UserAccountType } from '../constants/types'
import { NetworkOptions } from '../options/network-options'

const wyreID = 'AC_MGNVBGHPA9T'

export function BuyAssetUrl (network: Network, asset: AccountAssetOptionType, account: UserAccountType, buyAmount: string) {
  switch (network) {
    case Network.Mainnet:
      return `https://pay.sendwyre.com/?dest=ethereum:${account.address}&destCurrency=${asset.asset.symbol}&amount=${buyAmount}&accountId=${wyreID}&paymentMethod=debit-card`
    case Network.Ropsten:
      return 'https://faucet.metamask.io/'
    case Network.Kovan:
      return 'https://github.com/kovan-testnet/faucet'
    case Network.Rinkeby:
      return 'https://www.rinkeby.io/'
    case Network.Goerli:
      return 'https://goerli-faucet.slock.it/'
    default:
      throw new Error(`Unknown cryptocurrency exchange or faucet: "${NetworkOptions[network].name}"`)
  }
}

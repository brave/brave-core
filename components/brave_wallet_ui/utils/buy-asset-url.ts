import {
  BraveWallet,
  UserAccountType
} from '../constants/types'

import { getBuyAssetUrl } from '../common/async/lib'

export function GetBuyOrFaucetUrl (networkChainId: string, asset: BraveWallet.BlockchainToken, account: UserAccountType, buyAmount: string): Promise<string> {
  return new Promise(async (resolve, reject) => {
    switch (networkChainId) {
      case BraveWallet.MAINNET_CHAIN_ID:
        getBuyAssetUrl(account.address, asset.symbol, buyAmount)
          .then(resolve)
          .catch(reject)
        break
      case BraveWallet.ROPSTEN_CHAIN_ID:
        resolve('https://faucet.ropsten.be/')
        break
      case BraveWallet.KOVAN_CHAIN_ID:
        resolve('https://github.com/kovan-testnet/faucet')
        break
      case BraveWallet.RINKEBY_CHAIN_ID:
        resolve('https://www.rinkeby.io/#faucet')
        break
      case BraveWallet.GOERLI_CHAIN_ID:
        resolve('https://goerli-faucet.slock.it/')
        break
      default:
        reject()
    }
  })
}

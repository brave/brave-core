import { BraveWallet } from '../../../../constants/types'
import { AccountInfoEntity } from '../../../../common/slices/entities/account-info.entity'

export const getBalanceRegistryKey = (account: AccountInfoEntity, asset: BraveWallet.BlockchainToken) => {
  return `${account.address.toLocaleLowerCase()}-${asset.coin}-${asset.chainId}-${asset.contractAddress.toLowerCase()}`
}

export const getBalanceRegistryKeyRaw = (
  account: AccountInfoEntity,
  coin: BraveWallet.CoinType,
  chainId: string,
  contract: string
) => {
  return `${account.address.toLocaleLowerCase()}-${coin}-${chainId}-${contract.toLowerCase()}`
}

import * as React from 'react'
import {
  BraveWallet,
  BuySendSwapViewTypes,
  UserAccountType,
  WalletState
} from '../../../constants/types'

import {
  SelectAccount,
  SelectNetworkWithHeader,
  SelectAsset
} from '../'

// Styled Components
import {
  StyledWrapper
} from './style'
import { useSelector } from 'react-redux'

export interface Props {
  selectedView: BuySendSwapViewTypes
  assetOptions: BraveWallet.BlockchainToken[]

  onClickSelectAccount: (account: UserAccountType) => () => void
  onSelectedAsset: (account: BraveWallet.BlockchainToken) => () => void
  goBack: () => void
}

export const AccountsAssetsNetworks = (props: Props) => {
  const {
    selectedView,
    assetOptions,
    onClickSelectAccount,
    goBack,
    onSelectedAsset
  } = props

  // redux
  const {
    accounts,
    selectedAccount
  } = useSelector((state: {wallet: WalletState}) => {
    return state.wallet
  })

  // render
  return (
    <StyledWrapper>
      {selectedView === 'acounts' &&
        <SelectAccount
          selectedAccount={selectedAccount}
          accounts={accounts}
          onSelectAccount={onClickSelectAccount}
          onBack={goBack}
        />
      }
      {selectedView === 'assets' &&
        <SelectAsset
          assets={assetOptions}
          onSelectAsset={onSelectedAsset}
          onBack={goBack}
        />
      }
      {selectedView === 'networks' &&
        <SelectNetworkWithHeader
          onBack={goBack}
          hasAddButton={true}
        />
      }
    </StyledWrapper>
  )
}

export default AccountsAssetsNetworks

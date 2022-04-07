import * as React from 'react'
import {
  BraveWallet,
  BuySendSwapViewTypes,
  UserAccountType,
  WalletState
} from '../../../constants/types'

import {
  SelectAccount,
  SelectNetwork,
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
  onClickSelectNetwork: (network: BraveWallet.NetworkInfo) => () => void
  onSelectedAsset: (account: BraveWallet.BlockchainToken) => () => void
  goBack: () => void
}

function SelectHeader (props: Props) {
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
        <SelectNetwork
          onBack={goBack}
          hasAddButton={true}
        />
      }
    </StyledWrapper>
  )
}

export default SelectHeader

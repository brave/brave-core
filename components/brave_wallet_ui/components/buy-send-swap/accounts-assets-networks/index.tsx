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
  onAddAsset: () => void
  onClickSelectAccount: (account: UserAccountType) => () => void
  onClickSelectNetwork: (network: BraveWallet.NetworkInfo) => () => void
  onSelectedAsset: (account: BraveWallet.BlockchainToken) => () => void
  goBack: () => void
  onAddNetwork: () => void
}

function SelectHeader (props: Props) {
  const {
    selectedView,
    assetOptions,
    onAddAsset,
    onClickSelectAccount,
    goBack,
    onSelectedAsset,
    onClickSelectNetwork,
    onAddNetwork
  } = props

  // redux
  const {
    accounts,
    networkList,
    selectedAccount,
    selectedNetwork
  } = useSelector((state: {wallet: WalletState}) => {
    return state.wallet
  })

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
          onAddAsset={onAddAsset}
          assets={assetOptions}
          selectedNetwork={selectedNetwork}
          onSelectAsset={onSelectedAsset}
          onBack={goBack}
        />
      }
      {selectedView === 'networks' &&
        <SelectNetwork
          selectedNetwork={selectedNetwork}
          networks={networkList}
          onSelectNetwork={onClickSelectNetwork}
          onBack={goBack}
          hasAddButton={true}
          onAddNetwork={onAddNetwork}
        />
      }
    </StyledWrapper>
  )
}

export default SelectHeader

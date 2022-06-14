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

// Redux
import { useDispatch, useSelector } from 'react-redux'

// Actions
import { WalletActions } from '../../../common/actions'
import { SelectCurrency } from '../select-currency/select-currency'

export interface Props {
  selectedView: BuySendSwapViewTypes
  assetOptions: BraveWallet.BlockchainToken[]
  onClickSelectAccount: (account: UserAccountType) => () => void
  onSelectedAsset: (account: BraveWallet.BlockchainToken) => () => void
  onSelectCurrency?: () => void
  goBack: () => void
}

export const AccountsAssetsNetworks = (props: Props) => {
  const {
    selectedView,
    assetOptions,
    onClickSelectAccount,
    goBack,
    onSelectedAsset,
    onSelectCurrency
  } = props

  // redux
  const {
    accounts,
    selectedAccount
  } = useSelector((state: {wallet: WalletState}) => {
    return state.wallet
  })
  const dispatch = useDispatch()

  // methods
  const onAddNetwork = () => {
    dispatch(WalletActions.expandWalletNetworks())
  }

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
          onAddNetwork={onAddNetwork}
        />
      }

      {selectedView === 'currencies' &&
        <SelectCurrency
          onSelectCurrency={onSelectCurrency}
          onBack={goBack}
        />
      }
    </StyledWrapper>
  )
}

export default AccountsAssetsNetworks

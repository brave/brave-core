import * as React from 'react'
import {
  AssetOptionType,
  BuySendSwapViewTypes,
  NetworkOptionsType,
  UserAccountType
} from '../../../constants/types'
import { NetworkOptions } from '../../../options/network-options'
import {
  SelectAccount,
  SelectNetwork,
  SelectAsset
} from '../'

// Styled Components
import {
  StyledWrapper
} from './style'

export interface Props {
  selectedView: BuySendSwapViewTypes
  accounts: UserAccountType[]
  assetOptions: AssetOptionType[]
  onClickSelectAccount: (account: UserAccountType) => () => void
  onClickSelectNetwork: (network: NetworkOptionsType) => () => void
  onSelectedAsset: (account: AssetOptionType) => () => void
  goBack: () => void
}

function SelectHeader (props: Props) {
  const {
    selectedView,
    accounts,
    assetOptions,
    onClickSelectAccount,
    goBack,
    onSelectedAsset,
    onClickSelectNetwork
  } = props

  return (
    <StyledWrapper>
      {selectedView === 'acounts' &&
        <SelectAccount
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
          networks={NetworkOptions}
          onSelectNetwork={onClickSelectNetwork}
          onBack={goBack}
        />
      }
    </StyledWrapper>
  )
}

export default SelectHeader

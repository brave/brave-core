import * as React from 'react'
import {
  AssetOptionType,
  BuySendSwapViewTypes,
  NetworkOptionsType,
  ToOrFromType
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  FaucetTitle,
  FaucetWrapper,
  FaucetDescription
} from './style'

export interface Props {
  selectedAsset: AssetOptionType
  selectedNetwork: NetworkOptionsType
  buyAmount: string
  onSubmit: () => void
  onInputChange: (value: string, name: string) => void
  onChangeBuyView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
}

function Buy (props: Props) {
  const {
    selectedNetwork,
    selectedAsset,
    buyAmount,
    onInputChange,
    onSubmit,
    onChangeBuyView
  } = props

  const onShowAssets = () => {
    onChangeBuyView('assets', 'from')
  }

  return (
    <StyledWrapper>
      {selectedNetwork.id === 0 ? (
        <SwapInputComponent
          componentType='buyAmount'
          onInputChange={onInputChange}
          selectedAssetInputAmount={buyAmount}
          inputName='buy'
          selectedAsset={selectedAsset}
          onShowSelection={onShowAssets}
        />
      ) : (
        <FaucetWrapper>
          <FaucetTitle>{locale.buyTitle}</FaucetTitle>
          <FaucetDescription>{locale.buyDescription} {selectedNetwork.abbr}</FaucetDescription>
        </FaucetWrapper>
      )}
      <NavButton
        disabled={false}
        buttonType='primary'
        text={selectedNetwork.id === 0 ? locale.buyWyreButton : locale.buyFaucetButton}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Buy

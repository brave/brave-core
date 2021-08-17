import * as React from 'react'
import {
  AccountAssetOptionType,
  BuySendSwapViewTypes,
  EthereumChain,
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
  selectedAsset: AccountAssetOptionType
  selectedNetwork: EthereumChain
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
      {selectedNetwork.chainId === '0x1' ? (
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
          <FaucetDescription>{locale.buyDescription} {selectedNetwork.chainId}</FaucetDescription>
        </FaucetWrapper>
      )}
      <NavButton
        disabled={false}
        buttonType='primary'
        text={selectedNetwork.chainId === '0x1' ? locale.buyWyreButton : locale.buyFaucetButton}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Buy

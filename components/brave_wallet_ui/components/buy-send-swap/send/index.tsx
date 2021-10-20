import * as React from 'react'
import {
  AccountAssetOptionType,
  BuySendSwapViewTypes,
  ToOrFromType
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper
} from './style'

export interface Props {
  selectedAsset: AccountAssetOptionType
  selectedAssetAmount: string
  selectedAssetBalance: string
  toAddressOrUrl: string
  toAddress: string
  addressError: string
  onSubmit: () => void
  onInputChange: (value: string, name: string) => void
  onChangeSendView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onSelectPresetAmount: (percent: number) => void
}

function Send (props: Props) {
  const {
    selectedAsset,
    selectedAssetAmount,
    selectedAssetBalance,
    toAddressOrUrl,
    toAddress,
    addressError,
    onInputChange,
    onSelectPresetAmount,
    onSubmit,
    onChangeSendView
  } = props

  const onShowAssets = () => {
    onChangeSendView('assets', 'from')
  }

  const onPasteFromClipboard = async () => {
    const address = await navigator.clipboard.readText()
    onInputChange(address, 'address')
  }

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={onSelectPresetAmount}
        onInputChange={onInputChange}
        selectedAssetInputAmount={selectedAssetAmount}
        inputName='from'
        selectedAssetBalance={selectedAssetBalance}
        selectedAsset={selectedAsset}
        onShowSelection={onShowAssets}
      />
      <SwapInputComponent
        componentType='toAddress'
        onInputChange={onInputChange}
        toAddressOrUrl={toAddressOrUrl}
        addressError={addressError}
        toAddress={toAddress}
        inputName='address'
        onPaste={onPasteFromClipboard}
      />
      <NavButton
        disabled={addressError !== '' || toAddressOrUrl === ''}
        buttonType='primary'
        text={getLocale('braveWalletSend')}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Send

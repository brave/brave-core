import * as React from 'react'
import {
  AccountAssetOptionType,
  BuySendSwapViewTypes,
  ToOrFromType
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import { getLocale } from '../../../../common/locale'
import { ErrorText } from '../shared-styles'
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

  const insuficientFundsError = React.useMemo((): boolean => {
    if (parseFloat(selectedAssetAmount) === 0) {
      return false
    }
    return Number(selectedAssetAmount) > Number(selectedAssetBalance)
  }, [selectedAssetBalance, selectedAssetAmount])

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
      {insuficientFundsError &&
        <ErrorText>{getLocale('braveWalletSwapInsufficientBalance')}</ErrorText>
      }
      <NavButton
        disabled={addressError !== ''
          || toAddressOrUrl === ''
          || parseFloat(selectedAssetAmount) === 0
          || selectedAssetAmount === ''
          || insuficientFundsError
        }
        buttonType='primary'
        text={getLocale('braveWalletSend')}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Send

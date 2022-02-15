import * as React from 'react'
import {
  AmountPresetTypes,
  AmountValidationErrorType,
  BraveWallet,
  BuySendSwapViewTypes,
  SwapValidationErrorType,
  ToOrFromType
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import { getLocale } from '../../../../common/locale'
import { ErrorText, ResetButton } from '../shared-styles'

// Utils
import Amount from '../../../utils/amount'

// Styled Components
import {
  StyledWrapper
} from './style'

export interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
  selectedAssetAmount: string
  selectedAssetBalance: string
  toAddressOrUrl: string
  toAddress: string
  addressError: string
  addressWarning: string
  amountValidationError?: AmountValidationErrorType
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
    addressWarning,
    amountValidationError,
    onInputChange,
    onSelectPresetAmount,
    onSubmit,
    onChangeSendView
  } = props
  const [selectedPreset, setSelectedPreset] = React.useState<AmountPresetTypes | undefined>()

  const onShowAssets = () => {
    onChangeSendView('assets', 'from')
  }

  const onPasteFromClipboard = async () => {
    const address = await navigator.clipboard.readText()
    onInputChange(address, 'address')
  }

  const onReset = () => {
    onInputChange('', 'from')
    onInputChange('', 'address')
    setSelectedPreset(0)
  }

  const setPresetAmountValue = (percent: number) => {
    setSelectedPreset(percent as AmountPresetTypes)
    onSelectPresetAmount(percent)
  }

  const insufficientFundsError = React.useMemo((): boolean => {
    if (!selectedAsset) {
      return false
    }

    const amountWei = new Amount(selectedAssetAmount)
      .multiplyByDecimals(selectedAsset.decimals)

    if (amountWei.isZero()) {
      return false
    }

    return amountWei.gt(selectedAssetBalance)
  }, [selectedAssetBalance, selectedAssetAmount, selectedAsset])

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={setPresetAmountValue}
        onInputChange={onInputChange}
        selectedAssetInputAmount={selectedAssetAmount}
        inputName='from'
        selectedAssetBalance={selectedAssetBalance}
        selectedAsset={selectedAsset}
        onShowSelection={onShowAssets}
        autoFocus={true}
        selectedPreset={selectedPreset}
        validationError={amountValidationError as SwapValidationErrorType}
      />
      <SwapInputComponent
        componentType='toAddress'
        onInputChange={onInputChange}
        toAddressOrUrl={toAddressOrUrl}
        addressError={addressError}
        addressWarning={addressWarning}
        toAddress={toAddress}
        inputName='address'
        onPaste={onPasteFromClipboard}
      />
      {insufficientFundsError &&
        <ErrorText>{getLocale('braveWalletSwapInsufficientBalance')}</ErrorText>
      }
      <NavButton
        disabled={addressError !== '' ||
          toAddressOrUrl === '' ||
          parseFloat(selectedAssetAmount) === 0 ||
          selectedAssetAmount === '' ||
          insufficientFundsError ||
          amountValidationError !== undefined
        }
        buttonType='primary'
        text={getLocale('braveWalletSend')}
        onSubmit={onSubmit}
      />

      <ResetButton
        onClick={onReset}
      >
        {getLocale('braveWalletReset')}
      </ResetButton>
    </StyledWrapper>
  )
}

export default Send

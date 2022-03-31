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
import { Tooltip } from '../../shared'

// Utils
import Amount from '../../../utils/amount'

// Styled Components
import {
  StyledWrapper
} from './style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken | undefined
  selectedNetwork: BraveWallet.NetworkInfo
  selectedAssetAmount: string
  selectedAssetBalance: string
  toAddressOrUrl: string
  toAddress: string
  addressError: string
  addressWarning: string
  amountValidationError: AmountValidationErrorType | undefined
  onSubmit: () => void
  onInputChange: (value: string, name: string) => void
  onChangeSendView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
  onSelectPresetAmount: (percent: number) => void
}

function Send (props: Props) {
  const {
    selectedAsset,
    selectedNetwork,
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

  const handleOnInputChange = (value: string, name: string) => {
    if (name === 'from' && selectedPreset) {
      // Clear preset
      setSelectedPreset(undefined)
    }
    onInputChange(value, name)
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

  const tooltipMessage = React.useMemo((): string => {
    const amountWrapped = new Amount(selectedAssetAmount)
    if (amountWrapped.isUndefined() || amountWrapped.isZero()) {
      return getLocale('braveWalletZeroBalanceError')
    }
    if (toAddressOrUrl === '') {
      return getLocale('braveWalletAddressRequiredError')
    }
    return ''
  }, [toAddressOrUrl, selectedAssetAmount])

  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={setPresetAmountValue}
        onInputChange={handleOnInputChange}
        selectedAssetInputAmount={selectedAssetAmount}
        inputName='from'
        selectedAssetBalance={selectedAssetBalance}
        selectedAsset={selectedAsset}
        selectedNetwork={selectedNetwork}
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
      <Tooltip
        text={tooltipMessage}
        isVisible={
          parseFloat(selectedAssetAmount) === 0 ||
          selectedAssetAmount === '' ||
          toAddressOrUrl === ''
        }
      >
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
      </Tooltip>
      <ResetButton
        onClick={onReset}
      >
        {getLocale('braveWalletReset')}
      </ResetButton>
    </StyledWrapper>
  )
}

export default Send

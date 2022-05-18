import * as React from 'react'
import {
  AmountPresetTypes,
  BuySendSwapViewTypes,
  SwapValidationErrorType,
  ToOrFromType,
  WalletState
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
import useSend from '../../../common/hooks/send'
import { useSelector } from 'react-redux'
import useBalance from '../../../common/hooks/balance'
import { usePreset } from '../../../common/hooks'

export interface Props {
  onChangeSendView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
}

function Send (props: Props) {
  const { onChangeSendView } = props

  // redux
  const {
    selectedAccount,
    networkList,
    selectedNetwork
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const {
    toAddressOrUrl,
    toAddress,
    addressError,
    addressWarning,
    sendAmount,
    selectedSendAsset,
    sendAmountValidationError,
    setSendAmount,
    setToAddressOrUrl,
    submitSend
  } = useSend()
  const getSelectedAccountBalance = useBalance(networkList)

  // state
  const sendAssetBalance = getSelectedAccountBalance(selectedAccount, selectedSendAsset)
  const [selectedPreset, setSelectedPreset] = React.useState<AmountPresetTypes | undefined>()

  // methods
  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: setSendAmount,
      asset: selectedSendAsset
    }
  )

  const onInputChange = (value: string, name: string) => {
    if (name === 'address') {
      setToAddressOrUrl(value)
    } else {
      setSendAmount(value)
    }
  }

  const onShowAssets = () => onChangeSendView('assets', 'from')

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

  // memos
  const insufficientFundsError = React.useMemo((): boolean => {
    if (!selectedSendAsset) {
      return false
    }

    const amountWei = new Amount(sendAmount)
      .multiplyByDecimals(selectedSendAsset.decimals)

    if (amountWei.isZero()) {
      return false
    }

    return amountWei.gt(sendAssetBalance)
  }, [sendAssetBalance, sendAmount, selectedSendAsset])

  const tooltipMessage = React.useMemo((): string => {
    const amountWrapped = new Amount(sendAmount)
    if (amountWrapped.isUndefined() || amountWrapped.isZero()) {
      return getLocale('braveWalletZeroBalanceError')
    }
    if (toAddressOrUrl === '') {
      return getLocale('braveWalletAddressRequiredError')
    }
    return ''
  }, [toAddressOrUrl, sendAmount])

  // render
  return (
    <StyledWrapper>
      <SwapInputComponent
        componentType='fromAmount'
        onSelectPresetAmount={setPresetAmountValue}
        onInputChange={handleOnInputChange}
        selectedAssetInputAmount={sendAmount}
        inputName='from'
        selectedAssetBalance={sendAssetBalance}
        selectedAsset={selectedSendAsset}
        selectedNetwork={selectedNetwork}
        onShowSelection={onShowAssets}
        autoFocus={true}
        selectedPreset={selectedPreset}
        validationError={sendAmountValidationError as SwapValidationErrorType}
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
        selectedNetwork={selectedNetwork}
      />
      {insufficientFundsError &&
        <ErrorText>{getLocale('braveWalletSwapInsufficientBalance')}</ErrorText>
      }
      <Tooltip
        text={tooltipMessage}
        isVisible={
          parseFloat(sendAmount) === 0 ||
          sendAmount === '' ||
          toAddressOrUrl === ''
        }
      >
        <NavButton
          disabled={addressError !== '' ||
            toAddressOrUrl === '' ||
            parseFloat(sendAmount) === 0 ||
            sendAmount === '' ||
            insufficientFundsError ||
            sendAmountValidationError !== undefined
          }
          buttonType='primary'
          text={getLocale('braveWalletSend')}
          onSubmit={submitSend}
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

import * as React from 'react'
import BigNumber from 'bignumber.js'

import { getLocale } from '../../../../common/locale'
import { TransactionInfo, EthereumChain } from '../../../constants/types'
import { UpdateUnapprovedTransactionGasFieldsType } from '../../../common/constants/action_types'
import { NavButton, Panel } from '../'
import {
  formatFiatGasFee,
  toWei,
  toGWei,
  addCurrencies,
  formatGasFee,
  gWeiToWei,
  gWeiToWeiHex,
  toWeiHex
} from '../../../utils/format-balances'
import { useTransactionFeesParser } from '../../../common/hooks'

// Styled Components
import {
  StyledWrapper,
  FormColumn,
  Input,
  InputLabel,
  ButtonRow,
  Description,
  CurrentBaseRow,
  CurrentBaseText,
  MaximumFeeRow,
  MaximumFeeText,
  GasSlider,
  SliderLabelRow,
  SliderLabel,
  SliderWrapper,
  SliderValue
} from './style'

export enum MaxPriorityPanels {
  setSuggested = 0,
  setCustom = 1
}

export interface Props {
  onCancel: () => void
  networkSpotPrice: string
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
  baseFeePerGas: string
  suggestedMaxPriorityFeeChoices: string[]
  suggestedSliderStep: string
  maxPriorityPanel: MaxPriorityPanels
  updateUnapprovedTransactionGasFields: (payload: UpdateUnapprovedTransactionGasFieldsType) => void
  setSuggestedSliderStep: (value: string) => void
  setMaxPriorityPanel: (value: MaxPriorityPanels) => void
}

const EditGas = (props: Props) => {
  const {
    onCancel,
    networkSpotPrice,
    selectedNetwork,
    transactionInfo,
    baseFeePerGas,
    suggestedMaxPriorityFeeChoices,
    suggestedSliderStep,
    maxPriorityPanel,
    updateUnapprovedTransactionGasFields,
    setSuggestedSliderStep,
    setMaxPriorityPanel
  } = props
  const parseTransactionFees = useTransactionFeesParser(selectedNetwork, networkSpotPrice)
  const transactionFees = parseTransactionFees(transactionInfo)
  const { isEIP1559Transaction } = transactionFees

  const [suggestedMaxPriorityFee, setSuggestedMaxPriorityFee] = React.useState<string>(suggestedMaxPriorityFeeChoices[1])
  const [gasLimit, setGasLimit] = React.useState<string>(toWei(transactionFees.gasLimit, 0))
  const [gasPrice, setGasPrice] = React.useState<string>(toGWei(transactionFees.gasPrice, selectedNetwork.decimals))
  const [maxPriorityFeePerGas, setMaxPriorityFeePerGas] = React.useState<string>(toGWei(transactionFees.maxPriorityFeePerGas, selectedNetwork.decimals))
  const [maxFeePerGas, setMaxFeePerGas] = React.useState<string>(toGWei(transactionFees.maxFeePerGas, selectedNetwork.decimals))

  const handleGasPriceInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasPrice(event.target.value)
  }

  const handleGasLimitInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasLimit(event.target.value)
  }

  const handleMaxPriorityFeePerGasInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value
    setMaxPriorityFeePerGas(value)

    const computedMaxFeePerGasWeiHex = addCurrencies(baseFeePerGas, gWeiToWei(value))
    const computedMaxFeePerGasGWei = toGWei(
      computedMaxFeePerGasWeiHex,
      selectedNetwork.decimals
    )
    setMaxFeePerGas(computedMaxFeePerGasGWei)
  }

  const handleMaxFeePerGasInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setMaxFeePerGas(event.target.value)
  }

  const onSetPanelToCustom = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setCustom)
  }

  const onSetPanelToSuggested = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setSuggested)
  }

  const handleSliderChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const suggestedSliderStep = event.target.value
    setSuggestedSliderStep(suggestedSliderStep)
    setSuggestedMaxPriorityFee(suggestedMaxPriorityFeeChoices[Number(suggestedSliderStep)])
  }

  const showSuggestedMaxPriorityPanel = isEIP1559Transaction && maxPriorityPanel === MaxPriorityPanels.setSuggested
  const showCustomMaxPriorityPanel = isEIP1559Transaction && maxPriorityPanel === MaxPriorityPanels.setCustom
  const suggestedEIP1559GasFee = showSuggestedMaxPriorityPanel
    ? formatGasFee(
      addCurrencies(suggestedMaxPriorityFee, baseFeePerGas),
      gasLimit,
      selectedNetwork.decimals
    )
    : undefined
  const suggestedEIP1559FiatGasFee = suggestedEIP1559GasFee && formatFiatGasFee(suggestedEIP1559GasFee, networkSpotPrice)
  const customEIP1559GasFee = showCustomMaxPriorityPanel
    ? formatGasFee(gWeiToWei(maxFeePerGas), gasLimit, selectedNetwork.decimals)
    : undefined
  const customEIP1559FiatGasFee = customEIP1559GasFee && formatFiatGasFee(customEIP1559GasFee, networkSpotPrice)

  const gasLimitComponent = (
    <>
      <InputLabel>{getLocale('braveWalletEditGasLimit')}</InputLabel>
      <Input
        placeholder='0'
        type='number'
        value={gasLimit}
        onChange={handleGasLimitInputChanged}
      />
    </>
  )

  const onSave = () => {
    if (!isEIP1559Transaction) {
      updateUnapprovedTransactionGasFields({
        txMetaId: transactionInfo.id,
        gasPrice: gWeiToWeiHex(gasPrice),
        gasLimit: toWeiHex(gasLimit, 0)
      })

      onCancel()
      return
    }

    if (maxPriorityPanel === MaxPriorityPanels.setCustom) {
      updateUnapprovedTransactionGasFields({
        txMetaId: transactionInfo.id,
        maxPriorityFeePerGas: gWeiToWeiHex(maxPriorityFeePerGas),
        maxFeePerGas: gWeiToWeiHex(maxFeePerGas),
        gasLimit: toWeiHex(gasLimit, 0)
      })
    } else if (maxPriorityPanel === MaxPriorityPanels.setSuggested) {
      updateUnapprovedTransactionGasFields({
        txMetaId: transactionInfo.id,
        gasLimit: toWeiHex(gasLimit, 0),
        maxPriorityFeePerGas: suggestedMaxPriorityFee,
        maxFeePerGas: addCurrencies(suggestedMaxPriorityFee, baseFeePerGas)
      })
    }

    onCancel()
  }

  const isSaveButtonDisabled = React.useMemo(() => {
    if (gasLimit === '') {
      return true
    }

    if (new BigNumber(gasLimit).lte(0)) {
      return true
    }

    if (!isEIP1559Transaction && gasPrice === '') {
      return true
    }

    if (!isEIP1559Transaction && new BigNumber(gWeiToWei(gasPrice)).lte(0)) {
      return true
    }

    if (isEIP1559Transaction && maxFeePerGas === '') {
      return true
    }

    if (isEIP1559Transaction && new BigNumber(baseFeePerGas).gt(gWeiToWei(maxFeePerGas))) {
      return true
    }

    return isEIP1559Transaction && new BigNumber(gWeiToWei(maxPriorityFeePerGas)).lt(0)
  }, [gasLimit, gasPrice, maxPriorityFeePerGas, maxFeePerGas])

  return (
    <Panel
      navAction={onCancel}
      title={isEIP1559Transaction ? getLocale('braveWalletEditGasTitle1') : getLocale('braveWalletEditGasTitle2')}
    >
      <StyledWrapper>
        {isEIP1559Transaction &&
          <Description>{getLocale('braveWalletEditGasDescription')}</Description>
        }
        {showCustomMaxPriorityPanel &&
          <FormColumn>
            <CurrentBaseRow>
              <CurrentBaseText>{getLocale('braveWalletEditGasBaseFee')}</CurrentBaseText>
              <CurrentBaseText>
                {`${toGWei(baseFeePerGas, selectedNetwork.decimals)} ${getLocale('braveWalletEditGasGwei')}`}
              </CurrentBaseText>
            </CurrentBaseRow>

            {gasLimitComponent}

            <InputLabel>{getLocale('braveWalletEditGasPerTipLimit')}</InputLabel>
            <Input
              placeholder='0'
              type='number'
              value={maxPriorityFeePerGas}
              onChange={handleMaxPriorityFeePerGasInputChanged}
            />

            <InputLabel>{getLocale('braveWalletEditGasPerPriceLimit')}</InputLabel>
            <Input
              placeholder='0'
              type='number'
              value={maxFeePerGas}
              onChange={handleMaxFeePerGasInputChanged}
            />

            <MaximumFeeRow>
              <MaximumFeeText>{getLocale('braveWalletEditGasMaximumFee')}</MaximumFeeText>
              <MaximumFeeText>
                ~${customEIP1559FiatGasFee} USD (${customEIP1559GasFee} {selectedNetwork.symbol})
              </MaximumFeeText>
            </MaximumFeeRow>
          </FormColumn>
        }

        {showSuggestedMaxPriorityPanel &&
          <SliderWrapper>
            <SliderLabel>{getLocale('braveWalletEditGasMaxFee')}:</SliderLabel>
            <SliderValue>
              ~${suggestedEIP1559FiatGasFee}
              {` `}USD ({suggestedEIP1559GasFee}
              {` `}{selectedNetwork.symbol})</SliderValue>
            <GasSlider
              type='range'
              min='0'
              max={suggestedMaxPriorityFeeChoices.length - 1}
              value={suggestedSliderStep}
              onChange={handleSliderChange}
            />
            <SliderLabelRow>
              <SliderLabel>{getLocale('braveWalletEditGasLow')}</SliderLabel>
              <SliderLabel>{getLocale('braveWalletEditGasOptimal')}</SliderLabel>
              <SliderLabel>{getLocale('braveWalletEditGasHigh')}</SliderLabel>
            </SliderLabelRow>
          </SliderWrapper>
        }

        {!isEIP1559Transaction &&
          <FormColumn>
            {gasLimitComponent}

            <InputLabel>{getLocale('braveWalletEditGasPerGasPrice')}</InputLabel>
            <Input
              placeholder='0'
              type='number'
              value={gasPrice}
              onChange={handleGasPriceInputChanged}
            />
          </FormColumn>
        }

        <ButtonRow>
          <NavButton
            buttonType='secondary'
            text={!isEIP1559Transaction ? getLocale('braveWalletBackupButtonCancel')
              : maxPriorityPanel === MaxPriorityPanels.setCustom ? getLocale('braveWalletEditGasSetSuggested')
                : getLocale('braveWalletEditGasSetCustom')

            }
            onSubmit={
              !isEIP1559Transaction ? onCancel
                : maxPriorityPanel === MaxPriorityPanels.setCustom ? onSetPanelToSuggested
                  : onSetPanelToCustom}
          />
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletAccountSettingsSave')}
            onSubmit={onSave}
            disabled={isSaveButtonDisabled}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default EditGas

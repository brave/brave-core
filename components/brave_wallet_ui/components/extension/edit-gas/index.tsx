import * as React from 'react'
import locale from '../../../constants/locale'
import { TransactionInfo, EthereumChain } from '../../../constants/types'
import { NavButton, Panel } from '../'
import { formatFiatBalance, formatBalance } from '../../../utils/format-balances'
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

enum MaxPriorityPanels {
  setSuggested = 0,
  setCustom = 1
}

export interface Props {
  onCancel: () => void
  onSave: () => void
  networkSpotPrice: string
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
}

// Example of suggested max priority fees
const suggestedMaxPriorityFees = [
  '10000000000000000',
  '15000000000000000',
  '20000000000000000'
]

const EditGas = (props: Props) => {
  const {
    onCancel,
    onSave,
    networkSpotPrice,
    selectedNetwork,
    transactionInfo
  } = props
  const { txData } = transactionInfo
  const { baseData } = txData
  const [maxPriorityPanel, setMaxPriorityPanel] = React.useState<MaxPriorityPanels>(MaxPriorityPanels.setSuggested)
  const [suggestedMaxPriorityFee, setSuggestedMaxPriorityFee] = React.useState<string>(suggestedMaxPriorityFees[1])
  const [suggestedSliderStep, setSuggestedSliderStep] = React.useState<string>('1')
  const [gasPrice, setGasPrice] = React.useState<string>(baseData.gasPrice)
  const [gasAmountLimit, setGasAmountLimit] = React.useState<string>(baseData.gasLimit)

  // Will determine what values to set for all these when the API is ready
  const [gasTipAmountLimit, setGasTipAmountLimit] = React.useState<string>('2')
  const [gasTipPriceLimit, setGasTipPriceLimit] = React.useState<string>('150')

  // Will determine how best to differentiate between EIP1559 and Legacy
  // when wiring up to API.
  const isEIP1559 = false

  const handleGasPriceInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasPrice(event.target.value)
  }

  const handleGasAmmountLimitInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasAmountLimit(event.target.value)
  }

  const handleGasTipAmmountLimitInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasTipAmountLimit(event.target.value)
  }

  const handleGasTipPriceLimitInputChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setGasTipPriceLimit(event.target.value)
  }

  const onSetPanelToCustom = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setCustom)
  }

  const onSetPanelToSuggested = () => {
    setMaxPriorityPanel(MaxPriorityPanels.setSuggested)
  }

  const handleSliderChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setSuggestedSliderStep(event.target.value)
  }

  React.useMemo(() => {
    setSuggestedMaxPriorityFee(suggestedMaxPriorityFees[Number(suggestedSliderStep)])
  }, [suggestedSliderStep])

  return (
    <Panel
      navAction={onCancel}
      title={isEIP1559 ? locale.braveWalletEditGasTitle1 : locale.braveWalletEditGasTitle2}
    >
      <StyledWrapper>
        {isEIP1559 &&
          <Description>{locale.braveWalletEditGasDescription}</Description>
        }
        {(maxPriorityPanel !== MaxPriorityPanels.setSuggested || !isEIP1559) &&
          <FormColumn>
            {!isEIP1559 &&
              <>
                <InputLabel>{locale.braveWalletEditGasPerGasPrice}</InputLabel>
                <Input
                  placeholder='0'
                  type='number'
                  value={gasPrice}
                  onChange={handleGasPriceInputChanged}
                />
              </>
            }
            {maxPriorityPanel === MaxPriorityPanels.setCustom &&
              <CurrentBaseRow>
                <CurrentBaseText>{locale.braveWalletEditGasBaseFee}</CurrentBaseText>
                {/* Will determine what values to set here when the API is ready */}
                <CurrentBaseText>150 {locale.braveWalletEditGasGwei}</CurrentBaseText>
              </CurrentBaseRow>
            }
            <InputLabel>{locale.braveWalletEditGasAmountLimit}</InputLabel>
            <Input
              placeholder='0'
              type='number'
              value={gasAmountLimit}
              onChange={handleGasAmmountLimitInputChanged}
            />
            {isEIP1559 &&
              <>
                <InputLabel>{locale.braveWalletEditGasPerTipLimit}</InputLabel>
                <Input
                  placeholder='0'
                  type='number'
                  value={gasTipAmountLimit}
                  onChange={handleGasTipAmmountLimitInputChanged}
                />
                <InputLabel>{locale.braveWalletEditGasPerPriceLimit}</InputLabel>
                <Input
                  placeholder='0'
                  type='number'
                  value={gasTipPriceLimit}
                  onChange={handleGasTipPriceLimitInputChanged}
                />
              </>
            }
            {maxPriorityPanel === MaxPriorityPanels.setCustom &&
              <MaximumFeeRow>
                <MaximumFeeText>{locale.braveWalletEditGasMaxFee}</MaximumFeeText>
                {/* Will determine what values to set here when the API is ready */}
                <MaximumFeeText>~$0.24 USD (315,000 Gwei)</MaximumFeeText>
              </MaximumFeeRow>
            }
          </FormColumn>
        }
        {isEIP1559 && maxPriorityPanel === MaxPriorityPanels.setSuggested &&
          <SliderWrapper>
            <SliderValue>
              ~${formatFiatBalance(suggestedMaxPriorityFee, selectedNetwork.decimals, networkSpotPrice)}
              {` `}USD ({formatBalance(suggestedMaxPriorityFee, selectedNetwork.decimals)}
              {` `}{selectedNetwork.symbol})</SliderValue>
            <GasSlider
              type='range'
              min='0'
              max={suggestedMaxPriorityFees.length - 1}
              value={suggestedSliderStep}
              onChange={handleSliderChange}
            />
            <SliderLabelRow>
              <SliderLabel>{locale.braveWalletEditGasLow}</SliderLabel>
              <SliderLabel>{locale.braveWalletEditGasOptimal}</SliderLabel>
              <SliderLabel>{locale.braveWalletEditGasHigh}</SliderLabel>
            </SliderLabelRow>
          </SliderWrapper>
        }
        <ButtonRow>
          <NavButton
            buttonType='secondary'
            text={!isEIP1559 ? locale.backupButtonCancel
              : maxPriorityPanel === MaxPriorityPanels.setCustom ? locale.braveWalletEditGasSetSuggested
                : locale.braveWalletEditGasSetCustom

            }
            onSubmit={
              !isEIP1559 ? onCancel
                : maxPriorityPanel === MaxPriorityPanels.setCustom ? onSetPanelToSuggested
                  : onSetPanelToCustom}
          />
          <NavButton
            buttonType='primary'
            text={locale.accountSettingsSave}
            onSubmit={onSave}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default EditGas

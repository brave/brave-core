import * as React from 'react'
import {
  BraveWallet,
  BuySendSwapViewTypes,
  ToOrFromType,
  BuyOption, WalletState
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  Spacer
} from './style'
import { BuyOptions } from '../../../options/buy-with-options'
import { useAssets, useLib } from '../../../common/hooks'
import { useSelector } from 'react-redux'
import { getRampAssetSymbol, isSelectedAssetInAssetOptions } from '../../../utils/asset-utils'
import { SelectBuyOption } from '../select-buy-option/select-buy-option'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  onChangeBuyView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
}

function Buy (props: Props) {
  const {
    selectedAsset,
    onChangeBuyView
  } = props

  const [buyAmount, setBuyAmount] = React.useState('')
  const [showBuyOptions, setShowBuyOptions] = React.useState<boolean>(false)
  const [buyOptions, setBuyOptions] = React.useState<BuyOption[]>(BuyOptions)

  // Redux
  const {
    selectedNetwork,
    selectedAccount,
    defaultCurrencies,
    selectedCurrency
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // Custom Hooks
  const { wyreAssetOptions, rampAssetOptions } = useAssets()
  const { getBuyAssetUrl } = useLib()

  const onSubmitBuy = React.useCallback((buyOption: BraveWallet.OnRampProvider) => {
    const asset = buyOption === BraveWallet.OnRampProvider.kRamp
      ? { ...selectedAsset, symbol: getRampAssetSymbol(selectedAsset) }
      : selectedAsset
    getBuyAssetUrl({
      asset,
      onRampProvider: buyOption,
      chainId: selectedNetwork.chainId,
      address: selectedAccount.address,
      amount: buyAmount,
      currencyCode: selectedCurrency ? selectedCurrency.currencyCode : 'USD'
    })
      .then(url => {
        chrome.tabs.create({ url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      })
      .catch(e => console.error(e))
  }, [getBuyAssetUrl, selectedNetwork, selectedAccount, buyAmount, selectedAsset])

  React.useEffect(() => {
    const supportingBuyOptions = BuyOptions.filter(buyOption => {
      if (buyOption.id === BraveWallet.OnRampProvider.kWyre) {
        return isSelectedAssetInAssetOptions(selectedAsset, wyreAssetOptions)
      }

      if (buyOption.id === BraveWallet.OnRampProvider.kRamp) {
        return isSelectedAssetInAssetOptions(selectedAsset, rampAssetOptions)
      }

      return false
    })
    setBuyOptions(supportingBuyOptions)
  }, [selectedAsset, wyreAssetOptions, rampAssetOptions])

  const onShowAssets = React.useCallback(() => {
    onChangeBuyView('assets', 'from')
  }, [onChangeBuyView])

  const onContinue = React.useCallback(() => {
    setShowBuyOptions(true)
  }, [])

  const onBack = React.useCallback(() => {
    setShowBuyOptions(false)
  }, [])

  const onShowCurrencySelection = React.useCallback(() => {
    onChangeBuyView('currencies', 'from')
  }, [onChangeBuyView])

  const isSelectedNetworkSupported = React.useMemo(() => {
    return [...rampAssetOptions, ...wyreAssetOptions]
      .map(asset => asset.chainId.toLowerCase())
      .includes(selectedNetwork.chainId.toLowerCase())
  }, [selectedNetwork, rampAssetOptions, wyreAssetOptions])

  return (
    <StyledWrapper>
      {showBuyOptions
        ? <SelectBuyOption
          buyOptions={buyOptions}
          onSelect={onSubmitBuy}
          onBack={onBack}
        />
        : <>
          {isSelectedNetworkSupported &&
            <SwapInputComponent
              defaultCurrencies={defaultCurrencies}
              componentType='buyAmount'
              onInputChange={setBuyAmount}
              selectedAssetInputAmount={buyAmount}
              inputName='buy'
              selectedAsset={selectedAsset}
              selectedNetwork={selectedNetwork}
              onShowSelection={onShowAssets}
              onShowCurrencySelection={onShowCurrencySelection}
              autoFocus={true}
            />
          }
          <Spacer />
          <NavButton
            disabled={false}
            buttonType='primary'
            text={getLocale('braveWalletBuyContinueButton')}
            onSubmit={onContinue}
          />
        </>
      }
    </StyledWrapper>
  )
}

export default Buy

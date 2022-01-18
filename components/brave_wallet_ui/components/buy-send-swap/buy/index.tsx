import * as React from 'react'
import {
  BraveWallet,
  BuySendSwapViewTypes,
  ToOrFromType,
  DefaultCurrencies
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import { getLocale } from '../../../../common/locale'
// Styled Components
import {
  StyledWrapper,
  FaucetTitle,
  FaucetWrapper,
  FaucetDescription
} from './style'

export interface Props {
  selectedAsset: BraveWallet.BlockchainToken
  selectedNetwork: BraveWallet.EthereumChain
  buyAmount: string
  networkList: BraveWallet.EthereumChain[]
  defaultCurrencies: DefaultCurrencies
  onSubmit: () => void
  onInputChange: (value: string, name: string) => void
  onChangeBuyView: (view: BuySendSwapViewTypes, option?: ToOrFromType) => void
}

function Buy (props: Props) {
  const {
    selectedNetwork,
    selectedAsset,
    buyAmount,
    networkList,
    defaultCurrencies,
    onInputChange,
    onSubmit,
    onChangeBuyView
  } = props

  const onShowAssets = () => {
    onChangeBuyView('assets', 'from')
  }

  const networkName = React.useMemo((): string => {
    return networkList.find((network) => network.chainId === selectedNetwork.chainId)?.chainName ?? ''
  }, [networkList, selectedNetwork])

  return (
    <StyledWrapper>
      {selectedNetwork.chainId === BraveWallet.MAINNET_CHAIN_ID ? (
        <SwapInputComponent
          defaultCurrencies={defaultCurrencies}
          componentType='buyAmount'
          onInputChange={onInputChange}
          selectedAssetInputAmount={buyAmount}
          inputName='buy'
          selectedAsset={selectedAsset}
          onShowSelection={onShowAssets}
          autoFocus={true}
        />
      ) : (
        <FaucetWrapper>
          <FaucetTitle>{getLocale('braveWalletBuyTitle')}</FaucetTitle>
          <FaucetDescription>{getLocale('braveWalletBuyDescription').replace('$1', networkName)}</FaucetDescription>
        </FaucetWrapper>
      )}
      <NavButton
        disabled={false}
        buttonType='primary'
        text={selectedNetwork.chainId === BraveWallet.MAINNET_CHAIN_ID ? getLocale('braveWalletBuyWyreButton') : getLocale('braveWalletBuyFaucetButton')}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Buy

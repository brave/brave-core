import * as React from 'react'
import {
  AccountAssetOptionType,
  BuySendSwapViewTypes,
  EthereumChain,
  ToOrFromType,
  MAINNET_CHAIN_ID
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
  selectedAsset: AccountAssetOptionType
  selectedNetwork: EthereumChain
  buyAmount: string
  networkList: EthereumChain[]
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
      {selectedNetwork.chainId === MAINNET_CHAIN_ID ? (
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
          <FaucetTitle>{getLocale('braveWalletBuyTitle')}</FaucetTitle>
          <FaucetDescription>{getLocale('braveWalletBuyDescription').replace('$1', networkName)}</FaucetDescription>
        </FaucetWrapper>
      )}
      <NavButton
        disabled={false}
        buttonType='primary'
        text={selectedNetwork.chainId === MAINNET_CHAIN_ID ? getLocale('braveWalletBuyWyreButton') : getLocale('braveWalletBuyFaucetButton')}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Buy

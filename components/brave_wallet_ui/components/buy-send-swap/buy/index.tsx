import * as React from 'react'
import {
  AccountAssetOptionType,
  BuySendSwapViewTypes,
  EthereumChain,
  ToOrFromType,
  kMainnetChainId
} from '../../../constants/types'
import { NavButton } from '../../extension'
import SwapInputComponent from '../swap-input-component'
import locale from '../../../constants/locale'
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
      {selectedNetwork.chainId === kMainnetChainId ? (
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
          <FaucetTitle>{locale.buyTitle}</FaucetTitle>
          <FaucetDescription>{locale.buyDescription} {networkName}</FaucetDescription>
        </FaucetWrapper>
      )}
      <NavButton
        disabled={false}
        buttonType='primary'
        text={selectedNetwork.chainId === kMainnetChainId ? locale.buyWyreButton : locale.buyFaucetButton}
        onSubmit={onSubmit}
      />
    </StyledWrapper>
  )
}

export default Buy

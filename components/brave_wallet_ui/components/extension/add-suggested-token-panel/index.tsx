import * as React from 'react'

import { BraveWallet } from '../../../constants/types'

import {
  StyledWrapper,
  Title,
  Description,
  ButtonWrapper,
  TokenName,
  ContractAddress,
  AssetIcon,
  TopWrapper,
  NetworkText,
  TopRow
} from './style'
import { withPlaceholderIcon, Tooltip } from '../../shared'
import { NavButton } from '..'
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'

export interface Props {
  onCancel: () => void
  onAddToken: () => void
  selectedNetwork: BraveWallet.EthereumChain
  token?: BraveWallet.ERCToken
}

function AddSuggestedTokenPanel (props: Props) {
  const {
    onCancel,
    onAddToken,
    token,
    selectedNetwork
  } = props

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 0 })
  }, [])

  const onClickViewOnBlockExplorer = () => {
    const exporerURL = selectedNetwork.blockExplorerUrls[0]
    if (exporerURL && token?.contractAddress) {
      const url = `${exporerURL}/token/${token?.contractAddress ?? ''}`
      chrome.tabs.create({ url: url }, () => {
        if (chrome.runtime.lastError) {
          console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
        }
      })
    } else {
      alert(getLocale('braveWalletTransactionExplorerMissing'))
    }
  }

  return (
    <StyledWrapper>
      <TopWrapper>
        <TopRow>
          <NetworkText>{selectedNetwork.chainName}</NetworkText>
        </TopRow>
        <Title>{getLocale('braveWalletAddSuggestedTokenTitle')}</Title>
        <Description>{getLocale('braveWalletAddSuggestedTokenDescription')}</Description>
        <AssetIconWithPlaceholder selectedAsset={token} />
        <TokenName>{token?.name ?? ''} ({token?.symbol ?? ''})</TokenName>
        <Tooltip
          text={getLocale('braveWalletTransactionExplorer')}
        >
          <ContractAddress onClick={onClickViewOnBlockExplorer}>{reduceAddress(token?.contractAddress ?? '')}</ContractAddress>
        </Tooltip>
      </TopWrapper>
      <ButtonWrapper>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletBackupButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='confirm'
          text={getLocale('braveWalletWatchListAdd')}
          onSubmit={onAddToken}
        />
      </ButtonWrapper>

    </StyledWrapper>
  )
}

export default AddSuggestedTokenPanel

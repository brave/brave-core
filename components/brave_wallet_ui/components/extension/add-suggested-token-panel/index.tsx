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

// Utils
import { reduceAddress } from '../../../utils/reduce-address'

// Hooks
import { useExplorer } from '../../../common/hooks'

export interface Props {
  onCancel: () => void
  onAddToken: () => void
  selectedNetwork: BraveWallet.EthereumChain
  token?: BraveWallet.BlockchainToken
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

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

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
          <ContractAddress
            onClick={onClickViewOnBlockExplorer('token', token?.contractAddress)}
          >
            {reduceAddress(token?.contractAddress ?? '')}
          </ContractAddress>
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

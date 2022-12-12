// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { BraveWallet, SerializableOriginInfo } from '../../../constants/types'

// Styled Components
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
import { URLText } from '../shared-panel-styles'

// Components
import { withPlaceholderIcon, Tooltip, CreateSiteOrigin } from '../../shared'
import { NavButton } from '..'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

// Hooks
import { useExplorer } from '../../../common/hooks'

export interface Props {
  onCancel: () => void
  onAddToken: () => void
  originInfo: SerializableOriginInfo
  selectedNetwork?: BraveWallet.NetworkInfo
  token?: BraveWallet.BlockchainToken
}

function AddSuggestedTokenPanel (props: Props) {
  const {
    onCancel,
    onAddToken,
    token,
    selectedNetwork,
    originInfo
  } = props

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 0 })
  }, [])

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  return (
    <StyledWrapper>
      <TopWrapper>
        <TopRow>
          <NetworkText>{selectedNetwork?.chainName ?? ''}</NetworkText>
        </TopRow>
        <Title>{getLocale('braveWalletAddSuggestedTokenTitle')}</Title>
        <URLText>
          <CreateSiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </URLText>
        <Description>{getLocale('braveWalletAddSuggestedTokenDescription')}</Description>
        <AssetIconWithPlaceholder asset={token} network={selectedNetwork} />
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
          text={getLocale('braveWalletButtonCancel')}
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

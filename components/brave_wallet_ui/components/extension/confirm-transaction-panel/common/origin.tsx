// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Column, IconsWrapper, Row } from '../../../shared/style'
import { FavIcon, OriginURLText } from './style'
import BraveIcon from '../../../../assets/svg-icons/brave-icon.svg'
import { URLText } from '../../shared-panel-styles'
import { SiteOrigin } from '../../../shared/create-site-origin'
import {
  ContractOriginColumn,
  LaunchIcon
} from '../confirm_simulated_tx_panel.styles'
import {
  InlineAddressButton,
  InlineContractRow,
  OriginIndicatorIconWrapper,
  OriginWarningIndicator
} from '../style'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getIsBraveWalletOrigin } from '../../../../utils/string-utils'
import { getLocale } from '../../../../../common/locale'

interface Props {
  originInfo: BraveWallet.OriginInfo
}

export function Origin(props: Props) {
  const { originInfo } = props
  return (
    <>
      <FavIcon
        src={
          getIsBraveWalletOrigin(originInfo)
            ? BraveIcon
            : `chrome://favicon/size/64@1x/${originInfo.originSpec}`
        }
      />
      <URLText>
        <SiteOrigin
          originSpec={originInfo.originSpec}
          eTldPlusOne={originInfo.eTldPlusOne}
        />
      </URLText>
    </>
  )
}

export function TransactionOrigin({
  contractAddress,
  originInfo,
  onClickContractAddress,
  isFlagged
}: Props & {
  contractAddress?: string
  onClickContractAddress?: (address: string) => void
  isFlagged?: boolean
}) {
  // computed
  const isBraveWalletOrigin = getIsBraveWalletOrigin(originInfo)

  // render
  return (
    <Row alignItems='center' justifyContent='flex-start' padding={'16px 0px'}>
      <Column
        width='30px'
        height='30px'
        alignItems='stretch'
        justifyContent='stretch'
        margin={'0px 8px 0px 0px'}
      >
        <IconsWrapper marginRight='0px'>
          <FavIcon
            height='30px'
            src={
              isBraveWalletOrigin
                ? BraveIcon
                : `chrome://favicon/size/64@1x/${originInfo.originSpec}`
            }
          />
          {!isBraveWalletOrigin && isFlagged && (
            <OriginIndicatorIconWrapper>
              <OriginWarningIndicator />
            </OriginIndicatorIconWrapper>
          )}
        </IconsWrapper>
      </Column>
      <ContractOriginColumn>
        <OriginURLText>
          <SiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </OriginURLText>
        {contractAddress ? (
          <InlineContractRow>
            {getLocale('braveWalletContract')}
            <InlineAddressButton
              title={getLocale('braveWalletTransactionExplorer')}
              onClick={() => onClickContractAddress?.(contractAddress)}
            >
              {reduceAddress(contractAddress)} <LaunchIcon />
            </InlineAddressButton>
          </InlineContractRow>
        ) : null}
      </ContractOriginColumn>
    </Row>
  )
}

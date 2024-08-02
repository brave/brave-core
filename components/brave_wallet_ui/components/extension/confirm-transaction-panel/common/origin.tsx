// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Images
import BraveIcon from '../../../../assets/svg-icons/brave-icon.svg'

// Components
import { SiteOrigin } from '../../../shared/create-site-origin'
import {
  ChainInfo,
  InlineViewOnBlockExplorerIconButton
} from './view_on_explorer_button'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'

// Styled Components
import { Column, IconsWrapper, Row } from '../../../shared/style'
import { FavIcon } from './style'
import { URLText } from '../../shared-panel-styles'
import {
  ContractOriginColumn,
  InlineContractRow,
  OriginIndicatorIconWrapper,
  OriginURLText,
  OriginWarningIndicator
} from './origin.style'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { reduceAddress } from '../../../../utils/reduce-address'
import {
  getIsBraveWalletOrigin,
  isComponentInStorybook
} from '../../../../utils/string-utils'
import { getLocale } from '../../../../../common/locale'

interface Props {
  originInfo: BraveWallet.OriginInfo
}

const isStorybook = isComponentInStorybook()

export function Origin(props: Props) {
  const { originInfo } = props
  return (
    <>
      <FavIcon
        src={
          getIsBraveWalletOrigin(originInfo)
            ? BraveIcon
            : isStorybook
            ? `${originInfo.originSpec}/favicon.png`
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
  isFlagged,
  network
}: Props & {
  contractAddress?: string
  network: ChainInfo
  isFlagged?: boolean
}) {
  // computed
  const isBraveWalletOrigin = getIsBraveWalletOrigin(originInfo)

  // render
  return (
    <Row
      alignItems='center'
      justifyContent='flex-start'
      padding={'16px 0px'}
    >
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
                : isStorybook
                ? `${originInfo.originSpec}/favicon.png`
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
      <ContractOriginColumn
        alignItems='flex-start'
        justifyContent='flex-start'
        gap={'4px'}
      >
        <OriginURLText>
          <SiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </OriginURLText>
        {contractAddress ? (
          <InlineContractRow>
            <span>{getLocale('braveWalletContract')}</span>

            <CopyTooltip
              isAddress
              text={contractAddress}
              tooltipText={contractAddress}
            >
              <InlineContractRow>
                <span>{reduceAddress(contractAddress)}</span>
                <InlineViewOnBlockExplorerIconButton
                  address={contractAddress}
                  network={network}
                  urlType='contract'
                />
              </InlineContractRow>
            </CopyTooltip>
          </InlineContractRow>
        ) : null}
      </ContractOriginColumn>
    </Row>
  )
}

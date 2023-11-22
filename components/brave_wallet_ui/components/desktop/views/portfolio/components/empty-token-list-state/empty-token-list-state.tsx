// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import { useHistory } from 'react-router'

// Types
import { WalletRoutes } from '../../../../../../constants/types'

// Utils
import { getLocale } from '../../../../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  Title,
  Description,
  ButtonWrapper,
  EmptyStateIcon
} from './empty-token-list-state.style'
import {
  Row,
  VerticalSpace,
  HorizontalSpace
} from '../../../../../shared/style'

export const EmptyTokenListState = () => {
  // routing
  const history = useHistory()

  return (
    <StyledWrapper
      fullWidth={true}
      alignItems='center'
      justifyContent='center'
    >
      <EmptyStateIcon />
      <Title
        textSize='16px'
        isBold={true}
      >
        {getLocale('braveWalletNoAvailableAssets')}
      </Title>
      <Description
        textSize='14px'
        isBold={false}
      >
        {getLocale('braveWalletNoAvailableAssetsDescription')}
      </Description>
      <VerticalSpace space='24px' />
      <Row marginBottom={4}>
        <ButtonWrapper>
          <Button
            kind='outline'
            onClick={() => history.push(WalletRoutes.FundWalletPageStart)}
          >
            {getLocale('braveWalletBuyCryptoButton')}
          </Button>
        </ButtonWrapper>
        <HorizontalSpace space='12px' />
        <ButtonWrapper>
          <Button
            kind='outline'
            onClick={() => history.push(WalletRoutes.DepositFundsPageStart)}
          >
            {getLocale('braveWalletDepositCryptoButton')}
          </Button>
        </ButtonWrapper>
      </Row>
      <Description
        textSize='12px'
        isBold={false}
      >
        {getLocale('braveWalletWelcomeDividerText')}
      </Description>
      <Button
        kind='plain'
        onClick={() => history.push(WalletRoutes.AddAssetModal)}
      >
        {getLocale('braveWalletWatchlistAddCustomAsset')}
      </Button>
    </StyledWrapper>
  )
}

export default EmptyTokenListState

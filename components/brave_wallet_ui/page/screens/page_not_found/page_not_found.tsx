// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

import Button from '@brave/leo/react/button'

// Constants
import { WalletRoutes } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import {
  WalletPageWrapper, //
} from '../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper'

// Styled Components
import {
  ContentWrapper,
  PageNotFoundIllustration,
  StyledWrapper,
  Title,
} from './page_not_found.style'
import { Column, Text } from '../../../components/shared/style'

export const PageNotFound = () => {
  // routing
  const history = useHistory()

  return (
    <WalletPageWrapper
      wrapContentInBox={false}
      hideNav={true}
      hideHeaderMenu={true}
      noBorderRadius={true}
      noPadding={true}
      useFullHeight={true}
    >
      <StyledWrapper>
        <ContentWrapper padding='40px 48px'>
          <PageNotFoundIllustration />
          <Column
            gap='16px'
            margin='0px 0px 32px 0px'
          >
            <Title
              textColor='primary'
              isBold={true}
            >
              {getLocale(S.BRAVE_WALLET_PAGE_NOT_FOUND_TITLE)}
            </Title>
            <Text
              textColor='tertiary'
              textSize='14px'
              isBold={false}
            >
              {getLocale(S.BRAVE_WALLET_PAGE_NOT_FOUND_DESCRIPTION)}
            </Text>
          </Column>
          <div>
            <Button
              size='medium'
              onClick={() => history.push(WalletRoutes.Portfolio)}
            >
              {getLocale(S.BRAVE_WALLET_GO_TO_PORTFOLIO)}
            </Button>
          </div>
        </ContentWrapper>
      </StyledWrapper>
    </WalletPageWrapper>
  )
}

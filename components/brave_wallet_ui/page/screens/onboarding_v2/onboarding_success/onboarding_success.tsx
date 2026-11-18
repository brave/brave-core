// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Redux
import { useAppDispatch } from '$wallet/common/hooks/use-redux'

// Utils
import { getLocale } from '$web-common/locale'
import { WalletPageActions } from '$wallet/page/actions'
import { useDiscoverAssetsMutation } from '$wallet/common/slices/api.slice'

// Constants
import { WalletRoutes } from '$wallet/constants/types'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'
import { ConfettiBurst } from './confetti_burst'

// Styled Components
import { Row, Column, Text } from '$wallet/components/shared/style'
import {
  Description,
  CelebrationIcon,
  CelebrationSlot,
} from './onboarding_success.style'
import { Button } from '../onboarding.style'

export const OnboardingSuccess = () => {
  // Routing
  const history = useHistory()

  // Redux
  const dispatch = useAppDispatch()

  // Mutations
  const [discoverAssets] = useDiscoverAssetsMutation()

  // Methods
  const onComplete = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.PortfolioAssets)
  }, [dispatch, history])

  // Effects
  React.useEffect(() => {
    // now that the token registry is populated, discover assets
    discoverAssets()
  }, [discoverAssets])

  // Render
  return (
    <OnboardingContentLayout showBackButton={false}>
      <Column width='100%'>
        <CelebrationSlot>
          <ConfettiBurst />
          <CelebrationIcon>🎉</CelebrationIcon>
        </CelebrationSlot>
        <Column
          gap='4px'
          margin='0 0 32px 0'
        >
          <Text
            variant='heading.display3'
            textColor='primary'
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_WALLET_READY_TITLE)}
          </Text>
          <Description
            variant='large.regular'
            textColor='secondary'
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_WALLET_READY_DESCRIPTION)}
          </Description>
        </Column>
        <Row maxWidth='220px'>
          <Button
            onClick={onComplete}
            size='large'
          >
            {getLocale(S.BRAVE_WALLET_ONBOARDING_OPEN_WALLET_BUTTON)}
          </Button>
        </Row>
      </Column>
    </OnboardingContentLayout>
  )
}

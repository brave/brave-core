// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'
import { WalletPageActions } from '../../../actions'
import {
  useDiscoverAssetsMutation,
  useReportOnboardingActionMutation //
} from '../../../../common/slices/api.slice'

// constants
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// components
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'

// styles
import { Column, VerticalSpace } from '../../../../components/shared/style'
import { IntroImg, Title, SubTitle } from './onboarding_success.style'
import { ContinueButton } from '../onboarding.style'

export const OnboardingSuccess = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // mutations
  const [report] = useReportOnboardingActionMutation()
  const [discoverAssets] = useDiscoverAssetsMutation()

  // methods
  const onComplete = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.PortfolioAssets)
  }, [dispatch, history])

  // effects
  React.useEffect(() => {
    // now that the token registry is populated, discover assets
    discoverAssets()

    report(BraveWallet.OnboardingAction.Complete)
  }, [report, discoverAssets])

  // render
  return (
    <OnboardingContentLayout
      showBackButton={false}
      padding='0 0 100px 0'
    >
      <IntroImg />
      <Title>{getLocale('braveWalletOnboardingSuccessTitle')}</Title>
      <VerticalSpace space='16px' />
      <SubTitle>
        {getLocale('braveWalletOnboardingSuccessDescription')}
      </SubTitle>
      <VerticalSpace space='100px' />
      <Column>
        <ContinueButton onClick={onComplete}>
          {getLocale('braveWalletOnboardingSuccessGoToPortfolio')}
        </ContinueButton>
      </Column>
    </OnboardingContentLayout>
  )
}

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch, useSelector } from 'react-redux'

// redux
import { WalletPageActions } from '../../../actions'
import { PageSelectors } from '../../../selectors'

// utils
import { getLocale } from '../../../../../common/locale'
import { useApiProxy } from '../../../../common/hooks/use-api-proxy'

// components
import {
  WalletPageLayout //
} from '../../../../components/desktop/wallet-page-layout/index'
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button'
import { OnboardingDisclosures } from '../disclosures/disclosures'
import {
  OnboardingNetworkSelection //
} from '../network_selection/onboarding_network_selection'

// types & routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// styles
import {
  Row,
  VerticalSpace,
  WalletWelcomeGraphic
} from '../../../../components/shared/style'
import { OnboardingWrapper } from '../onboarding.style'
import {
  Title,
  ButtonContainer,
  LearnMoreLink,
  BlockQuote,
  BlockQuoteTextContainer,
  VerticalRule,
  SubDivider,
  SubDividerText
} from './onboarding-welcome.style'

type PossibleNextStep =
  | WalletRoutes.OnboardingCreatePassword
  | WalletRoutes.OnboardingConnectHardwareWalletCreatePassword
  | WalletRoutes.OnboardingImportOrRestore
  | WalletRoutes.OnboardingConnectHardwareWalletStart

export const OnboardingWelcome = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const setupStillInProgress = useSelector(PageSelectors.setupStillInProgress)

  // state
  const [showChainSelection, setShowChainSelection] = React.useState(false)
  const [nextStep, setNextStep] = React.useState<PossibleNextStep | undefined>(
    undefined
  )

  // TODO: handle chain selection "Back" button
  // methods
  const {
    hideDisclosures,
    showConnectHardwareDisclosures,
    showNewWalletDisclosures,
    showRestoredWalletDisclosures,
    onAgreeToWalletTerms,
    hideChainSelection
  } = React.useMemo(function () {
    return {
      hideDisclosures: function () {
        setNextStep(undefined)
      },
      hideChainSelection: function () {
        setShowChainSelection(false)
      },
      showNewWalletDisclosures: function () {
        setNextStep(WalletRoutes.OnboardingCreatePassword)
      },
      showRestoredWalletDisclosures: function () {
        setNextStep(WalletRoutes.OnboardingImportOrRestore)
      },
      showConnectHardwareDisclosures: function () {
        setNextStep(WalletRoutes.OnboardingConnectHardwareWalletCreatePassword)
      },
      onAgreeToWalletTerms: function () {
        dispatch(WalletPageActions.agreeToWalletTerms())
        setShowChainSelection(true)
      }
    }
  }, [])

  const onAfterChainSelection = React.useCallback(async () => {
    if (nextStep) {
      history.push(nextStep)
    }
  }, [nextStep])

  // custom hooks
  const { braveWalletP3A } = useApiProxy()

  // effects
  React.useEffect(() => {
    // start wallet setup
    if (!setupStillInProgress) {
      dispatch(WalletPageActions.walletSetupComplete(false))
    }
  }, [setupStillInProgress])

  React.useEffect(() => {
    let action = BraveWallet.OnboardingAction.Shown
    switch (nextStep) {
      case WalletRoutes.OnboardingImportOrRestore:
        action = BraveWallet.OnboardingAction.StartRestore
        break
      case WalletRoutes.OnboardingCreatePassword:
        action = BraveWallet.OnboardingAction.LegalAndPassword
        break
    }
    braveWalletP3A.reportOnboardingAction(action)
  }, [nextStep, braveWalletP3A])

  // render
  if (showChainSelection) {
    return (
      <OnboardingNetworkSelection
        onContinue={onAfterChainSelection}
        isHardwareOnboarding={
          nextStep ===
          WalletRoutes.OnboardingConnectHardwareWalletCreatePassword
        }
        onBack={hideChainSelection}
      />
    )
  }

  if (nextStep !== undefined) {
    return (
      <OnboardingDisclosures
        onContinue={onAgreeToWalletTerms}
        isHardwareOnboarding={
          nextStep ===
          WalletRoutes.OnboardingConnectHardwareWalletCreatePassword
        }
        onBack={hideDisclosures}
      />
    )
  }

  return (
    <WalletPageLayout>
      <OnboardingWrapper>
        <WalletWelcomeGraphic />

        <Title maxWidth='467px'>{getLocale('braveWalletWelcomeTitle')}</Title>

        <BlockQuote>
          <VerticalRule />
          <BlockQuoteTextContainer>
            <span>{getLocale('braveWalletPerksTokens')}</span>
            <span>{getLocale('braveWalletMultiChain')}</span>
            <span>{getLocale('braveWalletPerksBrowserNative')}</span>
          </BlockQuoteTextContainer>
        </BlockQuote>

        <VerticalSpace space='34px' />

        <ButtonContainer>
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletWelcomeButton')}
            onSubmit={showNewWalletDisclosures}
            maxHeight={'48px'}
            minWidth={'267px'}
          />

          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletImportExistingWallet')}
            onSubmit={showRestoredWalletDisclosures}
            maxHeight={'48px'}
            minWidth={'267px'}
          />
        </ButtonContainer>

        <Row>
          <SubDivider />
          <SubDividerText>
            {getLocale('braveWalletWelcomeDividerText')}
          </SubDividerText>
          <SubDivider />
        </Row>

        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletConnectHardwareWallet')}
          onSubmit={showConnectHardwareDisclosures}
          maxHeight={'48px'}
          minWidth={'267px'}
        />

        <VerticalSpace space='20px' />

        <LearnMoreLink
          href='https://support.brave.com/hc/en-us/categories/360001059151-Brave-Wallet'
          target='_blank'
          rel='noreferrer'
        >
          {getLocale('braveWalletLearnMoreAboutBraveWallet')}
        </LearnMoreLink>
      </OnboardingWrapper>
    </WalletPageLayout>
  )
}

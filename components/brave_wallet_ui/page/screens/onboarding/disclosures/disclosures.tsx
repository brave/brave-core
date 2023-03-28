// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch, useSelector } from 'react-redux'

// utils
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'

// routes
import { PageState, WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { Checkbox } from '../../../../components/shared/checkbox/checkbox'
import { NavButton } from '../../../../components/extension/buttons/nav-button/index'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

// styles
import { LinkText, VerticalSpace } from '../../../../components/shared/style'
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper,
  TitleAndDescriptionContainer
} from '../onboarding.style'
import { CheckboxText } from './disclosures.style'

export type OnboardingDisclosuresNextSteps =
  | WalletRoutes.OnboardingCreatePassword
  | WalletRoutes.OnboardingConnectHarwareWalletCreatePassword
  | WalletRoutes.OnboardingImportOrRestore
  | WalletRoutes.OnboardingConnectHardwareWalletStart

interface Props {
  nextStep: OnboardingDisclosuresNextSteps
  onBack?: () => void
  isHardwareOnboarding?: boolean
}

const TermsOfUseText: React.FC<{}> = () => {
  const text = getLocaleWithTag('braveWalletTermsOfServiceCheckboxText')
  return <p key={text.duringTag}>
    {text.beforeTag}
    <LinkText
      href='https://brave.com/terms-of-use/'
      target='_blank'
      rel='noopener noreferrer'
      onClick={
        // prevent checkbox toggle when clicking this link
        (e) => e.stopPropagation()
      }
    >
      {text.duringTag}
    </LinkText>
    {text.afterTag}
  </p>
}

export const OnboardingDisclosures = ({ nextStep, onBack }: Props) => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const walletTermsAcknowledged = useSelector(({ page }: { page: PageState }) => page.walletTermsAcknowledged)

  // state
  const [isResponsibilityCheckboxChecked, setIsResponsibilityCheckboxChecked] = React.useState(
    walletTermsAcknowledged
  )
  const [isTermsCheckboxChecked, setIsTermsCheckboxChecked] = React.useState(
    walletTermsAcknowledged
  )

  // memos
  const isNextStepEnabled = React.useMemo(() => {
    return isResponsibilityCheckboxChecked && isTermsCheckboxChecked
  }, [isResponsibilityCheckboxChecked, isTermsCheckboxChecked])

  // methods
  const onNext = React.useCallback(() => {
    if (isNextStepEnabled) {
      dispatch(WalletPageActions.agreeToWalletTerms())
      history.push(nextStep)
    }
  }, [isNextStepEnabled, nextStep])

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            goBack={onBack}
            currentStep={WalletRoutes.OnboardingWelcome}
            isHardwareOnboarding={nextStep === WalletRoutes.OnboardingConnectHarwareWalletCreatePassword}
            preventSkipAhead
          />

          <TitleAndDescriptionContainer style={{ marginLeft: 18 }}>
            <Title>
              {getLocale('braveWalletDisclosuresTitle')}
            </Title>
            <Description>
              {getLocale('braveWalletDisclosuresDescription')}
            </Description>
          </TitleAndDescriptionContainer>

          <div>
            <Checkbox
              isChecked={isResponsibilityCheckboxChecked}
              onChange={setIsResponsibilityCheckboxChecked}
              alignItems='flex-start'
            >
              <CheckboxText>
                <p>
                  {getLocale('braveWalletSelfCustodyDisclosureCheckboxText')}
                </p>
              </CheckboxText>
            </Checkbox>

            <Checkbox
              isChecked={isTermsCheckboxChecked}
              onChange={setIsTermsCheckboxChecked}
              alignItems='flex-start'
            >
              <CheckboxText>
                <TermsOfUseText />
              </CheckboxText>
            </Checkbox>

            <VerticalSpace space='44px' />
          </div>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonContinue')}
              onSubmit={onNext}
              disabled={!isNextStepEnabled}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default OnboardingDisclosures

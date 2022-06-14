// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// styles
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper,
  VerticalSpace
} from '../onboarding.style'

import {
  CheckboxText
} from './disclosures.style'

// utils
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'

// components
import { Checkbox } from 'brave-ui'
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { LinkText } from '../backup-recovery-phrase/onboarding-backup-recovery-phrase.style'

export const OnboardingDisclosures = () => {
  // routing
  const history = useHistory()

  // state
  const [isResponsibilityCheckboxChecked, setIsResponsibilityCheckboxChecked] = React.useState(false)
  const [isTermsCheckboxChecked, setIsTermsCheckboxChecked] = React.useState(false)

  // memos
  const isNextStepEnabled = React.useMemo(() => {
    return isResponsibilityCheckboxChecked && isTermsCheckboxChecked
  }, [isResponsibilityCheckboxChecked, isTermsCheckboxChecked])

  const termsOfServiceText = React.useMemo(() => {
    const text = getLocaleWithTag('braveWalletTermsOfServiceCheckboxText')
    return <span key={text.duringTag}>
      {text.beforeTag}
      <LinkText
        href='https://brave.com' // TODO
        target='_blank'
        rel='noopener noreferrer'
      >
        {text.duringTag}
      </LinkText>
      {text.afterTag}
    </span>
  }, [])

  // methods
  const nextStep = React.useCallback(() => {
    if (isNextStepEnabled) {
      history.push(WalletRoutes.OnboardingCreatePassword)
    }
  }, [isNextStepEnabled])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            goBackUrl={WalletRoutes.OnboardingWelcome}
            currentStep={WalletRoutes.OnboardingDisclosures}
            preventSkipAhead
          />

          <div>
            <Title>
              {getLocale('braveWalletDisclosuresTitle')}
            </Title>
            <Description>
              {getLocale('braveWalletDisclosuresDescription')}
            </Description>
          </div>

          <div>
            <Checkbox
              value={{ isResponsibilityCheckboxChecked }}
              onChange={(key, isSelected) => {
                setIsResponsibilityCheckboxChecked(isSelected)
              }}
            >
              <div data-key='isResponsibilityCheckboxChecked'>
                <CheckboxText>
                <VerticalSpace space='40px' />
                <p>
                  {getLocale('braveWalletSelfCustodyDisclosureCheckboxText')}
                </p>
                </CheckboxText>
              </div>
            </Checkbox>

            <Checkbox
              value={{ isTermsCheckboxChecked }}
              onChange={(key, isSelected) => {
                setIsTermsCheckboxChecked(isSelected)
              }}
            >
              <div data-key='isTermsCheckboxChecked'>
                <CheckboxText>
                    {termsOfServiceText}
                  <VerticalSpace space='4px' />
                </CheckboxText>
              </div>
            </Checkbox>

            <VerticalSpace space='150px' />
          </div>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonContinue')}
              onSubmit={nextStep}
              disabled={!isNextStepEnabled}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingDisclosures

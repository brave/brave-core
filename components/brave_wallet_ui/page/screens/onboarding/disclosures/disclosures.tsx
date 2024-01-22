// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'
import { PageSelectors } from '../../../selectors'
import { useLocationPathName } from '../../../../common/hooks/use-pathname'
import { getOnboardingTypeFromPath } from '../../../../utils/routes-utils'
import { WalletRoutes } from '../../../../constants/types'
import { useSafePageSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletPageActions } from '../../../actions'

// components
import { Checkbox } from '../../../../components/shared/checkbox/checkbox'

// styles
import { VerticalSpace } from '../../../../components/shared/style'
import { NextButtonRow } from '../onboarding.style'
import { CheckboxText, TermsLink, ContinueButton } from './disclosures.style'
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'

const TermsOfUseText: React.FC<{}> = () => {
  const text = getLocaleWithTag('braveWalletTermsOfServiceCheckboxText')
  return (
    <p key={text.duringTag}>
      {text.beforeTag}
      <TermsLink
        href='https://brave.com/terms-of-use/'
        target='_blank'
        rel='noopener noreferrer'
        onClick={
          // prevent checkbox toggle when clicking this link
          (e) => e.stopPropagation()
        }
      >
        {text.duringTag}
      </TermsLink>
      {text.afterTag}
    </p>
  )
}

export const OnboardingDisclosures = () => {
  // routing
  const history = useHistory()
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)

  // redux
  const dispatch = useDispatch()
  const walletTermsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged
  )

  // state
  const [isResponsibilityCheckboxChecked, setIsResponsibilityCheckboxChecked] =
    React.useState(walletTermsAcknowledged)
  const [isTermsCheckboxChecked, setIsTermsCheckboxChecked] = React.useState(
    walletTermsAcknowledged
  )

  // render
  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletDisclosuresTitle')}
      subTitle={getLocale('braveWalletDisclosuresDescription')}
    >
      <VerticalSpace space='55px' />
      <Checkbox
        isChecked={isResponsibilityCheckboxChecked}
        onChange={setIsResponsibilityCheckboxChecked}
        alignItems='flex-start'
      >
        <CheckboxText>
          <p>{getLocale('braveWalletSelfCustodyDisclosureCheckboxText')}</p>
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

      <NextButtonRow>
        <ContinueButton
          onClick={() => {
            dispatch(WalletPageActions.agreeToWalletTerms())
            history.push(
              onboardingType === 'hardware'
                ? WalletRoutes.OnboardingHardwareWalletNetworkSelection
                : onboardingType === 'import'
                ? WalletRoutes.OnboardingImportNetworkSelection
                : WalletRoutes.OnboardingNewWalletNetworkSelection
            )
          }}
          isDisabled={
            !(isResponsibilityCheckboxChecked && isTermsCheckboxChecked)
          }
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </NextButtonRow>
      <VerticalSpace space='36px' />
    </OnboardingContentLayout>
  )
}

export default OnboardingDisclosures

// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// redux
import { useAppDispatch } from '../../../../common/hooks/use-redux'

// utils
import { getLocale, formatLocale } from '$web-common/locale'
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
import { NextButtonRow, ContinueButton } from '../onboarding.style'
import { CheckboxText, TermsLink } from './disclosures.style'
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/content_layout'

const TermsOfUseText: React.FC<{}> = () => {
  const text = formatLocale('braveWalletTermsOfServiceCheckboxText', {
    $1: (content) => (
      <TermsLink
        href='https://brave.com/terms-of-use/'
        target='_blank'
        rel='noopener noreferrer'
        onClick={
          // prevent checkbox toggle when clicking this link
          (e) => e.stopPropagation()
        }
      >
        {content}
      </TermsLink>
    ),
  })
  return <p>{text}</p>
}

export const OnboardingDisclosures = () => {
  // routing
  const history = useHistory()
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)

  // redux
  const dispatch = useAppDispatch()
  const walletTermsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged,
  )

  // state
  const [isResponsibilityCheckboxChecked, setIsResponsibilityCheckboxChecked] =
    React.useState(walletTermsAcknowledged)
  const [isTermsCheckboxChecked, setIsTermsCheckboxChecked] = React.useState(
    walletTermsAcknowledged,
  )

  // render
  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletDisclosuresTitle')}
      subTitle={getLocale('braveWalletDisclosuresDescription')}
      padding='22px 0 36px'
    >
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
                  : WalletRoutes.OnboardingNewWalletNetworkSelection,
            )
          }}
          isDisabled={
            !(isResponsibilityCheckboxChecked && isTermsCheckboxChecked)
          }
        >
          {getLocale('braveWalletButtonContinue')}
        </ContinueButton>
      </NextButtonRow>
    </OnboardingContentLayout>
  )
}

export default OnboardingDisclosures

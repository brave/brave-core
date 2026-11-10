// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import { WalletRoutes } from '$wallet/constants/types'

// Redux
import { useAppDispatch } from '$wallet/common/hooks/use-redux'

// Selectors
import { PageSelectors } from '$wallet/page/selectors'
import { useSafePageSelector } from '$wallet/common/hooks/use-safe-selector'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'
import { useLocationPathName } from '$wallet/common/hooks/use-pathname'
import { getOnboardingTypeFromPath } from '$wallet/utils/routes-utils'
import { WalletPageActions } from '$wallet/page/actions'

// Components
import { Checkbox } from '$wallet/components/shared/checkbox/checkbox'
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Styled Components
import { Column, Row, Text } from '$wallet/components/shared/style'
import { Button } from '../onboarding.style'
import { CheckboxText, TermsLink } from './onboarding_disclosures.style'

const TermsOfUseText: React.FC<{}> = () => {
  const text = formatLocale(S.BRAVE_WALLET_TERMS_OF_SERVICE_CHECKBOX_TEXT, {
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
  // Routing
  const history = useHistory()
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)

  // Redux
  const dispatch = useAppDispatch()
  const walletTermsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged,
  )

  // State
  const [isResponsibilityCheckboxChecked, setIsResponsibilityCheckboxChecked] =
    React.useState(walletTermsAcknowledged)
  const [isTermsCheckboxChecked, setIsTermsCheckboxChecked] = React.useState(
    walletTermsAcknowledged,
  )

  // Render
  return (
    <OnboardingContentLayout>
      <Column
        width='100%'
        gap='4px'
        margin='0 0 32px 0'
      >
        <Text
          variant='heading.display3'
          textColor='primary'
        >
          {getLocale(S.BRAVE_WALLET_DISCLOSURES_TITLE)}
        </Text>
        <Text
          variant='large.regular'
          textColor='secondary'
        >
          {getLocale(S.BRAVE_WALLET_DISCLOSURES_DESCRIPTION)}
        </Text>
      </Column>
      <Column
        alignItems='flex-start'
        width='100%'
        gap='6px'
        margin='0 0 16px 0'
      >
        <Checkbox
          isChecked={isResponsibilityCheckboxChecked}
          onChange={setIsResponsibilityCheckboxChecked}
          alignItems='flex-start'
          color='brand'
        >
          <CheckboxText>
            <p>
              {getLocale(S.BRAVE_WALLET_SELF_CUSTODY_DISCLOSURE_CHECKBOX_TEXT)}
            </p>
          </CheckboxText>
        </Checkbox>

        <Checkbox
          isChecked={isTermsCheckboxChecked}
          onChange={setIsTermsCheckboxChecked}
          alignItems='flex-start'
          color='brand'
        >
          <CheckboxText>
            <TermsOfUseText />
          </CheckboxText>
        </Checkbox>
      </Column>
      <Row>
        <Button
          size='large'
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
          {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
        </Button>
      </Row>
    </OnboardingContentLayout>
  )
}

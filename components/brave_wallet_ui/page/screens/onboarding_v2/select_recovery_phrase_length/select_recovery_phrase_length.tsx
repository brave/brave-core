// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import { RecoveryPhraseLengths, WalletRoutes } from '$wallet/constants/types'

// Utils
import { getLocale } from '$web-common/locale'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/onboarding_content_layout'

// Styled Components
import { Column, Row, Text } from '$wallet/components/shared/style'
import {
  OptionButton,
  RadioIcon,
  Label,
} from './select_recovery_phrase_length.style'
import { Button } from '../onboarding.style'

interface Props {
  selectedLength?: RecoveryPhraseLengths
  onSelectedLengthChange: (length: RecoveryPhraseLengths) => void
}

export const SelectRecoveryPhraseLength = (props: Props) => {
  const { selectedLength, onSelectedLengthChange } = props

  // Routing
  const history = useHistory()

  // Methods
  const onContinue = React.useCallback(() => {
    if (!selectedLength) {
      return
    }

    history.push(WalletRoutes.OnboardingNewWalletCreatePassword)
  }, [selectedLength, history])

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
          {getLocale(S.BRAVE_WALLET_SELECT_RECOVERY_PHRASE_LENGTH_TITLE)}
        </Text>
        <Text
          variant='large.regular'
          textColor='secondary'
        >
          {getLocale(S.BRAVE_WALLET_SELECT_RECOVERY_PHRASE_LENGTH_DESCRIPTION)}
        </Text>
      </Column>
      <Column
        fullWidth={true}
        gap='32px'
      >
        <Column gap='16px'>
          <OptionButton
            isSelected={selectedLength === '12'}
            onClick={() => onSelectedLengthChange('12')}
          >
            <RadioIcon
              isSelected={selectedLength === '12'}
              name={
                selectedLength === '12' ? 'radio-checked' : 'radio-unchecked'
              }
            />
            <Column alignItems='flex-start'>
              <Row
                width='unset'
                gap='12px'
              >
                <Text
                  textColor='primary'
                  variant='large.semibold'
                >
                  {getLocale(S.BRAVE_WALLET_TWELVE_WORDS)}
                </Text>
                <Label>{getLocale(S.BRAVE_WALLET_RECOMMENDED)}</Label>
              </Row>
              <Text
                textColor='tertiary'
                variant='default.regular'
                textAlign='left'
              >
                {getLocale(S.BRAVE_WALLET_TWELVE_WORDS_DESCRIPTION)}
              </Text>
            </Column>
          </OptionButton>
          <OptionButton
            isSelected={selectedLength === '24'}
            onClick={() => onSelectedLengthChange('24')}
          >
            <RadioIcon
              isSelected={selectedLength === '24'}
              name={
                selectedLength === '24' ? 'radio-checked' : 'radio-unchecked'
              }
            />
            <Column alignItems='flex-start'>
              <Text
                textColor='primary'
                variant='large.semibold'
              >
                {getLocale(S.BRAVE_WALLET_TWENTY_FOUR_WORDS)}
              </Text>
              <Text
                textColor='tertiary'
                variant='default.regular'
                textAlign='left'
              >
                {getLocale(S.BRAVE_WALLET_TWENTY_FOUR_WORDS_DESCRIPTION)}
              </Text>
            </Column>
          </OptionButton>
        </Column>
        <Row>
          <Button
            size='large'
            isDisabled={!selectedLength}
            onClick={onContinue}
          >
            {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
          </Button>
        </Row>
      </Column>
    </OnboardingContentLayout>
  )
}

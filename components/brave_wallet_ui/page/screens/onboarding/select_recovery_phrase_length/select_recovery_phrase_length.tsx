// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'

// Types
import {
  RecoveryPhraseLengths,
  WalletRoutes,
} from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import {
  OnboardingContentLayout, //
} from '../components/onboarding_content_layout/content_layout'

// Styled Components
import { Column, Row, Text } from '../../../../components/shared/style'
import { OptionButton, RadioIcon } from './select_recovery_phrase_length.style'

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
    <OnboardingContentLayout
      showBackButton={true}
      centerContent={true}
      title={getLocale('braveWalletSelectRecoveryPhraseLengthTitle')}
      subTitle={getLocale('braveWalletSelectRecoveryPhraseLengthDescription')}
    >
      <Column
        fullWidth={true}
        gap='84px'
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
                  {getLocale('braveWalletTwelveWords')}
                </Text>
                <Label color='primary'>
                  {getLocale('braveWalletRecommended')}
                </Label>
              </Row>
              <Text
                textColor='tertiary'
                variant='default.regular'
                textAlign='left'
              >
                {getLocale('braveWalletTwelveWordsDescription')}
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
                {getLocale('braveWalletTwentyFourWords')}
              </Text>
              <Text
                textColor='tertiary'
                variant='default.regular'
                textAlign='left'
              >
                {getLocale('braveWalletTwentyFourWordsDescription')}
              </Text>
            </Column>
          </OptionButton>
        </Column>
        <Button
          isDisabled={!selectedLength}
          onClick={onContinue}
        >
          {getLocale('braveWalletButtonContinue')}
        </Button>
      </Column>
    </OnboardingContentLayout>
  )
}

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale, splitStringForTag } from '../../../../../../common/locale'

// styles
import {
  Bold,
  FormLabel,
  FormInput,
  CloseButton,
  CloseIcon
} from '../verify-recovery-phrase.style'
import {
  Column,
  VerticalSpace
} from '../../../../../components/shared/style'
import { InfoAlert } from './verification-progress.style'
import { AlertWrapper } from '../../../onboarding/onboarding.style'

const alertSlotStyle = {
  display: 'flex',
  alignItems: 'center',
  height: '20px'
}

interface Props {
  phrase: string
  wordPosition: number
  showError?: boolean
  onChange: (value: string) => void
  onHideError?: () => void
}

export const PhraseInput = ({
  phrase,
  showError,
  wordPosition,
  onChange,
  onHideError
}: Props) => {
  const { beforeTag, afterTag } = splitStringForTag(
    getLocale('braveWalletRecoveryWordInstructions'),
    1
  )

  return (
    <Column gap='25px'>
      <FormInput
        value={phrase}
        onInput={(event) => onChange(event.detail.value)}
      >
        <FormLabel>
          {beforeTag}
          <Bold>{wordPosition}</Bold>
          {afterTag}
        </FormLabel>
      </FormInput>
      {showError ? (
        <AlertWrapper>
          <InfoAlert
            type='error'
            kind='simple'
          >
            {getLocale('braveWalletVerifyError')}
            <div
              slot='content-after'
              style={alertSlotStyle}
            >
              <CloseButton onClick={onHideError}>
                <CloseIcon />
              </CloseButton>
            </div>
          </InfoAlert>
        </AlertWrapper>
      ) : (
        <VerticalSpace space='54px' />
      )}
    </Column>
  )
}

// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import {
  Bold,
  FormLabel,
  FormInput,
  ErrorAlert,
  CloseButton,
  CloseIcon
} from '../verify-recovery-phrase.style'
import { Column, VerticalSpace } from '../../../../../components/shared/style'

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
  return (
    <Column gap='25px'>
      <FormInput
        value={phrase}
        onInput={(event) => onChange(event.detail.value)}
      >
        <FormLabel>
          Enter the word in position&nbsp;<Bold>{wordPosition}</Bold>&nbsp; from
          your recovery phrase.
        </FormLabel>
      </FormInput>
      {showError ? (
        <ErrorAlert {...{ '--leo-alert-center-width': '100%' }}>
          Recovery phrase doesn’t match.
          <div
            slot='actions'
            style={{ display: 'flex', alignItems: 'center', height: '20px' }}
          >
            <CloseButton onClick={onHideError}>
              <CloseIcon />
            </CloseButton>
          </div>
        </ErrorAlert>
      ) : (
        <VerticalSpace space='54px' />
      )}
    </Column>
  )
}

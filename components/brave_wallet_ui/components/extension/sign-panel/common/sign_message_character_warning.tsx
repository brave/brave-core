// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale } from '../../../../../common/locale'
import { hasUnexpectedSignMessageCharacters } from '../../../../utils/string-utils'

// Styled Components
import { LearnMoreButton, WarningBox } from '../../shared-panel-styles'
import { WarningBoxWrapper } from './sign_message_character_warning.style'
import { Text, Row } from '../../../shared/style'

interface Props {
  messages: Array<string | undefined>
  showFormatted: boolean
  onToggle: () => void
  textColor?: 'primary' | 'error'
  // Domain and message JSON escape nulls and newlines, so scan decoded values.
  decodeJson?: boolean
}

export function SignMessageCharacterWarning(props: Props) {
  const {
    messages,
    showFormatted,
    onToggle,
    textColor = 'primary',
    decodeJson = false,
  } = props
  if (!hasUnexpectedSignMessageCharacters(messages, decodeJson)) {
    return null
  }

  return (
    <WarningBox warningType='warning'>
      <WarningBoxWrapper
        width='100%'
        gap='8px'
      >
        <Row
          justifyContent='flex-start'
          alignItems='flex-start'
          gap='8px'
        >
          <Icon name='warning-triangle-outline' />
          <Text
            textColor={textColor}
            variant='small.semibold'
          >
            {getLocale(
              S.BRAVE_WALLET_SIGN_MESSAGE_UNEXPECTED_CHARACTERS_WARNING,
            )}
          </Text>
        </Row>
        <LearnMoreButton onClick={onToggle}>
          {showFormatted
            ? getLocale(S.BRAVE_WALLET_VIEW_ENCODED_MESSAGE)
            : getLocale(S.BRAVE_WALLET_VIEW_DECODED_MESSAGE)}
        </LearnMoreButton>
      </WarningBoxWrapper>
    </WarningBox>
  )
}

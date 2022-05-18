// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// styles
import {
  RecoveryPhraseContainer,
  RecoveryBubble,
  RecoveryBubbleText,
  FrostedGlass,
  HiddenPhraseContainer,
  EyeOffIcon
} from './recovery-phrase.style'

interface Props {
  hidden: boolean
  recoveryPhrase: string[]
  onClickReveal: () => void
  verificationModeEnabled?: boolean
}

const FAKE_PHRASE_WORDS = new Array(12).fill('Fake')

export const RecoveryPhrase: React.FC<Props> = ({
  hidden,
  onClickReveal,
  recoveryPhrase
}) => {
  if (hidden) {
    return <HiddenPhraseContainer onClick={onClickReveal}>
      <FrostedGlass>
        <EyeOffIcon />
        <p>{getLocale('braveWalletClickToSeeRecoveryPhrase')}</p>
      </FrostedGlass>
      <RecoveryPhraseContainer>
        {FAKE_PHRASE_WORDS.map((word, index) =>
          <RecoveryBubble key={index}>
            <RecoveryBubbleText>{index + 1}. {word}</RecoveryBubbleText>
          </RecoveryBubble>
        )}
      </RecoveryPhraseContainer>
    </HiddenPhraseContainer>
  }

  return (
    <RecoveryPhraseContainer>
      {recoveryPhrase.map((word, index) =>
        <RecoveryBubble key={index}>
          <RecoveryBubbleText>{index + 1}. {word}</RecoveryBubbleText>
        </RecoveryBubble>
      )}
    </RecoveryPhraseContainer>
  )
}

export default RecoveryPhrase

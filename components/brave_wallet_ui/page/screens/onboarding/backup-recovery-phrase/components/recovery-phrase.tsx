// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// images
import EyeOffIcon from '../../../../../assets/svg-icons/eye-off-icon.svg'

// styles
import {
  RecoveryPhraseContainer,
  RecoveryBubble,
  RecoveryBubbleText,
  FrostedGlass,
  HiddenPhraseContainer
} from './recovery-phrase.style'

interface Props {
  hidden: boolean
  recoveryPhrase: string[]
}

const FAKE_PHRASE_WORDS = new Array(12).fill('Fake')

export const RecoveryPhrase: React.FC<Props> = ({
  hidden,
  recoveryPhrase
}) => {
  if (hidden) {
    return <HiddenPhraseContainer>
      <FrostedGlass>
        <img src={EyeOffIcon} />
        <p>Click here to see phrase</p>
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

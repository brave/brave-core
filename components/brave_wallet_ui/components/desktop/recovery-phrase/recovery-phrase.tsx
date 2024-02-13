// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { unbiasedRandom } from '../../../utils/random-utils'

// styles
import {
  RecoveryPhraseContainer,
  RecoveryBubble,
  FrostedGlass,
  HiddenPhraseContainer,
  EyeOffIcon,
  IconWrapper
} from './recovery-phrase.style'

export interface SelectedPhraseWord {
  index: number
  value: string
}

interface Props {
  recoveryPhrase: string[]
  verificationModeEnabled?: boolean
}

const FAKE_PHRASE_WORDS = new Array(12).fill('Fake')

export const RecoveryPhrase: React.FC<Props> = ({
  recoveryPhrase,
  verificationModeEnabled
}) => {
  // state
  const [hidden, setHidden] = React.useState(true)

  // memos
  const shuffledPhrase: string[] = React.useMemo(() => {
    const array = recoveryPhrase.slice().sort()

    for (let i = array.length - 1; i > 0; i--) {
      let j = unbiasedRandom(0, array.length - 1)
      let temp = array[i]
      array[i] = array[j]
      array[j] = temp
    }
    return array
  }, [recoveryPhrase])

  const phraseWordsToDisplay: SelectedPhraseWord[] = React.useMemo(() => {
    return (verificationModeEnabled ? shuffledPhrase : recoveryPhrase).map(
      (str, index) => ({ value: str, index })
    )
  }, [verificationModeEnabled, shuffledPhrase, recoveryPhrase])

  // render
  if (hidden) {
    return (
      <HiddenPhraseContainer onMouseEnter={() => setHidden(false)}>
        <FrostedGlass>
          <IconWrapper>
            <EyeOffIcon />
          </IconWrapper>
        </FrostedGlass>
        <RecoveryPhraseContainer phraseLength={12}>
          {FAKE_PHRASE_WORDS.map((word, index) => (
            <RecoveryBubble key={index}>
              <span>
                {index + 1}. {word}
              </span>
            </RecoveryBubble>
          ))}
        </RecoveryPhraseContainer>
      </HiddenPhraseContainer>
    )
  }

  return (
    <RecoveryPhraseContainer
      phraseLength={recoveryPhrase.length}
      onMouseLeave={() => setHidden(true)}
    >
      {phraseWordsToDisplay.map((word) => (
        <RecoveryBubble key={`${word.index}-${word.value}`}>
          <span>
            {word.index + 1}. {word.value}
          </span>
        </RecoveryBubble>
      ))}
    </RecoveryPhraseContainer>
  )
}

export default RecoveryPhrase

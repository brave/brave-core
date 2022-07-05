// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'
import { unbiasedRandom } from '../../../../../utils/random-utils'

// styles
import {
  RecoveryPhraseContainer,
  RecoveryBubble,
  FrostedGlass,
  HiddenPhraseContainer,
  EyeOffIcon,
  RecoveryBubbleBadge
} from './recovery-phrase.style'

export interface SelectedPhraseWord {
  index: number
  value: string
}

interface Props {
  hidden: boolean
  recoveryPhrase: string[]
  onClickReveal?: () => void
  verificationModeEnabled?: boolean
  onVerifyUpdate?: (doesWordOrderMatch: boolean) => void
  onSelectedWordListChange?: (words: SelectedPhraseWord[]) => void
}

const LAST_PHRASE_WORD_INDEX = 11
const FAKE_PHRASE_WORDS = new Array(LAST_PHRASE_WORD_INDEX + 1).fill('Fake')
const SELECTED_WORD_ORDINALS = {
  0: getLocale('braveWalletOrdinalFirst'),
  1: getLocale('braveWalletOrdinalThird'),
  2: getLocale('braveWalletOrdinalLast')
}

const getNewSelectedWordsList = (prevWords: SelectedPhraseWord[], word: SelectedPhraseWord): SelectedPhraseWord[] => {
  // remove from list if word already selected
  if (prevWords.find((prevWord) => prevWord.index === word.index)) {
    return prevWords.filter((prevWord) => prevWord.index !== word.index)
  }

  // 3 words selected max
  if (prevWords.length > 2) {
    return prevWords
  }

  // add word to list
  return [...prevWords, word]
}

export const RecoveryPhrase: React.FC<Props> = ({
  hidden,
  onClickReveal,
  onVerifyUpdate,
  onSelectedWordListChange,
  recoveryPhrase,
  verificationModeEnabled
}) => {
  // state
  const [selectedWords, setSelectedWords] = React.useState<SelectedPhraseWord[]>([])

  // methods
  const makeOnClickWord = React.useCallback((word: SelectedPhraseWord) => () => {
    if (verificationModeEnabled) {
      setSelectedWords(prev => getNewSelectedWordsList(prev, word))
    }
  }, [verificationModeEnabled])

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
    return (
      verificationModeEnabled
        ? shuffledPhrase
        : recoveryPhrase
      ).map((str, index) => ({ value: str, index }))
  }, [verificationModeEnabled, shuffledPhrase, recoveryPhrase])

  // effects
  React.useEffect(() => {
    // exit early if not monitoring list updates
    if (!onSelectedWordListChange) {
      return
    }

    onSelectedWordListChange(selectedWords)
  }, [selectedWords, onSelectedWordListChange])

  React.useEffect(() => {
    // exit early if not monitoring verification
    if (!onVerifyUpdate) {
      return
    }

    // wrong length
    if (selectedWords.length !== 3) {
      onVerifyUpdate(false)
      return
    }

    // check order
    const firstWordMatch = selectedWords[0].value === recoveryPhrase[0] // first
    const thirdWordMatch = selectedWords[1].value === recoveryPhrase[2] // third
    const lastWordMatch = selectedWords[2].value === recoveryPhrase[LAST_PHRASE_WORD_INDEX] // last (12th)

    onVerifyUpdate(firstWordMatch && thirdWordMatch && lastWordMatch)
  }, [selectedWords, onVerifyUpdate])

  // render
  if (hidden) {
    return <HiddenPhraseContainer onClick={onClickReveal}>
      <FrostedGlass>
        <EyeOffIcon />
        <p>{getLocale('braveWalletClickToSeeRecoveryPhrase')}</p>
      </FrostedGlass>
      <RecoveryPhraseContainer>
        {FAKE_PHRASE_WORDS.map((word, index) =>
          <RecoveryBubble
            key={index}
            verificationModeEnabled={verificationModeEnabled}
          >
            <span>{index + 1}. {word}</span>
          </RecoveryBubble>
        )}
      </RecoveryPhraseContainer>
    </HiddenPhraseContainer>
  }

  return (
    <RecoveryPhraseContainer>
      {phraseWordsToDisplay.map((word) => {
        const wordIndex = selectedWords?.findIndex((selectedWord) =>
          selectedWord.index === word.index &&
          selectedWord.value === word.value
        )
        const isWordSelected = wordIndex > -1
        return (
          <RecoveryBubble
            key={`${word.index}-${word.value}`}
            verificationModeEnabled={verificationModeEnabled}
            onClick={makeOnClickWord(word)}
            selected={isWordSelected}
          >

            {isWordSelected &&
              <RecoveryBubbleBadge>
                {SELECTED_WORD_ORDINALS[wordIndex] || ''}
              </RecoveryBubbleBadge>
            }

            <span>
              {verificationModeEnabled
                ? word.value
                : `${word.index + 1}. ${word.value}`
              }
            </span>

          </RecoveryBubble>
        )
      })}
    </RecoveryPhraseContainer>
  )
}

export default RecoveryPhrase

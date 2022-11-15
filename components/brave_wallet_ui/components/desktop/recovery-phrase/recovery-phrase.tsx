// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'
import { formatOrdinals } from '../../../utils/ordinal-utils'
import { unbiasedRandom } from '../../../utils/random-utils'

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
  verificationIndices?: number[]
  onClickReveal?: () => void
  verificationModeEnabled?: boolean
  onSelectedWordListChange?: (words: SelectedPhraseWord[], doesWordOrderMatch: boolean) => void
}

const FAKE_PHRASE_WORDS = new Array(12).fill('Fake')

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
  onSelectedWordListChange,
  recoveryPhrase,
  verificationModeEnabled,
  verificationIndices
}) => {
  // state
  const [selectedWords, setSelectedWords] = React.useState<SelectedPhraseWord[]>([])

  // methods
  const makeOnClickWord = React.useCallback((word: SelectedPhraseWord) => () => {
    if (verificationModeEnabled && verificationIndices) {
      const newSelectedWords = getNewSelectedWordsList(selectedWords, word)

      setSelectedWords(newSelectedWords)

      if (!onSelectedWordListChange) {
        return
      }

      // wrong length
      if (newSelectedWords.length !== 3) {
        onSelectedWordListChange(newSelectedWords, false)
        return
      }

      // check order
      const firstWordMatch = newSelectedWords[0].value === recoveryPhrase[verificationIndices[0]]

      const secondWordMatch = selectedWords[1].value === recoveryPhrase[verificationIndices[1]]

      const lastWordMatch = newSelectedWords[2].value === recoveryPhrase[verificationIndices[2]]

      const match = (firstWordMatch && secondWordMatch && lastWordMatch)

      onSelectedWordListChange(newSelectedWords, match)
    }
  }, [
    verificationModeEnabled,
    selectedWords,
    verificationIndices,
    onSelectedWordListChange,
    recoveryPhrase
  ])

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

            {isWordSelected && verificationIndices &&
              <RecoveryBubbleBadge>
                {formatOrdinals(verificationIndices[wordIndex] + 1)}
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

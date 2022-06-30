// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import { PageState, RecoveryObject } from '../../../../constants/types'
import { getLocale } from '../../../../../common/locale'
import { unbiasedRandom } from '../../../../utils/random-utils'

// components
import { NavButton } from '../../../../components/extension'
import { DropBoundary } from '../../../../components/shared/drop-boundary'

// style
import {
  StyledWrapper,
  Title,
  Description,
  RecoveryBubble,
  RecoveryBubbleText,
  RecoveryPhraseContainer,
  SelectedPhraseContainer,
  SelectedBubble,
  SelectedBubbleText,
  ErrorText,
  ErrorContainer,
  PlaceholderBubble
} from './backup-wallet-verify.style'

export interface Props {
  onNextStep: () => void
}

export function BackupWalletVerifyStep ({
  onNextStep
}: Props) {
  // redux
  const mnemonic = useSelector(({ page }: { page: PageState }) => page?.mnemonic)
  const recoveryPhrase = React.useMemo(() => (mnemonic || '').split(' '), [mnemonic])

  // state
  const [hasVerifyError, setVerifyError] = React.useState<boolean>(false)
  const [sortedPhrase, setSortedPhrase] = React.useState<RecoveryObject[]>([])
  const phraseSlots = React.useMemo(() => new Array<RecoveryObject>(
    recoveryPhrase?.slice()?.length || 0
  ).fill({ id: 1, value: '' }).map((_, index) => ({ id: index + 1, value: '' })), [recoveryPhrase])

  const shuffledPhrase = React.useMemo(() => {
    const array = recoveryPhrase.slice().sort()

    for (let i = array.length - 1; i > 0; i--) {
      let j = unbiasedRandom(0, array.length - 1)
      let temp = array[i]
      array[i] = array[j]
      array[j] = temp
    }
    return array.map((str, index) => ({ value: str, id: index + 1 }))
  }, [recoveryPhrase])

  const isDisabled = React.useMemo((): boolean => {
    return sortedPhrase.length !== shuffledPhrase.length
  }, [sortedPhrase, shuffledPhrase])

  const numberOfWordRows = React.useMemo((): number => {
    const numberOfWordColumns = 4
    return Math.ceil(shuffledPhrase.length / numberOfWordColumns)
  }, [shuffledPhrase])

  // methods
  const isWordSelected = (word: RecoveryObject) => {
    const searchable = sortedPhrase.map(phraseWord => `${phraseWord.id}-${phraseWord.value}`)
    const wordIndex = searchable.findIndex((sorted) => sorted === `${word.id}-${word.value}`)
    return {
      isSelected: wordIndex > -1,
      index: wordIndex
    }
  }

  const selectWord = (word: RecoveryObject, positionIndex?: number) => {
    const wasPositionProvided = (positionIndex === 0 || positionIndex)

    let tempList = [...sortedPhrase]

    if (wasPositionProvided) {
      if (isWordSelected(word).isSelected) {
        tempList = tempList.filter((listWord) => listWord.id !== word.id) // remove word for it's current position
      }
      tempList.splice(positionIndex, 0, word)
    }

    setSortedPhrase(wasPositionProvided ? tempList : [...sortedPhrase, word])
    setVerifyError(false)
  }

  const unSelectWord = (word: RecoveryObject) => {
    const newList = sortedPhrase.filter((key) => key !== word)
    setSortedPhrase(newList)
  }

  const addWord = (word: RecoveryObject, positionIndex?: number) => () => {
    selectWord(word, positionIndex)
  }

  const removeWord = (word: RecoveryObject) => () => {
    unSelectWord(word)
  }

  const showError = () => {
    setVerifyError(true)
    setTimeout(function () { setVerifyError(false) }, 3000)
  }

  const checkPhrase = () => {
    if (sortedPhrase.length === recoveryPhrase.length && sortedPhrase.every((v, i) => v.value === recoveryPhrase[i])) {
      onNextStep()
    } else {
      setSortedPhrase([])
      showError()
    }
  }

  const onWordDragStart = (word: RecoveryObject) => (event: React.DragEvent) => {
    event.dataTransfer.setData('text', JSON.stringify(word))
  }

  const onDrop = (event: React.DragEvent<HTMLDivElement>, index: number): void => {
    event.preventDefault()
    const data = event.dataTransfer.getData('text')
    const word = data ? JSON.parse(data) as RecoveryObject : undefined
    if (word) {
      selectWord(word, index)
    }
  }

  // render
  return (
    <StyledWrapper>
      <Title>{getLocale('braveWalletVerifyRecoveryTitle')}</Title>
      <Description>{getLocale('braveWalletVerifyRecoveryDescription')}</Description>

      <SelectedPhraseContainer error={hasVerifyError} numberOfRows={numberOfWordRows}>

        {/* SLOTS */}
        {phraseSlots.map((_word, index) => {
          const wordInSlot = sortedPhrase?.[index]
          return (
            <DropBoundary
              key={index}
              onDrop={(event) => onDrop(event, index)}
            >
              {(isDraggedOver) => wordInSlot
                ? <SelectedBubble
                  key={index}
                  isDraggedOver={isDraggedOver}
                  draggable={true}
                  onDrag={wordInSlot ? onWordDragStart(wordInSlot) : undefined}
                  onDragStart={wordInSlot ? onWordDragStart(wordInSlot) : undefined}
                  onClick={() => {
                    if (wordInSlot?.value) {
                      removeWord(wordInSlot)()
                    }
                  }}
                >
                  <SelectedBubbleText
                    isInCorrectPosition={recoveryPhrase[index] === wordInSlot?.value}
                  >
                    {index + 1}. {wordInSlot.value}
                  </SelectedBubbleText>
                </SelectedBubble>
                : <PlaceholderBubble />
              }
            </DropBoundary>
          )
        })}

        {hasVerifyError &&
          <ErrorContainer>
            <ErrorText>{getLocale('braveWalletVerifyError')}</ErrorText>
          </ErrorContainer>
        }
      </SelectedPhraseContainer>

      <RecoveryPhraseContainer>
        {shuffledPhrase.map((word) => {
          const { isSelected } = isWordSelected(word)
          return (
            <RecoveryBubble
              id={`${word.id}-${word.value}`}
              key={word.id}
              onClick={addWord(word)}
              disabled={isSelected}
              isSelected={isSelected}
              draggable={true}
              onDragStart={onWordDragStart(word)}
            >
              <RecoveryBubbleText
                id={`text-${word.id}-${word.value}`}
                isSelected={isSelected}
              >
                {word.value}
              </RecoveryBubbleText>
            </RecoveryBubble>)
        })}
      </RecoveryPhraseContainer>

      <NavButton disabled={isDisabled} buttonType='primary' text={getLocale('braveWalletButtonVerify')} onSubmit={checkPhrase} />
    </StyledWrapper>
  )
}

export default BackupWalletVerifyStep

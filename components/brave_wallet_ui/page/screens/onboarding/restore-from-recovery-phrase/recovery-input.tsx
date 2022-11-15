// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'
import { clearClipboard } from '../../../../utils/copy-to-clipboard'
import { removeDoubleSpaces } from '../../../../utils/string-utils'
import { cleanupRecoveryPhraseInput, isPhraseLengthValid } from '../../../../utils/recovery-phrase-utils'

// style
import { RecoveryTextArea, RecoveryTextInput } from './restore-from-recovery-phrase.style'
import { PhraseCardBody, PhraseCardBottomRow, PhraseCardTopRow } from '../onboarding.style'
import { ToggleVisibilityButton, WalletLink } from '../../../../components/shared/style'

interface Props {
  onChange: (results: { value: string, isValid: boolean, phraseLength: number }) => void
  onKeyDown: (event: React.KeyboardEvent<HTMLElement>) => void
  onToggleShowPhrase: (isShown: boolean) => void
}

export const RecoveryInput = ({
  onChange,
  onKeyDown,
  onToggleShowPhrase
}: Props) => {
  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState<boolean>(false)
  const [inputValue, setInputValue] = React.useState<string>('')

  // methods
  const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    const cleanedInput = cleanupRecoveryPhraseInput(event.target.value)
    const { isInvalid, wordsInPhraseValue } = isPhraseLengthValid(cleanedInput)

    // update local state
    setInputValue(cleanedInput)

    // update parent
    onChange({
      value: cleanedInput,
      isValid: !isInvalid,
      phraseLength: wordsInPhraseValue
    })
  }, [onChange])

  const toggleShowPhrase = React.useCallback(() => {
    setIsPhraseShown(!isPhraseShown)
    onToggleShowPhrase(!isPhraseShown)
  }, [onToggleShowPhrase, isPhraseShown])

  const onClickPasteFromClipboard = React.useCallback(async () => {
    const phraseFromClipboard = await navigator.clipboard.readText()
    clearClipboard()

    const removedDoubleSpaces = removeDoubleSpaces(phraseFromClipboard)

    handleChange({
      target: { value: removedDoubleSpaces }
    } as React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>)
  }, [handleChange])

  const onPasteFromClipboard = React.useCallback<React.ClipboardEventHandler<HTMLTextAreaElement | HTMLInputElement>>(async (event) => {
    const value = event.clipboardData.getData('Text')
    await clearClipboard()
    const removedDoubleSpaces = removeDoubleSpaces(value)

    handleChange({
      target: { value: removedDoubleSpaces }
    } as React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>)
  }, [handleChange])

  const onInputBlur = React.useCallback<React.FocusEventHandler<HTMLTextAreaElement | HTMLInputElement>>((event) => {
    const removedDoubleSpaces = removeDoubleSpaces(event.target.value)
    if (removedDoubleSpaces !== event.target.value) {
      handleChange({
        target: { value: removedDoubleSpaces }
      } as React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>)
    }
  }, [handleChange])

  // render
  return <>
    <PhraseCardTopRow>
      <ToggleVisibilityButton
        isVisible={isPhraseShown}
        onClick={toggleShowPhrase}
      />
    </PhraseCardTopRow>

    <PhraseCardBody>
      {isPhraseShown
        ? <RecoveryTextArea
            onChange={handleChange}
            onPaste={onPasteFromClipboard}
            onKeyDown={onKeyDown}
            onBlur={onInputBlur}
            value={inputValue}
            autoComplete='off'
          />
        : <RecoveryTextInput
            type='password'
            value={inputValue}
            onChange={handleChange}
            onBlur={onInputBlur}
            onPaste={onPasteFromClipboard}
            onKeyDown={onKeyDown}
            autoComplete='off'
          />
      }
    </PhraseCardBody>

    <PhraseCardBottomRow centered>
      <WalletLink
        as='button'
        onClick={onClickPasteFromClipboard}
      >
        {getLocale('braveWalletPasteFromClipboard')}
      </WalletLink>
    </PhraseCardBottomRow>
  </>
}

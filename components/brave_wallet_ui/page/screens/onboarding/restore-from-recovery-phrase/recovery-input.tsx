// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'
import { clearClipboard } from '../../../../utils/copy-to-clipboard'

// style
import { RecoveryTextDiv, RecoveryTextInput } from './restore-from-recovery-phrase.style'
import { PhraseCardBody, PhraseCardBottomRow, PhraseCardTopRow } from '../onboarding.style'
import { ToggleVisibilityButton, WalletLink } from '../../../../components/shared/style'

interface Props {
  onChange: (results: { value: string, isValid: boolean, phraseLength: number }) => void
  onKeyDown: (event: React.KeyboardEvent<HTMLElement>) => void
  onToggleShowPhrase: (isShown: boolean) => void
}

interface State {
  value: string
  isPhraseShown: boolean
}

function replaceCaret (el: HTMLElement) {
  // Place the caret at the end of the element
  const target = document.createTextNode('')
  el.appendChild(target)
  // do not move caret if element was not focused
  const isTargetFocused = document.activeElement === el
  if (target !== null && target.nodeValue !== null && isTargetFocused) {
    const sel = window.getSelection()
    if (sel !== null) {
      const range = document.createRange()
      range.setStart(target, target.nodeValue.length)
      range.collapse(true)
      sel.removeAllRanges()
      sel.addRange(range)
    }
    if (el instanceof HTMLElement) {
      el.focus()
    }
  }
}

// Forked from: https://github.com/lovasoa/react-contenteditable
// Using a class component due to this issue: https://github.com/lovasoa/react-contenteditable/issues/161
export class RecoveryInput extends React.Component<Props, State> {
  state: State = {
    isPhraseShown: false,
    value: ''
  }

  el: React.RefObject<HTMLDivElement> = React.createRef<HTMLDivElement>()

  componentDidUpdate = () => {
    if (!this.el.current) return
    replaceCaret(this.el.current)
  }

  handleChange = (event: React.ChangeEvent<
    HTMLInputElement | HTMLDivElement
  >) => {
    const inputValue = (event.target as HTMLInputElement)?.value
    const divValue = event.target.innerText

    const value = inputValue || divValue

    // This prevents there from being a space at the begining of the phrase.
    const removedBeginingWhiteSpace = value.trimStart()

    // This Prevents there from being more than one space between words.
    const removedDoubleSpaces = removedBeginingWhiteSpace.replace(/ +(?= )/g, '')

    // Although the above removes double spaces, it is initialy recognized as a
    // a double-space before it is removed and macOS automatically replaces double-spaces with a period.
    const removePeriod = removedDoubleSpaces.replace(/['/.']/g, '')

    // max length
    // the editable-content div input value is handled differently than the password input value for some reason
    const maxLength = this.state.isPhraseShown ? 24 : 25

    // This prevents an extra space at the end of a 24 word phrase.
    const needsCleaning = removedDoubleSpaces.split(' ').length === maxLength

    const cleanedInput = needsCleaning
      ? removePeriod.trimEnd()
      : removePeriod

    const wordsInPhraseValue = cleanedInput.trim().split(/\s+/g).length

    // valid lengths: 12, 15, 18, 21, or 24
    const isInvalid =
      wordsInPhraseValue < 12 ||
      wordsInPhraseValue > 12 && wordsInPhraseValue < 15 ||
      wordsInPhraseValue > 15 && wordsInPhraseValue < 18 ||
      wordsInPhraseValue > 18 && wordsInPhraseValue < 21 ||
      wordsInPhraseValue > 21 && wordsInPhraseValue < 24

    // update parent
    this.props.onChange({
      value: cleanedInput,
      isValid: !isInvalid,
      phraseLength: wordsInPhraseValue
    })

    // update local state
    this.setState({ value: cleanedInput })
    if (this.el.current) {
      this.el.current.innerText = cleanedInput
    }
  }

  toggleShowPhrase = () => {
    this.setState(prev => {
      this.props.onToggleShowPhrase(!prev.isPhraseShown)
      return {
        ...prev,
        isPhraseShown: !prev.isPhraseShown
      }
    })
  }

  onClickPasteFromClipboard = async () => {
    const phraseFromClipboard = await navigator.clipboard.readText()
    clearClipboard()
    this.handleChange({
      target: { value: phraseFromClipboard }
    } as React.ChangeEvent<HTMLInputElement>)
  }

  render = () => {
    const {
      isPhraseShown,
      value
    } = this.state
    const { onKeyDown } = this.props

    return <>
      <PhraseCardTopRow>
        <ToggleVisibilityButton
          isVisible={isPhraseShown}
          onClick={this.toggleShowPhrase}
        />
      </PhraseCardTopRow>

      <PhraseCardBody>
        {isPhraseShown
          ? <RecoveryTextDiv
              ref={this.el}
              onInput={this.handleChange}
              onPaste={clearClipboard}
              onKeyDown={onKeyDown}
              contentEditable
            >
              {value}
            </RecoveryTextDiv>
          : <RecoveryTextInput
              type='password'
              value={value}
              onChange={this.handleChange}
              onPaste={clearClipboard}
              onKeyDown={onKeyDown}
              autoComplete='off'
            />
        }
      </PhraseCardBody>

      <PhraseCardBottomRow centered>
        <WalletLink
          as='button'
          onClick={this.onClickPasteFromClipboard}
        >
          {getLocale('braveWalletPasteFromClipboard')}
        </WalletLink>
      </PhraseCardBottomRow>
    </>
  }
}

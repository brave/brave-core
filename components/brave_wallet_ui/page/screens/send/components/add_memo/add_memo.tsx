// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Constants
import { MAX_ZCASH_MEMO_LENGTH } from '../../constants/magics'

// Utils
import { getLocale } from '../../../../../../common/locale'

// Styled Components
import { Column, Text } from '../../../../../components/shared/style'
import { MemoInput } from './add_memo.style'

interface Props {
  memoText: string
  onUpdateMemoText: (value: string) => void
}

export const AddMemo = (props: Props) => {
  const { memoText, onUpdateMemoText } = props

  // State
  const [showMemoTextInput, setShowMemoTextInput] =
    React.useState<boolean>(false)

  // Memos
  const memoTextLength = React.useMemo(() => {
    return memoText.length
  }, [memoText])

  // Methods
  const onAddOrRemoveTextMemo = React.useCallback(() => {
    if (showMemoTextInput) {
      onUpdateMemoText('')
      setShowMemoTextInput(false)
      return
    }
    setShowMemoTextInput(true)
  }, [showMemoTextInput, onUpdateMemoText])

  return (
    <Column
      fullWidth={true}
      alignItems='flex-start'
      gap='8px'
      padding='16px 0px 0px 0px'
    >
      {showMemoTextInput && (
        <MemoInput
          value={memoText}
          onInput={(e) => onUpdateMemoText(e.value)}
          placeholder={getLocale('braveWalletEnterAMessage')}
          showErrors={memoText.length > MAX_ZCASH_MEMO_LENGTH}
        >
          <Text
            textSize='12px'
            isBold={true}
            textColor='primary'
          >
            {getLocale('braveWalletMessageOptional')}
          </Text>
          <span slot='extra'>
            {memoTextLength}/{MAX_ZCASH_MEMO_LENGTH}
          </span>
          <Text
            textSize='12px'
            isBold={true}
            textColor='error'
            slot='errors'
            textAlign='left'
          >
            {getLocale('braveWalletMemoLengthError')}
          </Text>
        </MemoInput>
      )}
      <Button
        kind='plain'
        size='medium'
        onClick={onAddOrRemoveTextMemo}
      >
        <Icon
          name={showMemoTextInput ? 'trash' : 'plus-add'}
          slot='icon-before'
        />
        {showMemoTextInput
          ? getLocale('braveWalletRemoveMemo')
          : getLocale('braveWalletAddMemo')}
      </Button>
    </Column>
  )
}

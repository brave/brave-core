// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  RecoveryPhraseContainer,
  RecoveryBubble,
  RecoveryBubbleText
} from './recovery-phrase.style'

interface Props {
  hidden: boolean
  recoveryPhrase: string[]
}

export const RecoveryPhrase: React.FC<Props> = ({
  hidden,
  recoveryPhrase
}) => {
  if (hidden) {
    return <></>
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

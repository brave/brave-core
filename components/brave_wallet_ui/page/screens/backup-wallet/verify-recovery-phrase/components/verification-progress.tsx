// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Wrapper, Rectangle } from './verification-progress.style'

interface Props {
  steps: number
  currentStep: number
}

export const VerificationProgress = ({ steps, currentStep }: Props) => {
  return (
    <Wrapper>
      {Array.from({ length: steps }).map((_, index) => (
        <Rectangle
          key={index}
          isActive={currentStep >= index}
          width={index === 0 ? 20 : 16}
        />
      ))}
    </Wrapper>
  )
}

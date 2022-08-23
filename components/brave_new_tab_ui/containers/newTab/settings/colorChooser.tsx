// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { StyledCustomBackgroundSettings } from '../../../components/default'
import NavigateBack from '../../../components/default/settings/navigateBack'
import ColorBackgroundOption from './colorBackgroundOption'

interface Props {
  title: string
  values: string[]
  currentValue?: string
  onSelectValue: (value: string) => void
  onBack: () => void
}

function ColorChooser ({ title, values, onBack, onSelectValue, currentValue }: Props) {
  const containerEl = React.useRef<HTMLDivElement>(null)
  React.useEffect(() => {
    containerEl.current?.scrollIntoView(true)
  }, [])

  return (
      <div ref={containerEl}>
        <NavigateBack onBack={onBack} title={title} />
        <StyledCustomBackgroundSettings>
          {values.map(value => <ColorBackgroundOption key={value} color={value} onSelectValue={onSelectValue} selected={currentValue === value} />)}
        </StyledCustomBackgroundSettings>
      </div>
  )
}

export default ColorChooser

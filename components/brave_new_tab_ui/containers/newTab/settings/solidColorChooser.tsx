// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { StyledCustomBackgroundSettings } from '../../../components/default'
import NavigateBack from '../../../components/default/settings/navigateBack'
import SolidColorBackgroundOption from './solidColorBackgroundOption'
import { solidColorsForBackground } from '../../../data/colors'
import { getLocale } from '$web-common/locale'

interface Props {
  currentColor?: string
  setSolidColorBackground: (color: string) => void
  onBack: () => void
}

function SolidColorChooser (props: Props) {
  const { onBack, setSolidColorBackground, currentColor } = props

  const containerEl = React.useRef<HTMLDivElement>(null)
  React.useEffect(() => {
    containerEl.current?.scrollIntoView(true)
  }, [])

  return (
      <div ref={containerEl}>
        <NavigateBack onBack={onBack} title={getLocale('solidColorTitle')} />
        <StyledCustomBackgroundSettings>
          {solidColorsForBackground.map(color => <SolidColorBackgroundOption key={color} color={color} onSelectColor={setSolidColorBackground} selected={currentColor === color} />)}
        </StyledCustomBackgroundSettings>
      </div>
  )
}

export default SolidColorChooser

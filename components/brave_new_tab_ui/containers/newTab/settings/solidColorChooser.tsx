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
  useSolidColorBackground: (color: string) => void
  onBack: () => void
}

class SolidColorChooser extends React.PureComponent<Props, {}> {
  #containerElem: React.RefObject<HTMLDivElement>

  constructor (props: Props) {
    super(props)
    this.#containerElem = React.createRef()
  }

  onBack = () => {
    this.props.onBack()
  }

  componentDidMount () {
    this.#containerElem.current?.scrollIntoView(true)
  }

  render () {
    return (
      <div ref={this.#containerElem}>
        <NavigateBack onBack={this.onBack} title={getLocale('solidColorTitle')} />
        <StyledCustomBackgroundSettings>
          {solidColorsForBackground.map((color: string) => {
            return (
              <SolidColorBackgroundOption
                key={color}
                color={color}
                useSolidColorBackground={this.props.useSolidColorBackground}
                selected={this.props.currentColor === color}
              />
            )
          })}
        </StyledCustomBackgroundSettings>
      </div>
    )
  }
}

export default SolidColorChooser

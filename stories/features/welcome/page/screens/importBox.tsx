/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph } from '../../../../../src/features/welcome/'

// Shared components
import { Button } from '../../../../../src/components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeImportImage } from '../../../../../src/features/welcome/images'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

interface State {
  onClickFired: boolean
}

export default class ImportBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { onClickFired: false }
  }

  onClickImport = () => {
    this.setState({ onClickFired: !this.state.onClickFired })
    this.props.onClick()
  }

  render () {
    const { index, currentScreen } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeImportImage />
        <Title>{locale.importFromAnotherBrowser}</Title>
        <Paragraph>{locale.setupImport}</Paragraph>
          <Button
            level='primary'
            type='accent'
            size='large'
            text={this.state.onClickFired ? locale.import : locale.import}
            onClick={this.onClickImport}
          />
      </Content>
    )
  }
}

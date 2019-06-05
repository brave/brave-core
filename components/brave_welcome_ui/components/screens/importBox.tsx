/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, PrimaryButton } from 'brave-ui/features/welcome'

// Images
import { WelcomeImportImage } from 'brave-ui/features/welcome/images'

// Utils
import { getLocale } from '../../../common/locale'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

export default class ImportBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeImportImage />
        <Title>{getLocale('importFromAnotherBrowser')}</Title>
        <Paragraph>{getLocale('setupImport')}</Paragraph>
          <PrimaryButton
            level='primary'
            type='accent'
            size='large'
            text={getLocale('import')}
            onClick={onClick}
          />
      </Content>
    )
  }
}

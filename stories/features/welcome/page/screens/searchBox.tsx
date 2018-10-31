
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, SearchImage, Paragraph } from '../../../../../src/features/welcome/'

// Shared components
import { Button } from '../../../../../src/components'

// Utils
import locale from '../fakeLocale'

// Images
const searchImage = require('../../../../assets/img/welcome_search.svg')

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

export default class SearchEngineBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick } = this.props
    return (
      <Content zIndex={index} active={index === currentScreen}>
        <SearchImage src={searchImage} />
        <Title>{locale.setDefaultSearchEngine}</Title>
        <Paragraph>{locale.chooseSearchEngine}</Paragraph>
          <Button
            level='primary'
            type='accent'
            size='large'
            text={locale.search}
            onClick={onClick}
          />
      </Content>
    )
  }
}


/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { Content, Title, Paragraph, SelectGrid } from '../../../../../src/features/welcome/'
import { SelectBox } from '../../../../../src/features/shields'

// Shared components
import { Button } from '../../../../../src/components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeSearchImage } from '../../../../../src/features/welcome/images'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
  fakeOnChange: () => void
}

export default class SearchEngineBox extends React.PureComponent<Props, {}> {
  render () {
    const { index, currentScreen, onClick, fakeOnChange } = this.props
    return (
      <Content
        zIndex={index}
        active={index === currentScreen}
        screenPosition={'1' + (index + 1) + '0%'}
        isPrevious={index <= currentScreen}
      >
        <WelcomeSearchImage />
        <Title>{locale.setDefaultSearchEngine}</Title>
        <Paragraph>{locale.chooseSearchEngine}</Paragraph>
          <SelectGrid>
            <SelectBox onChange={fakeOnChange}>
              <option value='DuckDuckGo'>{locale.fakeSearchProvider1}</option>
              <option value='Google'>{locale.fakeSearchProvider2}</option>
              <option value='Bing'>{locale.fakeSearchProvider3}</option>
            </SelectBox>
            <Button
              level='primary'
              type='accent'
              size='large'
              text={locale.confirm}
              onClick={onClick}
            />
          </SelectGrid>
      </Content>
    )
  }
}

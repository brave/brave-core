/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Content,
  Title,
  Paragraph,
  PrimaryButton,
  SelectGrid,
  SelectBox
} from '../../../components'

// Utils
import locale from '../fakeLocale'

// Images
import { WelcomeImportImage } from '../../../components/images'

interface Props {
  index: number
  currentScreen: number
  onClick: () => void
}

interface State {
  importSelected: boolean
}

export default class ImportBox extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      importSelected: false
    }
  }

  onClickImport = () => {
    this.props.onClick()
  }

  onChangeImportOption = (event: React.ChangeEvent<HTMLSelectElement>) => {
    this.setState({ importSelected: event.target.value !== '' })
  }

  render () {
    const { index, currentScreen } = this.props
    const { importSelected } = this.state
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
          <SelectGrid>
              <SelectBox onChange={this.onChangeImportOption}>
                <option value=''>{locale.importFrom}</option>
                <option value='Chrome'>{locale.fakeBrowser1}</option>
                <option value='Firefox'>{locale.fakeBrowser2}</option>
              </SelectBox>
              <PrimaryButton
                level='primary'
                type='accent'
                size='large'
                text={locale.import}
                disabled={!importSelected}
                onClick={this.onClickImport}
              />
            </SelectGrid>
      </Content>
    )
  }
}

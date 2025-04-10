/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import * as React from 'react'
import { ModalTitle } from './basic/display'
import { Container, HorizontalContainer, LeftAlignedItem, PaddedButton, RightAlignedItem, TextSection } from './basic/structure'
import SettingsCard, { SettingItem } from './SettingsCard'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

 export interface Props {
    titleText: string
    descriptionText: string
    settingItems: SettingItem[]
    onSubmitReport: () => void
    onClose: () => void
  }

 export default class PsstProgressModal extends React.PureComponent<Props, {}> {
    constructor (props: Props) {
      super(props)
    }

    render () {
        const {
          titleText,
          descriptionText,
          settingItems,
          onClose
          } = this.props

        return (
        <Container>
          <HorizontalContainer>
            <LeftAlignedItem>
              <TextSection>
                <ModalTitle>
                  {titleText}
                </ModalTitle>
              </TextSection>
            </LeftAlignedItem>
            <RightAlignedItem>
              <Button fab kind='plain-faint' onClick={onClose}>
                <Icon name={"close-circle"}/>
              </Button>
            </RightAlignedItem>
          </HorizontalContainer>
          <TextSection>
              {descriptionText}
          </TextSection>
          <SettingsCard settingItems={settingItems}/>
          <RightAlignedItem>
              <PaddedButton
                kind='outline'
                size='medium'
                onClick={() => console.log('click 1')}
              >
                {'Cancel'}
              </PaddedButton>
              <PaddedButton
                kind='filled'
                size='medium'
                onClick={() => console.log('click 1')}
              >
                {'Apply changes'}
              </PaddedButton>

          </RightAlignedItem>
        </Container>)
    }
}
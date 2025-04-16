/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { CheckBoxIconCompleted, CheckBoxIconFailed, SettingGrid, SettingGridHeaderRow, SettingGridRow, SettingProgressRing } from './basic/display'
import Checkbox from '@brave/leo/react/checkbox'
import { PsstProgressModalState, SettingState } from './PsstProgressModal'
import { HorizontalContainer, TextLabel, TextSection } from './basic/structure'

export interface Props {
  title: string
  subTitle: string
  progressModelState: PsstProgressModalState | null
  onItemChecked: (url: string, checked: boolean) => void
}

export default class SettingsCard extends React.PureComponent<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  render () {
    const {
      title,
      subTitle,
      progressModelState,
      onItemChecked
    } = this.props

    return (
      <SettingGrid>
        <SettingGridHeaderRow>
          <div style={iconContainerStyle}>
            {/* <img
              src="https://abs.twimg.com/favicons/favicon.ico" // Replace with the actual X icon URL
              alt="X Icon"
              style={iconStyle}
            /> */}
          </div>
          <div>
            <div style={titleStyle}>{title} X</div>
            <div style={urlStyle}>{subTitle}</div>
          </div>
        </SettingGridHeaderRow>
        {
        progressModelState && progressModelState.optionsStatuses && Array.from(progressModelState.optionsStatuses.values()).map((item) => (
           <SettingGridRow key={item.url}>
            {(() => {
              if (item.settingState == SettingState.Progress) {
                console.log('[PSST] Progress Icon')
                return (<HorizontalContainer>
                  <SettingProgressRing mode="indeterminate" />
                  <TextSection><TextLabel>{item.description}</TextLabel></TextSection>
                </HorizontalContainer>)
              } else if (item.settingState == SettingState.Selection) {
                console.log('[PSST] CheckBox')
                return (<Checkbox checked={item.checked} isDisabled={item.disabled} onChange={(e) => onItemChecked(item.url, e.checked)}>
                  <TextLabel>{item.description}</TextLabel>
                </Checkbox>)
              } else if (item.settingState == SettingState.Completed) {
                return (
                  <HorizontalContainer>
                    <CheckBoxIconCompleted name={"check-circle-outline"}/>
                    <TextLabel>{item.description}</TextLabel>
                  </HorizontalContainer>)
              } else if (item.settingState == SettingState.Failed) {
                console.log('[PSST] Error Icon')
                return (
                  <HorizontalContainer>
                    <CheckBoxIconFailed name={"close-circle"}/>
                    <TextLabel>{item.description}</TextLabel>
                  </HorizontalContainer>)
              } else {
                return null
              }
            })()}
           </SettingGridRow>
        ))
        }
      </SettingGrid>
    )
  }

};


const iconContainerStyle = {
  width: '32px',
  height: '32px',
  borderRadius: '4px',
  backgroundColor: '#fff', // White background for the icon
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  marginRight: '12px',
};


const titleStyle = {
  fontWeight: 'bold',
  fontSize: '16px',
  color: '#333',
};

const urlStyle = {
  fontSize: '14px',
  color: '#777',
};

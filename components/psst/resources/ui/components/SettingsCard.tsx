/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import Checkbox from '@brave/leo/react/checkbox'
import Ring from '@brave/leo/react/progressRing'
import Icon from '@brave/leo/react/icon'
import { PsstProgressModalState, SettingState } from './PsstProgressModal'
import { LeftAlignedItem } from './basic/structure'
import { color, font } from '@brave/leo/tokens/css/variables'
import Flex from '$web-common/Flex'

// Styled components
const TextSection = styled.div`
`

const SettingGrid = styled(LeftAlignedItem)`
  border-radius: 8px;
  border: ${color.primitive.neutral[90]} 1px solid;
  margin-bottom: 18px;
`

const SettingGridHeaderRow = styled(LeftAlignedItem)`
  padding: 16px;
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  background-color: ${color.container.interactive};
  display: flex;
  align-items: center;
`

const SettingGridRow = styled(LeftAlignedItem)`
  padding: 16px;
  border-top: ${color.primitive.neutral[90]} 1px solid;
`

const SettingsGridBoldText = styled(TextSection)`
  color: ${color.text.primary};
  font: ${font.default.semibold};
`

const SettingsGridSmallText = styled(TextSection)`
  color: ${color.text.primary};
  font: ${font.small.regular};
`

const SettingProgressRing = styled(Ring)`
  --leo-progressring-size: 18px;
    margin-right: 10px;
`

const CheckBoxIconCompleted = styled(Icon)`
  --leo-icon-color: ${color.systemfeedback.successIcon};
  --leo-icon-size: 20px;
  margin-right: 8px;
`

const CheckBoxIconFailed = styled(Icon)`
  --leo-icon-color: ${color.systemfeedback.errorIcon};
  --leo-icon-size: 20px;
  margin-right: 8px;
`

export const TextLabel = styled('label') <{}>`
  font: ${font.default.regular};
  color: ${color.text.primary};
`

export interface Props {
  title: string
  subTitle: string
  progressModelState: PsstProgressModalState | null
  onItemChecked: (url: string, checked: boolean) => void
}

export default class SettingsCard extends React.PureComponent<Props, {}> {
  constructor(props: Props) {
    super(props)
  }

  render() {
    const { title, subTitle, progressModelState, onItemChecked } = this.props

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
            <SettingsGridBoldText>{title}</SettingsGridBoldText>
            <SettingsGridSmallText>{subTitle}</SettingsGridSmallText>
          </div>
        </SettingGridHeaderRow>
        {progressModelState
          && progressModelState.optionsStatuses
          && Array.from(progressModelState.optionsStatuses.values()).map(
            (item) => (
              <SettingGridRow key={item.url}>
                {(() => {
                  if (item.settingState === SettingState.Progress) {
                    return (
                      <Flex
                        direction='row'
                        justify='flex-start'
                        align='flex-start'
                      >
                        <SettingProgressRing mode='indeterminate' />
                        <TextSection>
                          <TextLabel>{item.description}</TextLabel>
                        </TextSection>
                      </Flex>
                    )
                  } else if (item.settingState === SettingState.Selection) {
                    return (
                      <Checkbox
                        checked={item.checked}
                        isDisabled={item.disabled}
                        onChange={(e) => onItemChecked(item.url, e.checked)}
                      >
                        <TextLabel>{item.description}</TextLabel>
                      </Checkbox>
                    )
                  } else if (item.settingState === SettingState.Completed) {
                    return (
                      <Flex
                        direction='row'
                        justify='flex-start'
                        align='flex-start'
                      >
                        <CheckBoxIconCompleted name='check-circle-outline' />
                        <TextLabel>{item.description}</TextLabel>
                      </Flex>
                    )
                  } else if (item.settingState === SettingState.Failed) {
                    return (
                      <Flex
                        direction='row'
                        justify='flex-start'
                        align='flex-start'
                      >
                        <CheckBoxIconFailed name='close-circle' />
                        <TextLabel>{item.description}</TextLabel>
                      </Flex>
                    )
                  } else {
                    return null
                  }
                })()}
              </SettingGridRow>
            ),
          )}
      </SettingGrid>
    )
  }
}

const iconContainerStyle = {
  width: '32px',
  height: '32px',
  borderRadius: '4px',
  backgroundColor: '#fff', // White background for the icon
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  marginRight: '12px',
}

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
import { color, font } from '@brave/leo/tokens/css/variables'
import Flex from '$web-common/Flex'

// Styled components
const SettingGrid = styled.div`
  border-radius: 8px;
  border: ${color.primitive.neutral[90]} 1px solid;
  margin-bottom: 18px;
`

const SettingGridHeaderRow = styled.div`
  padding: 16px;
  border-top-left-radius: 8px;
  border-top-right-radius: 8px;
  background-color: ${color.container.interactive};
  display: flex;
  align-items: center;
`

const SettingGridRow = styled.div`
  padding: 16px;
  border-top: ${color.primitive.neutral[90]} 1px solid;
`

const SettingsGridBoldText = styled.div`
  color: ${color.text.primary};
  font: ${font.default.semibold};
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

const SettingText = styled.span`
  font: ${font.default.regular};
  color: ${color.text.primary};
`
const IconContainer = styled.div`
  width: 32px;
  height: 32px;
  border-radius: 4px;
  background-color: ${color.white};
  display: flex;
  justify-content: center;
  align-items: center;
  margin-right: 12px;
`

export interface Props {
  title: string
  progressModelState: PsstProgressModalState | null
  onItemChecked: (uid: string, checked: boolean) => void
}

const SettingsCard: React.FC<Props> = ({
  title,
  progressModelState,
  onItemChecked,
}) => {
  return (
    <SettingGrid>
      <SettingGridHeaderRow>
        <IconContainer />
        <div>
          <SettingsGridBoldText>{title}</SettingsGridBoldText>
        </div>
      </SettingGridHeaderRow>
      {progressModelState
        && progressModelState.optionsStatuses
        && Array.from(progressModelState.optionsStatuses.values()).map(
          (item) => (
            <SettingGridRow key={item.uid}>
              {item.settingState === SettingState.Progress && (
                <Flex
                  direction='row'
                  justify='flex-start'
                  align='flex-start'
                >
                  <SettingProgressRing mode='indeterminate' />
                  <SettingText>{item.description}</SettingText>
                </Flex>
              )}
              {item.settingState === SettingState.Selection && (
                <Checkbox
                  checked={item.checked}
                  isDisabled={item.disabled}
                  onChange={(e) => onItemChecked(item.uid, e.checked)}
                >
                  {item.description}
                </Checkbox>
              )}
              {item.settingState === SettingState.Completed && (
                <Flex
                  direction='row'
                  justify='flex-start'
                  align='flex-start'
                >
                  <CheckBoxIconCompleted name='check-circle-outline' />
                  <SettingText>{item.description}</SettingText>
                </Flex>
              )}
              {item.settingState === SettingState.Failed && (
                <Flex
                  direction='row'
                  justify='flex-start'
                  align='flex-start'
                >
                  <CheckBoxIconFailed name='close-circle' />
                  <SettingText>{item.description}</SettingText>
                </Flex>
              )}
            </SettingGridRow>
          ),
        )}
    </SettingGrid>
  )
}

export default React.memo(SettingsCard)

/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Ring from '@brave/leo/react/progressRing'
import { color, font, spacing, radius } from '@brave/leo/tokens/css/variables'

import Flex from '$web-common/Flex'

import { OptionStatus, SettingState } from './PsstProgressModal'

// Styled components
const SettingGrid = styled.div`
  border-radius: ${radius.m};
  border: ${color.divider.subtle} 1px solid;
  margin-bottom: ${spacing['2Xl']};
`

const SettingGridHeaderRow = styled.div`
  padding: ${spacing.l} ${spacing.xl} ${spacing.l} ${spacing.xl};
  border-top-left-radius: ${radius.m};
  border-top-right-radius: ${radius.m};
  background-color: ${color.container.highlight};
  display: flex;
  align-items: center;
`

const SettingGridRow = styled.div`
  padding: ${spacing.l};
  border-top: ${color.divider.subtle} 1px solid;
`

const SettingsGridBoldText = styled.div`
  color: ${color.text.secondary};
  font: ${font.default.semibold};
`

const SettingProgressRing = styled(Ring)`
  --leo-progressring-size: ${spacing.xl};
  padding-right: ${spacing.l};
`

const CheckBoxIconCompleted = styled(Icon)`
  --leo-icon-color: ${color.systemfeedback.successIcon};
  --leo-icon-size: ${spacing.xl};
  padding-right: ${spacing.l};
`

const CheckBoxIconFailed = styled(Icon)`
  --leo-icon-color: ${color.systemfeedback.errorIcon};
  --leo-icon-size: ${spacing.xl};
  display: flex;
  padding-right: ${spacing.l};
`

const SettingText = styled.span`
  font: ${font.default.regular};
  color: ${color.text.secondary};
`
const SettingErrorText = styled(SettingText)`
  font: ${font.default.regular};
  color: ${color.systemfeedback.errorText};
`

const DummyIconContainer = styled.div`
  width: ${spacing['3Xl']};
  height: ${spacing['3Xl']};
  border-radius: ${radius.m};
  background-color: ${color.container.background};
  display: flex;
  justify-content: center;
  align-items: center;
  margin-right: ${spacing.l};
`
interface PsstProgressModalState {
  siteName: string
  optionsStatuses: OptionStatus[] | undefined
}

export interface Props {
  title: string
  progressModelState: PsstProgressModalState | undefined
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
        <DummyIconContainer />
        <div>
          <SettingsGridBoldText>{title}</SettingsGridBoldText>
        </div>
      </SettingGridHeaderRow>
      {progressModelState
        && progressModelState.optionsStatuses
        && Array.from(progressModelState.optionsStatuses)
          .filter((item) => item.settingState !== SettingState.None)
          .map((item) => (
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
                  align='center'
                >
                  <CheckBoxIconFailed name='close-circle' />
                  <Flex
                    direction='column'
                    justify='flex-start'
                    align='flex-start'
                  >
                    <SettingText>{item.description}</SettingText>
                    <SettingErrorText>{item.error}</SettingErrorText>
                  </Flex>
                </Flex>
              )}
            </SettingGridRow>
          ))}
    </SettingGrid>
  )
}

export default React.memo(SettingsCard)

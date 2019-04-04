/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledCard,
  StyledDescription,
  StyledSettingsIcon,
  StyledContent,
  StyledTitle,
  StyledSettingsWrapper,
  StyledSettingsClose,
  StyledSettingsTitle,
  StyledSettingsText,
  StyledSettingTitleWrapper,
  StyledContentWrapper,
  StyledFlip,
  StyledSettingsToggleContainer
} from './style'
import { Tooltip } from '../'
import Toggle from '../../../components/formControls/toggle/index'
import { getLocale } from '../../../helpers'
import { CloseCircleOIcon, SettingsIcon } from '../../../components/icons'

export type Type = 'ads' | 'contribute' | 'donation'

export interface Props {
  title: string
  id?: string
  description?: string
  toggle?: boolean
  checked?: boolean
  attachedAlert?: React.ReactNode
  onToggle?: () => void
  settingsChild?: React.ReactNode
  disabledContent?: React.ReactNode
  children?: React.ReactNode
  testId?: string
  type: Type
  onSettingsClick?: () => void
  settingsOpened?: boolean
}

/*
  TODO
  - add fade effect
 */
export default class Box extends React.PureComponent<Props, {}> {
  getSettingsTitle = (title: string) => {
    return `${title} ${getLocale('settings')}`
  }

  render () {
    const {
      id,
      title,
      toggle,
      checked,
      attachedAlert,
      onToggle,
      settingsChild,
      disabledContent,
      description,
      type,
      children,
      testId,
      settingsOpened,
      onSettingsClick
    } = this.props

    return (
      <StyledWrapper>
        <StyledCard
          testId={id}
          hasAlert={!!attachedAlert}
        >
          <StyledFlip>
            <StyledContentWrapper open={!settingsOpened}>
              <StyledSettingTitleWrapper>
                <StyledTitle type={type} checked={checked}>
                  {title}
                </StyledTitle>
                <StyledSettingsToggleContainer>
                  {
                    settingsChild && ((toggle && checked) || !toggle) ?
                      <Tooltip
                        id={'brave-ads-tip'}
                        content={this.getSettingsTitle(title)}
                      >
                        <StyledSettingsIcon onClick={onSettingsClick}>
                          <SettingsIcon />
                        </StyledSettingsIcon>
                      </Tooltip>
                      : null
                  }
                  {
                    toggle ?
                      <Toggle onToggle={onToggle} checked={checked} testId={testId} />
                      : null
                  }
                </StyledSettingsToggleContainer>
              </StyledSettingTitleWrapper>
              <StyledDescription>
                {description}
              </StyledDescription>
              <StyledContent>
                {
                  disabledContent
                    ? disabledContent
                    : children
                }
              </StyledContent>
            </StyledContentWrapper>
            <StyledSettingsWrapper open={settingsOpened}>
              <StyledSettingsClose onClick={onSettingsClick} open={settingsOpened}>
                <CloseCircleOIcon />
              </StyledSettingsClose>
              <StyledSettingsTitle>
                <StyledSettingsText>{this.getSettingsTitle(title)}</StyledSettingsText>
              </StyledSettingsTitle>
              {settingsChild}
            </StyledSettingsWrapper>
          </StyledFlip>
        </StyledCard>
        {attachedAlert}
      </StyledWrapper>
    )
  }
}

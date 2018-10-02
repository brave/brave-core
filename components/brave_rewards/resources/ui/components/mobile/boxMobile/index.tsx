/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledCard,
  StyledLeft,
  StyledRight,
  StyledDescription,
  StyledContent,
  StyledTitle,
  StyledBreak,
  StyledContentWrapper,
  StyledFlip,
  StyleDetailsLink,
  StyledDetailContent,
  StyledSettingsListTitle,
  StyledArrow,
  StyledToggleHeader,
  StyledBackArrow,
  StyledFullSizeWrapper,
  StyledDetailInfo,
  StyledSettingsContent,
  StyledSettingsHeader,
  StyledSettingsTitle,
  StyledSettingsClose,
  StyledChildContent,
  StyledSettingsIcon,
  StyledSettingsText,
  StyledToggleWrapper
} from './style'
import {
  ArrowIcon,
  CaratRightIcon,
  CloseStrokeIcon,
  SettingsIcon
} from '../../../../components/icons'
import { List } from '../../'
import { getLocale } from '../../../../helpers'
import Toggle from '../../../../components/formControls/toggle/index'

export type Type = 'ads' | 'contribute' | 'donation'

export interface Props {
  type: Type
  id?: string
  title: string
  description?: string
  settingsChild?: React.ReactNode
  children?: React.ReactNode
  toggle?: boolean
  checked?: boolean
  toggleAction?: () => void
}

interface State {
  contentShown: boolean
  settingsOpened: boolean
}

export default class BoxMobile extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      contentShown: false,
      settingsOpened: false
    }
  }

  detailsClick = () => {
    this.setState({
      contentShown: !this.state.contentShown
    })
  }

  settingsClick = () => {
    this.setState({
      settingsOpened: !this.state.settingsOpened
    })
  }

  onToggle = () => {
    if (this.props.checked) {
      this.setState({
        contentShown: false,
        settingsOpened: false
      })
    }

    if (this.props.toggleAction) {
      this.props.toggleAction()
    }
  }

  getSettingsTitle = (title: string) => {
    return `${title} ${getLocale('settings')}`
  }

  getToggleHeader = (props: Props) => {
    const {
      title,
      type,
      checked,
      toggle
    } = props
    const isDetailView = checked && this.state.contentShown

    return (
      <StyledToggleHeader detailView={isDetailView}>
        <StyledLeft>
          {
            isDetailView
            ? <StyledBackArrow onClick={this.detailsClick}>
                <ArrowIcon />
              </StyledBackArrow>
            : null
          }
          <StyledTitle
            type={type}
            contentShown={isDetailView}
          >
            {title}
          </StyledTitle>
        </StyledLeft>
        <StyledRight>
          {
            toggle ?
            <StyledToggleWrapper contentShown={isDetailView}>
              <Toggle
                size={'small'}
                onToggle={this.onToggle}
                checked={checked}
              />
            </StyledToggleWrapper>
            : null
          }
        </StyledRight>
      </StyledToggleHeader>
    )
  }

  getBoxContent = (checked?: boolean) => {
    if (!checked || this.state.contentShown) {
      return null
    }

    return (
      <StyledContent contentShown={this.state.contentShown}>
        <StyleDetailsLink onClick={this.detailsClick}>
          {getLocale('viewDetails')}
          <StyledArrow>
            <CaratRightIcon/>
          </StyledArrow>
        </StyleDetailsLink>
      </StyledContent>
    )
  }

  getSettingsListTitle = () => {
    return (
      <StyledSettingsListTitle onClick={this.settingsClick}>
        <StyledSettingsIcon>
          <SettingsIcon />
        </StyledSettingsIcon>
        <StyledSettingsText>
          {getLocale('settings')}
        </StyledSettingsText>
      </StyledSettingsListTitle>
    )
  }

  getSettingsContent = (title: string, children: React.ReactNode, checked?: boolean) => {
    if (!checked || !this.state.settingsOpened) {
      return null
    }

    return (
      <StyledFullSizeWrapper>
        <StyledSettingsHeader>
          <StyledSettingsTitle>
            {this.getSettingsTitle(title)}
          </StyledSettingsTitle>
          <StyledSettingsClose onClick={this.settingsClick}>
            <CloseStrokeIcon />
          </StyledSettingsClose>
          <StyledSettingsContent>
            {children}
          </StyledSettingsContent>
        </StyledSettingsHeader>
      </StyledFullSizeWrapper>
    )
  }

  getDetailContent = (children: React.ReactNode, checked?: boolean) => {
    if (!checked || !this.state.contentShown) {
      return null
    }

    return (
      <StyledFullSizeWrapper>
        {this.getToggleHeader(this.props)}
        <StyledDetailContent>
          <StyledDetailInfo>
            <StyledDescription contentShown={this.state.contentShown}>
              {this.props.description}
            </StyledDescription>
          </StyledDetailInfo>
          <StyledChildContent>
            <List title={this.getSettingsListTitle()} />
            {children}
          </StyledChildContent>
        </StyledDetailContent>
      </StyledFullSizeWrapper>
    )
  }

  render () {
    const {
      id,
      title,
      children,
      description,
      checked,
      settingsChild
    } = this.props

    return (
      <StyledCard
        testId={id}
        checked={checked}
      >
        <StyledFlip>
          <StyledContentWrapper open={!this.state.settingsOpened}>
            {this.getToggleHeader(this.props)}
            <StyledBreak />
            <StyledLeft>
              <StyledDescription>
                {description}
              </StyledDescription>
            </StyledLeft>
            {this.getBoxContent(checked)}
          </StyledContentWrapper>
        </StyledFlip>
        {this.getDetailContent(children, checked)}
        {this.getSettingsContent(title, settingsChild, checked)}
      </StyledCard>
    )
  }
}

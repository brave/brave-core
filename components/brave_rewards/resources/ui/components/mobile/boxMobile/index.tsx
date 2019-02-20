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
  ArrowLeftIcon,
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
  detailView: boolean
  settings: boolean
}

export default class BoxMobile extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      detailView: false,
      settings: false
    }
  }

  componentDidMount () {
    this.handleUrl()
    window.addEventListener('popstate', (e) => {
      this.handleUrl()
    })
  }

  handleUrl = () => {
    const path = window.location.pathname
    // Index view, no need to do anything here
    if (path === '/') {
      this.setState({
        detailView: false,
        settings: false
      })
      return
    }

    const { type } = this.props
    const typeString = `/${type}`
    const settingsString = `/${type}-settings`

    if (path === typeString) {
      this.setView('detailView', false)
    } else if (path === settingsString) {
      this.setView('settings', false)
    }
  }

  setView = (view: string, updateHistory: boolean = true) => {
    const isSettingsView = view === 'settings'
      ? !this.state.settings
      : this.state.settings
    const isDetailView = view === 'detailView'
      ? !this.state.detailView
      : this.state.detailView

    this.setState({
      detailView: isDetailView,
      settings: isSettingsView
    })

    if (updateHistory) {
      let newPath = ''
      const { type } = this.props

      if (!isSettingsView && !isDetailView) {
        newPath = '/'
      } else if (isSettingsView) {
        newPath = `/${type}-settings`
      } else if (isDetailView) {
        newPath = `/${type}`
      }

      window.history.pushState(null, '', newPath)
    }
  }

  onToggle = () => {
    if (this.props.checked) {
      this.setState({
        detailView: false,
        settings: false
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
    const isDetailView = checked && this.state.detailView

    return (
      <StyledToggleHeader detailView={isDetailView}>
        <StyledLeft>
          {
            isDetailView
            ? <StyledBackArrow onClick={this.setView.bind(this, 'detailView') as any}>
                <ArrowLeftIcon />
              </StyledBackArrow>
            : null
          }
          <StyledTitle
            type={type}
            detailView={isDetailView}
          >
            {title}
          </StyledTitle>
        </StyledLeft>
        <StyledRight>
          {
            toggle ?
            <StyledToggleWrapper detailView={isDetailView}>
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

  getBoxContent = () => {
    const { checked } = this.props

    if (!checked || this.state.detailView) {
      return null
    }

    return (
      <StyledContent detailView={this.state.detailView}>
        <StyleDetailsLink onClick={this.setView.bind(this, 'detailView') as any}>
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
      <StyledSettingsListTitle onClick={this.setView.bind(this, 'settings') as any}>
        <StyledSettingsIcon>
          <SettingsIcon />
        </StyledSettingsIcon>
        <StyledSettingsText>
          {getLocale('settings')}
        </StyledSettingsText>
      </StyledSettingsListTitle>
    )
  }

  getSettingsContent = (show?: boolean) => {
    const { title, settingsChild } = this.props

    if (!show || !settingsChild) {
      return null
    }

    return (
      <StyledFullSizeWrapper>
        <StyledSettingsHeader>
          <StyledSettingsTitle>
            {this.getSettingsTitle(title)}
          </StyledSettingsTitle>
          <StyledSettingsClose onClick={this.setView.bind(this, 'settings') as any}>
            <CloseStrokeIcon />
          </StyledSettingsClose>
          <StyledSettingsContent>
            {settingsChild}
          </StyledSettingsContent>
        </StyledSettingsHeader>
      </StyledFullSizeWrapper>
    )
  }

  getDetailContent = (show?: boolean) => {
    const { children, settingsChild } = this.props

    if (!show) {
      return null
    }

    return (
      <StyledFullSizeWrapper>
        {this.getToggleHeader(this.props)}
        <StyledDetailContent>
          <StyledDetailInfo>
            <StyledDescription detailView={this.state.detailView}>
              {this.props.description}
            </StyledDescription>
          </StyledDetailInfo>
          <StyledChildContent>
            {
              settingsChild
              ? <List title={this.getSettingsListTitle()} />
              : null
            }
            {children}
          </StyledChildContent>
        </StyledDetailContent>
      </StyledFullSizeWrapper>
    )
  }

  render () {
    const {
      id,
      description,
      checked
    } = this.props

    const showDetailView = checked && this.state.detailView
    const showSettingsView = checked && this.state.settings

    return (
      <StyledCard
        testId={id}
        checked={checked}
      >
        <StyledFlip>
          <StyledContentWrapper open={!this.state.settings}>
            {this.getToggleHeader(this.props)}
            <StyledBreak />
            <StyledLeft>
              <StyledDescription>
                {description}
              </StyledDescription>
            </StyledLeft>
            {this.getBoxContent()}
          </StyledContentWrapper>
        </StyledFlip>
        {this.getDetailContent(showDetailView)}
        {this.getSettingsContent(showSettingsView)}
      </StyledCard>
    )
  }
}

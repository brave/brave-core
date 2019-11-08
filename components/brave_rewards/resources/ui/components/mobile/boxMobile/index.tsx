/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  ArrowLeftIcon,
  CaratRightIcon,
  CloseStrokeIcon,
  SettingsIcon
} from 'brave-ui/components/icons'
import { List } from '../../'
import { getLocale } from 'brave-ui/helpers'
import Toggle from 'brave-ui/components/formControls/toggle/index'
import * as Styled from './style'

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
  alertContent?: React.ReactNode
  extraDescriptionChild?: React.ReactNode
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

  /*
   * Sets state given the view that should be shown.
   *
   * When user is on the index page: (brave://rewards)
   * (state) { detailView: false, settings: false }
   *
   * When user is on a detail view: (brave://rewards/ads)
   * (state) { detailView: true, settings: false }
   *
   * When user is on a settings view: (brave://rewards/ads-settings)
   * (state) { detailView: true, settings: true }
   */
  setView = (view: string, updateHistory: boolean = true) => {
    let isDetailView
    let isSettingsView

    switch (view) {
      case 'detailView':
        isDetailView = true
        isSettingsView = false
        break
      case 'settings':
        isDetailView = true
        isSettingsView = true
        break
      default:
        isDetailView = false
        isSettingsView = false
        break
    }

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
      <Styled.ToggleHeader detailView={isDetailView}>
        <Styled.Left>
          {
            isDetailView
              ? <Styled.BackArrow onClick={this.setView.bind(this, 'index')}>
                <ArrowLeftIcon />
              </Styled.BackArrow>
              : null
          }
          <Styled.Title
            type={type}
            detailView={isDetailView}
          >
            {title}
          </Styled.Title>
        </Styled.Left>
        <Styled.Right>
          {
            toggle ?
              <Styled.ToggleWrapper detailView={isDetailView}>
                <Toggle
                  size={'small'}
                  onToggle={this.onToggle}
                  checked={checked}
                />
              </Styled.ToggleWrapper>
              : null
          }
        </Styled.Right>
      </Styled.ToggleHeader>
    )
  }

  getBoxContent = () => {
    const { checked } = this.props

    if (!checked || this.state.detailView) {
      return null
    }

    return (
      <Styled.Content detailView={this.state.detailView}>
        <Styled.DetailsLink onClick={this.setView.bind(this, 'detailView')}>
          {getLocale('viewDetails')}
          <Styled.Arrow>
            <CaratRightIcon />
          </Styled.Arrow>
        </Styled.DetailsLink>
      </Styled.Content>
    )
  }

  getSettingsListTitle = () => {
    return (
      <Styled.MobileListWrapper>
        <Styled.SettingsListTitle onClick={this.setView.bind(this, 'settings')}>
          <Styled.SettingsText>
            {getLocale('settings')}
          </Styled.SettingsText>
          <Styled.SettingsIcon>
            <SettingsIcon />
          </Styled.SettingsIcon>
        </Styled.SettingsListTitle>
      </Styled.MobileListWrapper>
    )
  }

  getSettingsContent = (show?: boolean) => {
    const { title, settingsChild } = this.props

    if (!show || !settingsChild) {
      return null
    }

    return (
      <Styled.FullSizeWrapper>
        <Styled.SettingsClose onClick={this.setView.bind(this, 'detailView')}>
          <CloseStrokeIcon />
        </Styled.SettingsClose>
        <Styled.SettingsHeader>
          <Styled.SettingsTitle>
            {this.getSettingsTitle(title)}
          </Styled.SettingsTitle>
          <Styled.SettingsClose onClick={this.setView.bind(this, 'detailView')}>
            <CloseStrokeIcon />
          </Styled.SettingsClose>
        </Styled.SettingsHeader>
        <Styled.SettingsContent>
          {settingsChild}
        </Styled.SettingsContent>
      </Styled.FullSizeWrapper>
    )
  }

  getDetailContent = (show?: boolean) => {
    const { children, settingsChild } = this.props

    if (!show) {
      return null
    }

    return (
      <Styled.FullSizeWrapper>
        {this.getToggleHeader(this.props)}
        <Styled.DetailContent>
          <Styled.DetailInfo>
            <Styled.Description detailView={this.state.detailView}>
              {this.props.description}
            </Styled.Description>
          </Styled.DetailInfo>
          <Styled.ChildContent>
            {
              settingsChild
                ? <List title={this.getSettingsListTitle()} />
                : null
            }
            {children}
          </Styled.ChildContent>
        </Styled.DetailContent>
      </Styled.FullSizeWrapper>
    )
  }

  renderDescription (isInitialView: boolean) {
    const {
      description,
      extraDescriptionChild
    } = this.props

    const descriptionExtra = isInitialView && extraDescriptionChild
                           ? extraDescriptionChild
                           : null

    return (
      <>
        <Styled.Description>
          {description}
        </Styled.Description>
        {descriptionExtra}
      </>
    )
  }

  render () {
    const {
      id,
      checked
    } = this.props

    const showDetailView = checked && this.state.detailView
    const showSettingsView = checked && this.state.settings

    return (
      <Styled.BoxCard
        testId={id}
      >
        <Styled.Flip>
          <Styled.ContentWrapper open={!this.state.settings}>
            {this.getToggleHeader(this.props)}
            <Styled.Break />
            <Styled.DetailInfo>
              {this.renderDescription(!showDetailView && !showSettingsView)}
            </Styled.DetailInfo>
            {this.getBoxContent()}
          </Styled.ContentWrapper>
        </Styled.Flip>
        {this.getDetailContent(showDetailView)}
        {this.getSettingsContent(showSettingsView)}
        {this.renderAlert()}
      </Styled.BoxCard>
    )
  }

  renderAlert () {
    const {
      alertContent
    } = this.props

    if (!alertContent) {
      return null
    }
    return (
      <Styled.Alert>
        <Styled.AlertIcon />
        <Styled.AlertContent>
          {alertContent}
        </Styled.AlertContent>
      </Styled.Alert>
    )
  }
}

// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../../common/locale'

import {
  CloseButton,
  StyledDialogWrapper,
  StyledDialog,
  DialogTitle,
  StyledInputLabel,
  StyledInput,
  StyledButtonsContainer,
  StyledButton
} from './style'
import CloseIcon from './assets/close-icon'

interface State {
  title: string
  url: string
}

interface Props {
  targetTopSiteForEditing?: NewTab.Site
  textDirection: string
  onClose: () => void
  onSave: (title: string, url: string, newUrl: string) => void
}

export default class EditTopSite extends React.PureComponent<Props, State> {
  editTopSiteDialogRef: React.RefObject<any>
  constructor (props: Props) {
    super(props)
    this.editTopSiteDialogRef = React.createRef()

    this.state = {
      title: (props.targetTopSiteForEditing && props.targetTopSiteForEditing.title) ? props.targetTopSiteForEditing.title : '',
      url: (props.targetTopSiteForEditing && props.targetTopSiteForEditing.url) ? props.targetTopSiteForEditing.url : ''
    }
  }

  componentDidMount () {
    document.addEventListener('mousedown', this.handleClickOutside)
    document.addEventListener('keydown', this.onKeyPressSettings)
  }

  componentWillUnmount () {
    document.removeEventListener('mousedown', this.handleClickOutside)
    document.removeEventListener('keydown', this.onKeyPressSettings)
  }

  onKeyPressSettings = (event: KeyboardEvent) => {
    if (event.key === 'Escape') {
      this.props.onClose()
    } else if (event.key === 'Enter') {
      if (this.state.url) {
        this.saveNewTopSite()
      }
    }
  }

  handleClickOutside = (event: Event) => {
    if (
      this.editTopSiteDialogRef &&
      this.editTopSiteDialogRef.current &&
      !this.editTopSiteDialogRef.current.contains(event.target)
    ) {
      this.props.onClose()
    }
  }

  saveNewTopSite = () => {
    this.props.onSave(this.state.title,
                      this.props.targetTopSiteForEditing ? this.props.targetTopSiteForEditing.url
                                                         : '',
                      this.state.url)
  }

  onClickSave = () => {
    if (this.state.url) {
      this.saveNewTopSite()
    } else {
      // Just close if user don't type anything to url.
      this.props.onClose()
    }
  }

  onTitleInputChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ title: e.target.value })
  }

  onURLInputChanged = (e: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ url: e.target.value })
  }

  render () {
    const {
      targetTopSiteForEditing,
      textDirection,
      onClose
    } = this.props

    return (
      <StyledDialogWrapper textDirection={textDirection}>
        <StyledDialog
          ref={this.editTopSiteDialogRef}
          textDirection={textDirection}
        >
          <DialogTitle>
            {targetTopSiteForEditing ? getLocale('editTopSiteDialogTitle')
                                     : getLocale('addTopSiteDialogTitle')}
          </DialogTitle>
          <CloseButton onClick={onClose}>
            <CloseIcon/>
          </CloseButton>
          <StyledInputLabel>
            {getLocale('addTopSiteDialogNameLabel')}
          </StyledInputLabel>
          <StyledInput
            autoFocus={true}
            type='text'
            value={this.state.title}
            onChange={this.onTitleInputChanged}
            placeholder={getLocale('addTopSiteDialogNameInputPlaceHolder')}
          />
          <StyledInputLabel>
            {getLocale('addTopSiteDialogURLLabel')}
          </StyledInputLabel>
          <StyledInput
            type='url'
            value={this.state.url}
            onChange={this.onURLInputChanged}
            placeholder={getLocale('addTopSiteDialogURLInputPlaceHolder')}
          />
          <StyledButtonsContainer>
            <StyledButton
              text={getLocale('addTopSiteDialogCancelButtonLabel')}
              level={'secondary'}
              size={'small'}
              onClick={onClose}
            />
            <StyledButton
              text={getLocale('addTopSiteDialogSaveButtonLabel')}
              level={'primary'}
              type={'accent'}
              size={'small'}
              onClick={this.onClickSave}
            />
          </StyledButtonsContainer>
        </StyledDialog>
      </StyledDialogWrapper>
    )
  }
}

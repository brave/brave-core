/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StyledWrapper, StyledHeader, StyledTitle, StyledClose, StyledText, StyledGrantIcon, StyledPanelText, StyledHint } from './style'
import { CloseCircleOIcon } from '../../../components/icons'
import { getLocale } from '../../../helpers'

import header from './assets/header'
import giftIconUrl from './assets/gift.svg'

export interface Props {
  id?: string
  testId?: string
  isPanel?: boolean
  onClose: () => void
  title: string
  fullScreen?: boolean
  hint?: string
  text: React.ReactNode
  children: React.ReactNode
}

export default class GrantWrapper extends React.PureComponent<Props, {}> {
  render () {
    const { id, testId, isPanel, fullScreen, hint, onClose, title, text, children } = this.props

    return (
      <StyledWrapper
        id={id}
        data-test-id={testId}
        isPanel={isPanel}
        fullScreen={fullScreen}
      >
        <StyledClose onClick={onClose}>
          <CloseCircleOIcon />
        </StyledClose>
        {
          !isPanel
            ? <StyledHeader>
              {header}
            </StyledHeader>
            : <StyledGrantIcon src={giftIconUrl} />
        }
        <StyledTitle isPanel={isPanel}>
          {title}
        </StyledTitle>
        {
          !isPanel || !hint
            ? <StyledText>
              {text}
            </StyledText>
            : null
        }
        {
          isPanel && hint
            ? <StyledPanelText>
              {getLocale('captchaDrag')} <StyledHint>{hint}</StyledHint> {getLocale('captchaTarget')}
            </StyledPanelText>
            : null
        }
        {children}
      </StyledWrapper>
    )
  }
}

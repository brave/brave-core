/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledLoader,
  StyledError,
  StyledLink,
  StyledButton
} from './style'
import Modal from 'brave-ui/components/popupModals/modal/index'
import { LoaderIcon } from 'brave-ui/components/icons'
import { Button } from 'brave-ui/components'
import { getLocale, splitStringForTag } from '../../../../../common/locale'

export interface Props {
  id?: string
  errorText?: string
  errorTextLink?: string
  buttonText?: string
  titleText?: string
  learnMore?: string
  walletType?: string
  displayCloseButton?: boolean
  isMobile?: boolean
  onClick?: () => void
  onClose?: () => void
}

export default class ModalRedirect extends React.PureComponent<Props, {}> {
  getButton = () => {
    const { onClick, buttonText } = this.props
    if (!onClick || !buttonText) {
      return null
    }

    return (
      <StyledButton>
        <Button onClick={onClick} text={buttonText} type={'accent'} />
      </StyledButton>
    )
  }

  render () {
    const {
      id,
      errorText,
      errorTextLink,
      titleText,
      learnMore,
      displayCloseButton,
      isMobile,
      onClose
    } = this.props
    let errorTextLinkTags = null
    if (errorText && errorText.includes('$2')) {
      errorTextLinkTags = splitStringForTag(errorText, 2)
    }

    return (
      <Modal
        id={id}
        displayCloseButton={!!displayCloseButton}
        isMobile={isMobile}
        onClose={onClose}
      >
        <StyledWrapper>
          <StyledTitle>
            {titleText}
          </StyledTitle>
          {
            errorText
            ? <StyledError>
              <p>
              {
                errorTextLinkTags
                ? <>
                    {errorTextLinkTags.beforeTag}
                      {
                        errorTextLink
                        ? <a href={errorTextLink} target='_blank' rel='noopener noreferrer'>
                            {errorTextLinkTags.duringTag}
                          </a>
                        : errorTextLinkTags.duringTag
                      }
                    {errorTextLinkTags.afterTag}
                  </>
                : errorText
              }
              </p>
              {
                learnMore &&
                  <StyledLink href={learnMore} target='_blank' rel='noopener noreferrer'>
                    {getLocale('learnMore')}
                  </StyledLink>
              }
              {this.getButton()}
            </StyledError>
            : <StyledLoader>
              <LoaderIcon />
            </StyledLoader>
          }
        </StyledWrapper>
      </Modal>
    )
  }
}

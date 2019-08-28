/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledTitle,
  StyledLoader,
  StyledError,
  StyledButton
} from './style'
import Modal from '../../../components/popupModals/modal/index'
import { LoaderIcon } from '../../../components/icons'
import { Button } from '../../../components'

export interface Props {
  id?: string
  errorText?: {
    __html: string;
  }
  buttonText?: string
  titleText?: string
  onClick?: () => void
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
    const { id, errorText, titleText } = this.props

    return (
      <Modal id={id} displayCloseButton={false}>
        <StyledWrapper>
          <StyledTitle>
            {titleText}
          </StyledTitle>
          {
            errorText
            ? <StyledError>
              <p dangerouslySetInnerHTML={errorText} />
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

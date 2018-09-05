/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import {
  StyledWrapper,
  StyledHeaderText,
  StyledOverlayTop,
  StyledOverlayContent,
  StyledMessage,
  StyledIcon,
  StyledIconWrapper,
  StyledProviderImage,
  StyledFailWrapper,
  StyledCloseIcon,
  StyledFailTitle,
  StyledFailMsg,
  StyledOuterWrapper,
  StyledBackgroundCurve,
  StyledImageBorder
} from './style'
import { getLocale } from '../../../helpers'
import { CloseCircleIcon, PaperAirplaneIcon } from '../../../components/icons'

export interface Props {
  id?: string
  success?: boolean
  siteImg?: string
  onClose: () => void
}

export default class DonationOverlay extends React.PureComponent<Props, {}> {

  getFailureContent () {
    return (
      <StyledFailWrapper>
        <StyledCloseIcon>
          <CloseCircleIcon onClick={this.props.onClose}/>
        </StyledCloseIcon>
        <StyledFailTitle>
          {getLocale('uhOh')}
        </StyledFailTitle>
        <StyledFailMsg>
          {getLocale('donationFailureMsg')}
        </StyledFailMsg>
      </StyledFailWrapper>
    )
  }

  getOverlayContent (success: boolean | undefined, siteImg: string | undefined) {
    return (
      <StyledOverlayContent>
        <StyledOverlayTop>
          <StyledIconWrapper success={success}>
          {
            success
            ? <StyledIcon>
                <PaperAirplaneIcon/>
              </StyledIcon>
            : <StyledProviderImage src={siteImg}>
                <StyledImageBorder/>
              </StyledProviderImage>
          }
          </StyledIconWrapper>
          <StyledMessage success={success}>
            <StyledHeaderText>
              {
                success
                ? getLocale('donationSent')
                : getLocale('thankYou')
              }
            </StyledHeaderText>
          </StyledMessage>
        </StyledOverlayTop>
        {
          success
          ? null
          : this.getFailureContent()
        }
      </StyledOverlayContent>
    )
  }

  render () {
    const { id, success, siteImg } = this.props

    return (
      <StyledOuterWrapper>
        {
          success
          ? <StyledBackgroundCurve/>
          : null
        }
        <StyledWrapper id={id}>
          {this.getOverlayContent(success, siteImg)}
        </StyledWrapper>
      </StyledOuterWrapper>
    )
  }
}

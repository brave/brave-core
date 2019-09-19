/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

import * as React from 'react'
import * as CSS from 'csstype'
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
  StyledImageBorder,
  StyleSubHeaderText,
  StyledLetter,
  StyledClose,
  StyledLogoWrapper,
  StyledLogoBorder,
  StyledLogoIcon,
  StyledLogoImage,
  StyledDomainText,
  StyledDateText,
  StyledDate,
  StyledMonthlyInfo,
  StyledButtonWrapper,
  StyledTwitterShareButton
} from './style'
import { getLocale } from 'brave-ui/helpers'
import { CloseCircleOIcon, LogoTwitterIcon, PaperAirplaneIcon } from 'brave-ui/components/icons'

export interface Props {
  id?: string
  send?: boolean
  success?: boolean
  siteImg?: string
  domain?: string
  logoBgColor?: CSS.Color
  logo?: string
  amount?: string
  monthlyDate?: string
  onTweet?: () => void
  onClose: () => void
}

export default class DonationOverlay extends React.PureComponent<Props, {}> {

  getFailureContent () {
    return (
      <StyledFailWrapper>
        <StyledCloseIcon>
          <CloseCircleOIcon onClick={this.props.onClose} />
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

  getOverlayContent = () => {
    const {
      success,
      send,
      siteImg,
      logo,
      domain,
      logoBgColor,
      amount,
      monthlyDate,
      onTweet
    } = this.props
    return (
      <StyledOverlayContent>
        {
          success || send
            ? <StyledOverlayTop>
              <StyledIconWrapper success={success}>
                {
                  send
                    ? <StyledIcon>
                      <PaperAirplaneIcon />
                    </StyledIcon>
                    : null
                }
                {
                  !send && siteImg
                    ? <StyledProviderImage src={siteImg}>
                      <StyledImageBorder />
                    </StyledProviderImage>
                    : null
                }
                {
                  !send && !siteImg && !logo && domain
                    ? <StyledLetter logoBgColor={logoBgColor}>
                      {(domain && domain.substring(0, 1)) || ''}
                    </StyledLetter>
                    : null
                }
                {
                  !send && !siteImg && logo
                    ? <StyledLogoWrapper>
                      <StyledLogoBorder bg={logoBgColor}>
                        <StyledLogoImage bg={logo} />
                      </StyledLogoBorder>
                    </StyledLogoWrapper>
                    : null
                }
              </StyledIconWrapper>
              <StyledMessage success={success} monthly={amount}>
                <StyledHeaderText>
                  {
                    send
                      ? getLocale('donationSent')
                      : null
                  }
                  {
                    success
                      ? <>
                        {getLocale('thankYou')}
                        <StyledMonthlyInfo>
                          <StyleSubHeaderText>
                            {
                              monthlyDate
                                ? getLocale('autoTipText')
                                : getLocale('tipText')
                            }
                          </StyleSubHeaderText>
                          <StyledDomainText>
                            {domain}<br />{amount} {getLocale('bat')}
                            {
                              monthlyDate
                                ? `, ${getLocale('monthlyText')}`
                                : null
                            }
                          </StyledDomainText>
                          {
                            monthlyDate
                              ? <>
                                <StyledDateText>
                                  {getLocale('firstTipDateText')}
                                </StyledDateText>
                                <StyledDate>
                                  {monthlyDate}
                                </StyledDate>
                              </>
                              : null
                          }
                        </StyledMonthlyInfo>
                        {
                          onTweet
                          ? <>
                            <StyleSubHeaderText>
                              {getLocale('tellOthers')}
                            </StyleSubHeaderText>
                            <StyledButtonWrapper>
                              <StyledTwitterShareButton onClick={onTweet}>
                                {getLocale('tweetNow')}
                                <StyledLogoIcon>
                                  <LogoTwitterIcon />
                                </StyledLogoIcon>
                              </StyledTwitterShareButton>
                            </StyledButtonWrapper>
                          </>
                          : null
                        }
                      </>
                      : null
                  }
                </StyledHeaderText>
              </StyledMessage>
            </StyledOverlayTop>
            : this.getFailureContent()
        }
      </StyledOverlayContent>
    )
  }

  render () {
    const { id, send, onClose } = this.props

    return (
      <StyledOuterWrapper>
        {
          send
            ? <StyledBackgroundCurve />
            : null
        }
        <StyledWrapper id={id}>
          <StyledClose onClick={onClose}>
            <CloseCircleOIcon />
          </StyledClose>
          {this.getOverlayContent()}
        </StyledWrapper>
      </StyledOuterWrapper>
    )
  }
}

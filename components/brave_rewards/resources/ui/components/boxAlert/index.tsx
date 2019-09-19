/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

// Utils
import * as React from 'react'
import { getLocale } from 'brave-ui/helpers'

// Components
import {
  StyledWrapper,
  StyledAlertIcon,
  StyledInfo,
  StyledMessage,
  StyledMonthlyTips,
  StyledReviewWrapper,
  StyledReviewList,
  StyledModalContent,
  StyledModalInfo,
  StyledListMessage,
  StyledList,
  StyledListItem,
  StyledTipsIcon,
  StyledButton,
  StyledButtonContainer
} from './style'
import { Modal } from 'brave-ui/components'
import { AlertCircleIcon, RewardsSendTipsIcon } from 'brave-ui/components/icons'
import Button from 'brave-ui/components/buttonsIndicators/button'

export type Type = 'tips' | 'ads'

export interface Props {
  type: Type
  testId?: string
  onReview?: () => void
}

interface State {
  modalShown: boolean
}

export default class BoxAlert extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalShown: false
    }
  }

  toggleModalDisplay = () => {
    this.setState({
      modalShown: !this.state.modalShown
    })

    if (this.state.modalShown && this.props.onReview) {
      this.props.onReview()
    }
  }

  pinnedSitesModal = () => {
    return (
      <Modal
        size={'small'}
        outsideClose={false}
        onClose={this.toggleModalDisplay}
      >
        <StyledModalContent>
          <StyledTipsIcon>
            <RewardsSendTipsIcon />
          </StyledTipsIcon>
          <StyledModalInfo>
            <StyledMessage modal={true}>
              {getLocale('pinnedSitesHeader')}
            </StyledMessage>
            <StyledMonthlyTips modal={true}>
              {getLocale('monthlyTips')}
            </StyledMonthlyTips>
            <StyledListMessage>
              {getLocale('pinnedSitesMsg')}
            </StyledListMessage>
            <StyledList>
              <StyledListItem>
                {getLocale('pinnedSitesOne')}
              </StyledListItem>
              <StyledListItem>
                {getLocale('pinnedSitesTwo')}
              </StyledListItem>
              <StyledListItem>
                {getLocale('pinnedSitesThree')}
              </StyledListItem>
              <StyledListItem>
                {getLocale('pinnedSitesFour')}
              </StyledListItem>
            </StyledList>
          </StyledModalInfo>
          <StyledButtonContainer>
            <StyledButton>
              <Button
                text={getLocale('ok')}
                size={'call-to-action'}
                type={'accent'}
                onClick={this.toggleModalDisplay}
              />
            </StyledButton>
          </StyledButtonContainer>
        </StyledModalContent>
      </Modal>
    )
  }

  render () {
    const { testId, type } = this.props

    return (
      <StyledWrapper data-test-id={testId}>
        <StyledAlertIcon>
          <AlertCircleIcon />
        </StyledAlertIcon>
        <StyledInfo type={type}>
          {
            type === 'tips'
            ? <>
                <StyledMessage>
                  {getLocale('reviewSitesMsg')}
                </StyledMessage>
                <StyledMonthlyTips>
                  {getLocale('monthlyTips')}
                </StyledMonthlyTips>
              </>
            : <StyledMessage>
                {getLocale('adsNotSupported')}
              </StyledMessage>
          }
        </StyledInfo>
        {
          type === 'tips'
          ? <StyledReviewWrapper>
              <StyledReviewList onClick={this.toggleModalDisplay}>
                {getLocale('learnMore')}
              </StyledReviewList>
            </StyledReviewWrapper>
          : null
        }
        {
          this.state.modalShown && type === 'tips'
          ? this.pinnedSitesModal()
          : null
        }
      </StyledWrapper>
    )
  }
}

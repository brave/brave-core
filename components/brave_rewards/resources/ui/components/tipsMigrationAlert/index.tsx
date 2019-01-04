/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

// Utils
import * as React from 'react'
import { getLocale } from '../../../helpers'

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
import { Modal } from '../../../components'
import { AlertCircleIcon, RewardsSendTipsIcon } from '../../../components/icons'
import Button from '../../../components/buttonsIndicators/button'

export interface Props {
  testId?: string
  onReview: () => void
}

interface State {
  modalShown: boolean
}

export default class TipsMigrationAlert extends React.PureComponent<Props, State> {
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
    const { testId } = this.props

    return (
      <StyledWrapper data-test-id={testId}>
        <StyledAlertIcon>
          <AlertCircleIcon />
        </StyledAlertIcon>
        <StyledInfo>
          <StyledMessage>
            {getLocale('reviewSitesMsg')}
          </StyledMessage>
          <StyledMonthlyTips>
            {getLocale('monthlyTips')}
          </StyledMonthlyTips>
        </StyledInfo>
        <StyledReviewWrapper>
          <StyledReviewList onClick={this.toggleModalDisplay}>
            {getLocale('learnMore')}
          </StyledReviewList>
        </StyledReviewWrapper>
        {
          this.state.modalShown
          ? this.pinnedSitesModal()
          : null
        }
      </StyledWrapper>
    )
  }
}

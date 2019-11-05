/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import {
  GrantClaim,
  GrantComplete,
  GrantWrapper
} from '../../ui/components'
import { Type as GrantType } from '../../ui/components/grantClaim'

// Utils
import { getLocale } from '../../../../common/locale'
import * as rewardsActions from '../actions/rewards_actions'
import { convertProbiToFixed } from '../utils'

type Step = '' | 'complete'

interface State {
  grantShown: boolean
  grantStep: Step
  loading: boolean
}

interface Props extends Rewards.ComponentProps {
  grant: Rewards.Grant
}

type GrantWrapperContentProps = {
  title: string
  text: string
}

type GrantCompleteContentProps = {
  amountTitleText: string
  dateTitleText: string
}

function getGrantWrapperContentProps (grantType: GrantType): GrantWrapperContentProps {
  if (grantType === 'ads') {
    return {
      title: getLocale('grantTitleAds'),
      text: getLocale('grantSubtitleAds')
    }
  }
  return {
    title: getLocale('grantTitleUGP'),
    text: getLocale('grantSubtitleUGP')
  }
}

function getGrantCompleteContentProps (grantType: GrantType): GrantCompleteContentProps {
  if (grantType === 'ads') {
    return {
      amountTitleText: getLocale('grantAmountTitleAds'),
      dateTitleText: getLocale('grantDateTitleAds')
    }
  }
  return {
    amountTitleText: getLocale('grantAmountTitleUGP'),
    dateTitleText: getLocale('grantDateTitleUGP')
  }
}

// TODO add local when we know what we will get from the server
class Grant extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      grantShown: true,
      grantStep: '',
      loading: false
    }
  }

  get actions () {
    return this.props.actions
  }

  onClaim = (promotionId?: string) => {
    this.setState({ loading: true })
    this.actions.getGrantCaptcha(promotionId)
    this.setState({ grantStep: 'complete' })
  }

  onSuccess = () => {
    this.setState({
      grantShown: false,
      loading: false
    })
    this.actions.onDeleteGrant()
  }

  validateGrant = (tokens?: string) => {
    const { grant } = this.props
    const { safetyNetFailed } = this.props.rewardsData

    if (!tokens || !grant) {
      return false
    }

    if (safetyNetFailed) {
      this.setState({ loading: false })
      return false
    }

    return (tokens !== '0.0' && grant.expiryTime)
  }

  render () {
    const { grant } = this.props

    if (!grant) {
      return null
    }

    // Handle that ugp type string can actually be 'android' on android
    let type: GrantType = grant.type === 'ads' ? 'ads' : 'ugp'
    let promoId
    let tokens = '0.0'
    let date = ''

    if (grant.promotionId) {
      promoId = grant.promotionId
    }
    if (grant.probi) {
      tokens = convertProbiToFixed(grant.probi)
    }

    if (grant.type !== 'ads') {
      date = new Date(grant.expiryTime).toLocaleDateString()
    }

    // Guard against null grant statuses
    const validGrant = this.validateGrant(tokens)

    return (
      <React.Fragment>
        {
          (this.state.grantShown && type)
          ? <GrantClaim
            type={type}
            isMobile={true}
            onClaim={this.onClaim.bind(this, promoId)}
            loading={this.state.loading}
          />
          : null
        }
        {
          this.state.grantStep === 'complete' && validGrant
            ? <GrantWrapper
              fullScreen={true}
              onClose={this.onSuccess}
              {...getGrantWrapperContentProps(type)}
            >
              <GrantComplete
                isMobile={true}
                onClose={this.onSuccess}
                amount={tokens}
                date={date}
                {...getGrantCompleteContentProps(type)}
              />
            </GrantWrapper>
            : null
        }
      </React.Fragment>
    )
  }
}

const mapStateToProps = (state: Rewards.ApplicationState) => ({
  rewardsData: state.rewardsData
})

const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(Grant)

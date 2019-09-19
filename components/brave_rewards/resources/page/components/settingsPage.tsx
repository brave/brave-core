/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { Column, Grid } from 'brave-ui/components'
import { DisabledBox, MainToggle, SettingsPage as Page, ModalRedirect } from '../../ui/components'
import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import Grant from './grant'
import { getLocale } from '../../../../common/locale'

interface Props extends Rewards.ComponentProps {
}

interface State {
  redirectModalDisplayed: 'hide' | 'show'
}

class SettingsPage extends React.Component<Props, State> {
  private balanceTimerId: number

  constructor (props: Props) {
    super(props)
    this.state = {
      redirectModalDisplayed: 'hide'
    }
  }

  onToggle = () => {
    this.actions.onSettingSave('enabledMain', !this.props.rewardsData.enabledMain)
  }

  get actions () {
    return this.props.actions
  }

  refreshActions () {
    this.actions.getCurrentReport()
    this.actions.getTipTable()
    this.actions.getContributeList()
    this.actions.getPendingContributions()
    this.actions.getReconcileStamp()
    this.actions.getTransactionHistory()
    this.actions.getAdsData()
    this.actions.getExcludedSites()
  }

  componentDidMount () {
    if (this.props.rewardsData.firstLoad === null) {
      // First load ever
      this.actions.onSettingSave('firstLoad', true)
      this.actions.getWalletPassphrase()
    } else if (this.props.rewardsData.firstLoad) {
      // Second load ever
      this.actions.onSettingSave('firstLoad', false)
    }

    this.actions.onlyAnonWallet()
    this.actions.getWalletProperties()
    this.actions.getBalance()
    this.balanceTimerId = setInterval(() => {
      this.actions.getBalance()
    }, 60000)

    if (this.props.rewardsData.firstLoad === false) {
      this.refreshActions()
    } else {
      this.actions.getAdsData()
    }

    this.actions.checkImported()
    this.actions.getGrants()
    this.actions.getRewardsMainEnabled()
    this.actions.updateAdsRewards()
    this.actions.getExternalWallet('uphold')

    // one time check (legacy fix)
    if (!this.props.rewardsData.ui.paymentIdCheck) {
      // https://github.com/brave/brave-browser/issues/3061
      this.actions.getWalletPassphrase()
    }

    if (window.location.pathname.length > 1) {
      const pathElements = window.location.pathname.split('/')
      if (pathElements.length > 2) {
        this.actions.processRewardsPageUrl(window.location.pathname, window.location.search)
      }
    }
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (
      !prevProps.rewardsData.enabledMain &&
      this.props.rewardsData.enabledMain &&
      this.props.rewardsData.firstLoad === false
    ) {
      this.refreshActions()
    }

    if (
      !prevProps.rewardsData.enabledContribute &&
      this.props.rewardsData.enabledContribute
    ) {
      this.actions.getContributeList()
      this.actions.getReconcileStamp()
    }

    if (
      prevState.redirectModalDisplayed !== 'hide' &&
      this.props.rewardsData.ui.modalRedirect === 'hide'
    ) {
      this.setState({
        redirectModalDisplayed: 'hide'
      })
      window.history.replaceState({}, 'Rewards', '/')
    } else if (
      prevState.redirectModalDisplayed === 'hide' &&
      this.props.rewardsData.ui.modalRedirect !== 'hide'
    ) {
      this.setState({
        redirectModalDisplayed: 'show'
      })
    }
  }

  openTOS () {
    window.open('https://brave.com/terms-of-use', '_blank')
  }

  openPrivacyPolicy () {
    window.open('https://brave.com/privacy#rewards', '_blank')
  }

  getGrantClaims = () => {
    const { grants } = this.props.rewardsData

    if (!grants) {
      return null
    }

    return (
      <div style={{ width: '100%' }}>
        {grants.map((grant?: Rewards.Grant, index?: number) => {
          if (!grant || !grant.promotionId) {
            return null
          }

          return (
            <div key={`grant-${index}`}>
              <Grant grant={grant} />
            </div>
          )
        })}
      </div>
    )
  }

  componentWillUnmount () {
    clearInterval(this.balanceTimerId)
  }

  onRedirectError = () => {
    this.actions.hideRedirectModal()

    const { externalWallet } = this.props.rewardsData

    if (externalWallet && externalWallet.verifyUrl) {
      window.open(externalWallet.verifyUrl, '_self')
    }
  }

  getRedirectModal = () => {
    const { ui } = this.props.rewardsData

    if (ui.modalRedirect === 'show') {
      return (
        <ModalRedirect
          id={'redirect-modal-show'}
          titleText={getLocale('processingRequest')}
        />
      )
    }

    if (ui.modalRedirect === 'notAllowed') {
      return (
        <ModalRedirect
          id={'redirect-modal-not-allowed'}
          errorText={{ __html: getLocale('redirectModalNotAllowed') }}
          titleText={getLocale('redirectModalErrorWallet')}
          buttonText={getLocale('redirectModalClose')}
          onClick={this.actions.hideRedirectModal}
        />
      )
    }

    if (ui.modalRedirect === 'error') {
      return (
        <ModalRedirect
          id={'redirect-modal-error'}
          errorText={{ __html: getLocale('redirectModalError') }}
          buttonText={getLocale('processingRequestButton')}
          titleText={getLocale('processingRequest')}
          onClick={this.onRedirectError}
        />
      )
    }

    return null
  }

  render () {
    const { enabledMain } = this.props.rewardsData

    return (
      <Page>
        <Grid columns={3} customStyle={{ gridGap: '32px' }}>
          <Column size={2} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {
              enabledMain
              ? this.getRedirectModal()
              : null
            }
            <MainToggle
              onToggle={this.onToggle}
              enabled={enabledMain}
              testId={'enableMain'}
              onTOSClick={this.openTOS}
              onPrivacyClick={this.openPrivacyPolicy}
            />
            {
              !enabledMain
              ? <DisabledBox />
              : null
            }
            <AdsBox />
            <ContributeBox />
            <TipBox />
          </Column>
          <Column size={1} customStyle={{ justifyContent: 'center', flexWrap: 'wrap' }}>
            {
              enabledMain
              ? this.getGrantClaims()
              : null
            }
            <PageWallet />
          </Column>
        </Grid>
      </Page>
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
)(SettingsPage)

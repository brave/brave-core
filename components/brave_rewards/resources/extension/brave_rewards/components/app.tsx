/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'
import { PanelWelcome } from 'brave-ui/features/rewards'

// Components
import Panel from './panel'

// Utils
import * as rewardsPanelActions from '../actions/rewards_panel_actions'

interface Props extends RewardsExtension.ComponentProps {
}

interface State {
  windowId: number
  creating: boolean
}

export class RewardsPanel extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      windowId: -1,
      creating: false
    }
  }

  componentDidMount () {
    chrome.windows.getCurrent({}, this.onWindowCallback)
  }

  componentDidUpdate (prevProps: Props, prevState: State) {
    if (
      !prevProps.rewardsPanelData.walletCreated &&
      this.props.rewardsPanelData.walletCreated
    ) {
      this.getTabData()
    }

    if (
      this.state.creating &&
      !prevProps.rewardsPanelData.walletCreateFailed &&
      this.props.rewardsPanelData.walletCreateFailed
    ) {
      this.setState({
        creating: false
      })
    }
  }

  getTabData () {
    chrome.tabs.query({
      active: true,
      currentWindow: true
    }, (tabs) => {
      if (!tabs || !tabs.length) {
        return
      }
      this.props.actions.onTabRetrieved(tabs[0])
    })
  }

  getGrant () {
    this.props.actions.getGrant()
  }

  onWindowCallback = (window: chrome.windows.Window) => {
    this.setState({
      windowId: window.id
    })

    if (this.props.rewardsPanelData.walletCreated) {
      this.getGrant()
      this.getTabData()
    }
  }

  openRewards () {
    chrome.tabs.create({
      url: 'chrome://rewards'
    })
  }

  get actions () {
    return this.props.actions
  }

  onCreate = () => {
    this.setState({
      creating: true
    })
    this.actions.createWallet()
  }

  render () {
    const { walletCreateFailed, walletCreated } = this.props.rewardsPanelData

    return (
      <>
        {
          !walletCreated
          ? <PanelWelcome
            error={walletCreateFailed}
            creating={this.state.creating}
            variant={'two'}
            optInAction={this.onCreate}
            optInErrorAction={this.onCreate}
            moreLink={this.openRewards}
          />
          : <Panel windowId={this.state.windowId} />
        }
      </>
    )
  }
}

export const mapStateToProps = (state: RewardsExtension.ApplicationState) => ({
  rewardsPanelData: state.rewardsPanelData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(rewardsPanelActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(RewardsPanel)

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const welcomeActions = require('../actions/welcome_actions')

const BraveRewardsLink = (props) =>
  <div><a href='chrome://rewards/' target='_blank' onClick={props.onClicked}>Brave Rewards</a></div>
const ImportNowLink = (props) =>
  <div><a href='#' onClick={props.onClicked}>Import Now</a></div>
const PreferencesLink = (props) =>
  <div><a href='chrome://settings/' target='_blank' onClick={props.onClicked}>Preferences</a></div>
const GoToPage = (props) =>
  <div><a href='#' onClick={props.onClicked}>Go to Page {props.pageIndex}</a></div>

class WelcomePage extends React.Component {
  constructor (props) {
    super(props)
    this.onImportNowClicked = this.onImportNowClicked.bind(this)
    this.onGoToPage1Clicked = this.onGoToPageClicked.bind(this, 0)
    this.onGoToPage2Clicked = this.onGoToPageClicked.bind(this, 1)
  }

  onImportNowClicked () {
    this.actions.importNowRequested()
  }

  onGoToPageClicked (pageIndex) {
    this.actions.goToPageRequested(pageIndex)
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { welcomeData } = this.props
    return (
      <div>
      <BraveRewardsLink />
      <ImportNowLink onClicked={this.onImportNowClicked}/>
      <PreferencesLink />
      <GoToPage pageIndex={0} onClicked={this.onGoToPage1Clicked} />
      <GoToPage pageIndex={1} onClicked={this.onGoToPage2Clicked} />
      <div>Current page: {(welcomeData.pageIndex || 0) + 1}</div>
      </div>)
  }
}

const mapStateToProps = (state) => ({
  welcomeData: state.welcomeData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(welcomeActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(WelcomePage)

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const React = require('react')
const { bindActionCreators } = require('redux')
const { connect } = require('react-redux')
const adblockActions = require('../actions/adblock_actions')

const NumBlockedStat = (props) =>
  <div>
    <span i18n-content='adsBlocked'/>
    <span> </span>
    { props.adsBlockedStat || 0 }
  </div>

const RegionalAdBlockEnabled = (props) =>
  <div>
    <span i18n-content='regionalAdblockEnabledTitle'/>
    <span> </span>
    {
      props.regionalAdBlockEnabled
      ? <span i18n-content='regionalAdblockEnabled'/>
      : <span i18n-content='regionalAdblockDisabled'/>
    }
    <div>
    {
      props.regionalAdBlockEnabled
      ? props.regionalAdBlockTitle
      : null
    }
    </div>
  </div>

class AdblockPage extends React.Component {
  constructor (props) {
    super(props)
  }

  get actions () {
    return this.props.actions
  }

  render () {
    const { adblockData } = this.props
    return (
      <div>
        <NumBlockedStat adsBlockedStat={adblockData.stats.adsBlockedStat} />
        <RegionalAdBlockEnabled
          regionalAdBlockEnabled={adblockData.stats.regionalAdBlockEnabled}
          regionalAdBlockTitle={adblockData.stats.regionalAdBlockTitle} />
      </div>)
  }
}

const mapStateToProps = (state) => ({
  adblockData: state.adblockData
})

const mapDispatchToProps = (dispatch) => ({
  actions: bindActionCreators(adblockActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(AdblockPage)

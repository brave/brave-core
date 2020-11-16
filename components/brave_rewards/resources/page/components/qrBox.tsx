/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { ModalQRCode } from '../../ui/components'
import { Button } from 'brave-ui/components'
import { QRBoxStyle, QRText, QRBoxWrapper } from './style'

// Utils
import * as rewardsActions from '../actions/rewards_actions'
import { getLocale } from '../../../../common/locale'

interface Props extends Rewards.ComponentProps {
}

interface State {
  modalQRCode: boolean
}

class QRBox extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      modalQRCode: false
    }
  }

  toggleQRCodeModal = () => {
    const { paymentId } = this.props.rewardsData
    if (!paymentId || paymentId.length === 0) {
      this.props.actions.getPaymentId()
    }

    this.setState({
      modalQRCode: !this.state.modalQRCode
    })
  }

  render () {
    const {
      paymentId
    } = this.props.rewardsData

    return (
      <>
        <QRBoxWrapper>
          <QRBoxStyle>
            <QRText>
              {getLocale('qrBoxText')}
            </QRText>
            <Button onClick={this.toggleQRCodeModal} text={getLocale('qrBoxButton')} type={'accent'} />
          </QRBoxStyle>
        </QRBoxWrapper>
        {
          this.state.modalQRCode
          ? <ModalQRCode
              paymentId={paymentId}
              onClose={this.toggleQRCodeModal}
          />
          : null
        }
      </>
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
)(QRBox)

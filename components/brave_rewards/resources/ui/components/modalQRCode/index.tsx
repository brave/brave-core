/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  QRImage,
  ImageWrapper
} from './style'
import { Modal } from 'brave-ui/components'
import * as qr from 'qr-image'
import { getLocale } from 'brave-ui/helpers'

export interface Props {
  paymentId: string
  testId?: string
  onClose: () => void
}

interface State {
  qrCode: string
}

export default class ModalQRCode extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      qrCode: ''
    }
  }

  componentDidMount () {
    if (!this.props.paymentId) {
      return
    }

    if (this.props.paymentId.length > 0) {
      this.generateQRData()
    }
  }

  componentDidUpdate () {
    if (this.state.qrCode.length > 0) {
      return
    }

    if (this.props.paymentId && this.props.paymentId.length > 0) {
      this.generateQRData()
    }
  }

  generateQRData = () => {
    const image: any = qr.image(this.props.paymentId)
    try {
      let chunks: Uint8Array[] = []
      image
        .on('data', (chunk: Uint8Array) => chunks.push(chunk))
        .on('end', () => {
          this.setState({
            qrCode: `data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`
          })
        })
    } catch (error) {
      console.error('Could not create deposit QR', error.toString())
    }
  }

  render () {
    const {
      testId,
      onClose
    } = this.props

    return (
      <Modal onClose={onClose} size={'small'} testId={testId}>
        <ImageWrapper>
          {
            this.state.qrCode
            ? <QRImage src={this.state.qrCode} />
            : getLocale('qrCodeLoading')
          }
        </ImageWrapper>
      </Modal>
    )
  }
}

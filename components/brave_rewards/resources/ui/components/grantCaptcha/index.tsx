/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledDropArea,
  StyledDrag,
  StyledText,
  StyledImageWrap,
  StyledImage
} from './style'
import { getLocale } from 'brave-ui/helpers'

import batUrl from './assets/bat.png'

export interface Props {
  id?: string
  isPanel?: true
  isWindows?: boolean
  onSolution: (x: number, y: number) => void
  captchaImage: string
  hint: string
}

export default class GrantCaptcha extends React.PureComponent<Props, {}> {
  private readonly offset: number
  private dndStartPosition: { x: number; y: number; width: number; height: number }
  private captchaBox: HTMLDivElement | null
  private captchaDrag: HTMLDivElement | null

  constructor (props: Props) {
    super(props)
    this.captchaBox = null
    this.captchaDrag = null
    this.offset = 5
    this.dndStartPosition = {
      x: 0,
      y: 0,
      width: 0,
      height: 0
    }
  }

  onCaptchaDrop = (event: React.DragEvent) => {
    event.preventDefault()
    if (!this.captchaBox || !this.captchaDrag) {
      return
    }

    const target = this.captchaBox.getBoundingClientRect()
    const captchaDrag = this.captchaDrag.getBoundingClientRect()

    let x = event.clientX - target.left - this.dndStartPosition.x + (this.dndStartPosition.width / 2)
    let y = event.clientY - target.top - this.dndStartPosition.y + (this.dndStartPosition.height / 2) - captchaDrag.height

    if (this.props.isWindows) {
      const dpr = window.devicePixelRatio
      const factor = (dpr <= 1) ? 0 : (this.offset * dpr)
      x = Math.round(x + factor)
      y = Math.round(y + factor)
    }

    this.props.onSolution(x, y)
  }

  onCaptchaDrag = (event: any) => {
    if (!event || !event.target) {
      return
    }

    const target = event.target.getBoundingClientRect()
    this.dndStartPosition = {
      x: event.clientX - target.left,
      y: event.clientY - target.top,
      width: target.width,
      height: target.height
    }
  }

  preventDefault (event: React.DragEvent) {
    event.preventDefault()
  }

  refWrapper = (node: HTMLDivElement) => {
    this.captchaBox = node
  }

  refDrag = (node: HTMLDivElement) => {
    this.captchaDrag = node
  }

  render () {
    const { id, isPanel, captchaImage, hint } = this.props

    return (
      <StyledWrapper
        id={id}
        innerRef={this.refWrapper}
      >
        <StyledDrag innerRef={this.refDrag}>
          <StyledImageWrap>
            <StyledImage src={batUrl} onDragStart={this.onCaptchaDrag} draggable={true} />
          </StyledImageWrap>
          {
            !isPanel
            ? <StyledText>
                {getLocale('dndCaptchaText1')} <b>{hint}</b> {getLocale('dndCaptchaText2')}
              </StyledText>
            : null
          }
        </StyledDrag>
        <StyledDropArea src={captchaImage} draggable={false} onDrop={this.onCaptchaDrop} onDragOver={this.preventDefault} />
      </StyledWrapper>
    )
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { CaptchaStatus, CaptchaResult } from '../lib/interfaces'

import * as styles from './adaptive_captcha_view.style'

import checkIconSrc from '../assets/check.svg'
import smileySadIconSrc from '../assets/smiley_sad.svg'

const iframeAllow = `
  accelerometer "none";
  ambient-light-sensor "none";
  camera "none";
  display-capture "none";
  document-domain "none";
  fullscreen "none";
  geolocation "none";
  gyroscope "none";
  magnetometer "none";
  microphone "none";
  midi "none";
  payment "none";
  usb "none";
  vibrate "none";
  vr "none";
  webauthn "none"
`

interface Props {
  captchaURL: string
  captchaStatus: CaptchaStatus
  onClose: () => void
  onContactSupport: () => void
  onCaptchaResult: (result: CaptchaResult) => void
}

export function AdaptiveCaptchaView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)

  React.useEffect(() => {
    const listener = (event: MessageEvent) => {
      // Sandboxed iframes which lack the 'allow-same-origin' header have "null"
      // rather than a valid origin.
      if (event.origin !== 'null') {
        return
      }

      if (!iframeRef.current) {
        return
      }

      const { contentWindow } = iframeRef.current
      if (!event.source || event.source !== contentWindow || !event.data) {
        return
      }

      switch (event.data) {
        case 'captchaSuccess':
          props.onCaptchaResult('success')
          break
        case 'captchaFailure':
          props.onCaptchaResult('failure')
          break
        case 'error':
          props.onCaptchaResult('error')
          break
      }
    }

    window.addEventListener('message', listener)
    return () => { window.removeEventListener('message', listener) }
  }, [props.onCaptchaResult])

  function renderCaptcha () {
    return (
      <styles.frameBox>
        <iframe
          ref={iframeRef}
          allow={iframeAllow.trim()}
          src={props.captchaURL}
          sandbox='allow-scripts'
          scrolling='no'
        />
      </styles.frameBox>
    )
  }

  function renderSuccess () {
    return (
      <styles.root>
        <styles.title>
          <img src={checkIconSrc} />{getString('captchaSolvedTitle')}
        </styles.title>
        <styles.text>
          {getString('captchaSolvedText')}
        </styles.text>
        <styles.closeAction>
          <button onClick={props.onClose}>
            {getString('captchaDismiss')}
          </button>
        </styles.closeAction>
      </styles.root>
    )
  }

  function renderMaxAttemptsExceededMessage () {
    return (
      <styles.root>
        <styles.title className='long'>
          <img src={smileySadIconSrc} />
          {getString('captchaMaxAttemptsExceededTitle')}
        </styles.title>
        <styles.text>
          {getString('captchaMaxAttemptsExceededText')}
        </styles.text>
        <styles.helpAction>
          <button onClick={props.onContactSupport}>
            {getString('captchaContactSupport')}
          </button>
        </styles.helpAction>
      </styles.root>
    )
  }

  switch (props.captchaStatus) {
    case 'pending': return renderCaptcha()
    case 'success': return renderSuccess()
    case 'max-attempts-exceeded': return renderMaxAttemptsExceededMessage()
  }
}

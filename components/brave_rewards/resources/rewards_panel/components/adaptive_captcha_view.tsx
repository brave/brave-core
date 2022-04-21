/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { AdaptiveCaptchaInfo, AdaptiveCaptchaResult } from '../../rewards_panel/lib/interfaces'

import * as styles from './adaptive_captcha_view.style'

import checkIconSrc from '../assets/check.svg'
import smileySadIconSrc from '../assets/smiley_sad.svg'

const iframeAllow = `
  accelerometer 'none';
  ambient-light-sensor 'none';
  camera 'none';
  display-capture 'none';
  document-domain 'none';
  fullscreen 'none';
  geolocation 'none';
  gyroscope 'none';
  magnetometer 'none';
  microphone 'none';
  midi 'none';
  payment 'none';
  publickey-credentials-get 'none';
  usb 'none'
`

interface Props {
  adaptiveCaptchaInfo: AdaptiveCaptchaInfo
  onClose: () => void
  onCaptchaResult: (result: AdaptiveCaptchaResult) => void
}

export function AdaptiveCaptchaView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const { adaptiveCaptchaInfo } = props

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

  function onContactSupport () {
    // TODO(zenparsing): This doesn't work in WebUI. We should just make this
    // into a link instead.
    window.open('https://support.brave.com/', '_blank')
    props.onClose()
  }

  function renderCaptcha () {
    return (
      <styles.overlay>
        <styles.frameRoot>
          <iframe
            ref={iframeRef}
            allow={iframeAllow.trim().replace(/\n/g, '')}
            src={adaptiveCaptchaInfo.url}
            sandbox='allow-scripts'
            scrolling='no'
          />
        </styles.frameRoot>
      </styles.overlay>
    )
  }

  function renderSuccess () {
    return (
      <styles.overlay>
        <styles.modalRoot>
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
        </styles.modalRoot>
      </styles.overlay>
    )
  }

  function renderMaxAttemptsExceededMessage () {
    return (
      <styles.overlay>
        <styles.modalRoot>
          <styles.title className='long'>
            <img src={smileySadIconSrc} />
            {getString('captchaMaxAttemptsExceededTitle')}
          </styles.title>
          <styles.text>
            {getString('captchaMaxAttemptsExceededText')}
          </styles.text>
          <styles.helpAction>
            <button onClick={onContactSupport}>
              {getString('captchaContactSupport')}
            </button>
          </styles.helpAction>
        </styles.modalRoot>
      </styles.overlay>
    )
  }

  switch (adaptiveCaptchaInfo.status) {
    case 'pending': return renderCaptcha()
    case 'success': return renderSuccess()
    case 'max-attempts-exceeded': return renderMaxAttemptsExceededMessage()
  }
}

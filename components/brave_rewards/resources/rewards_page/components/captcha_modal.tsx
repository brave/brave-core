/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { TabOpenerContext } from '../../shared/components/new_tab_link'
import { Modal } from './modal'
import { CaptchaInfo } from '../lib/app_state'
import { useLocaleContext } from '../lib/locale_strings'
import * as urls from '../../shared/lib/rewards_urls'

import { style } from './captcha_modal.style'

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
  captchaInfo: CaptchaInfo
  onCaptchaResult: (success: boolean) => void
  onClose: () => void
}

export function CaptchaModal(props: Props) {
  const tabOpener = React.useContext(TabOpenerContext)
  const { getString } = useLocaleContext()
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const [captchaSolved, setCaptchaSolved] = React.useState(false)

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
          props.onCaptchaResult(true)
          setCaptchaSolved(true)
          break
        case 'captchaFailure':
        case 'error':
          props.onCaptchaResult(false)
          break
      }
    }

    window.addEventListener('message', listener)
    return () => { window.removeEventListener('message', listener) }
  }, [props.onCaptchaResult])

  function renderContent() {
    if (captchaSolved) {
      return (
        <div className='content solved'>
          <div className='icon'><Icon name='check-circle-filled' /></div>
          <h3>{getString('captchaSolvedTitle')}</h3>
          <p>{getString('captchaSolvedText')}</p>
          <Button kind='outline' onClick={props.onClose}>
            {getString('closeButtonLabel')}
          </Button>
        </div>
      )
    }

    if (props.captchaInfo.maxAttemptsExceeded) {
      return (
        <div className='content max-attempts-exceeded'>
          <div className='icon'><Icon name='warning-circle-filled' /></div>
          <h3>{getString('captchaMaxAttemptsExceededTitle')}</h3>
          <p>{getString('captchaMaxAttemptsExceededText')}</p>
          <Button
            kind='outline'
            onClick={() => {
              props.onClose()
              tabOpener.openTab(urls.contactSupportURL)
            }}
          >
            {getString('captchaSupportButtonLabel')}
          </Button>
        </div>
      )
    }

    return (
      <iframe
        ref={iframeRef}
        allow={iframeAllow.trim().replace(/\n/g, '')}
        src={props.captchaInfo.url}
        sandbox='allow-scripts'
      />
    )
  }

  return (
    <Modal onEscape={props.onClose}>
      <div {...style}>
        <Modal.Header onClose={props.onClose} />
        {renderContent()}
      </div>
    </Modal>
  )
}

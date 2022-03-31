/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Host, HostState } from './interfaces'
import { createStateManager } from '../../shared/lib/state_manager'

export function createHost (): Host {
  const stateManager = createStateManager<HostState>({
    loading: true,
    captchaURL: '',
    captchaStatus: 'pending'
  })

  function loadCaptcha () {
    chrome.braveRewards.getScheduledCaptchaInfo((scheduledCaptcha) => {
      stateManager.update({
        loading: false,
        captchaURL: scheduledCaptcha.url,
        captchaStatus: scheduledCaptcha.maxAttemptsExceeded
          ? 'max-attempts-exceeded'
          : 'pending'
      })
    })
  }

  loadCaptcha()

  return {
    get state () { return stateManager.getState() },

    getString (key) {
      return chrome.i18n.getMessage(key,
        ['$1', '$2', '$3', '$4', '$5', '$6', '$7', '$8', '$9'])
    },

    addListener: stateManager.addListener,

    handleCaptchaResult (result) {
      switch (result) {
        case 'success':
          chrome.braveRewards.updateScheduledCaptchaResult(true)
          stateManager.update({ captchaStatus: 'success' })
          break
        case 'failure':
        case 'error':
          chrome.braveRewards.updateScheduledCaptchaResult(false)
          loadCaptcha()
          break
      }
    }
  }
}

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { getMessage } from '../background/api/locale_api'

let timeout: any = null

const tipActionCountClass = 'SoundCloudTip-actionCount'
const tipIconContainerClass = 'IconContainer'
const actionTipClass = 'action-brave-tip'

type soundcloudEvent = {
  action: string
  id: string
} | null

const createBraveTipAction = (elem: Element, getMetaData: (elem: Element) => RewardsTip.MediaMetaData | null) => {
  const hoverClasses = ' tooltipped tooltipped-sw tooltipped-align-right-1'

  // Create the tip action
  const braveTipAction = document.createElement('div')
  braveTipAction.className = 'SoundCloudTip-action js-tooltip ' + actionTipClass + hoverClasses
  braveTipAction.style.display = 'inline-block'
  braveTipAction.style.minWidth = '40px'
  braveTipAction.setAttribute('aria-label', getMessage('soundcloudTipsHoverText'))

  // Create the tip button
  const braveTipButton = document.createElement('button')
  braveTipButton.className = 'SoundCloudTip-actionButton u-textUserColorHover js-actionButton'
  braveTipButton.style.background = 'transparent'
  braveTipButton.style.border = '0'
  braveTipButton.style.color = '#657786'
  braveTipButton.style.cursor = 'pointer'
  braveTipButton.style.display = 'inline-block'
  braveTipButton.style.fontSize = '16px'
  braveTipButton.style.lineHeight = '1'
  braveTipButton.style.outline = '0'
  braveTipButton.style.padding = '0 2px'
  braveTipButton.style.position = 'relative'
  braveTipButton.type = 'button'
  braveTipButton.onclick = function (event) {
    const soundcloudMetaData = getMetaData(elem)
    if (soundcloudMetaData) {
      const msg = { type: 'tipInlineMedia', mediaMetaData: soundcloudMetaData }
      chrome.runtime.sendMessage(msg)
    }
    event.stopPropagation()
  }

  // Create the tip icon container
  const braveTipIconContainer = document.createElement('div')
  braveTipIconContainer.className = tipIconContainerClass + ' js-tooltip'
  braveTipIconContainer.style.display = 'inline-block'
  braveTipIconContainer.style.lineHeight = '0'
  braveTipIconContainer.style.position = 'relative'
  braveTipIconContainer.style.verticalAlign = 'middle'
  braveTipButton.appendChild(braveTipIconContainer)

  // Create the tip icon
  const braveTipIcon = document.createElement('span')
  braveTipIcon.className = 'Icon Icon--medium'
  braveTipIcon.style.background = 'transparent'
  braveTipIcon.style.content = 'url(\'data:image/svg+xml;utf8,<svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px" viewBox="0 0 105 100" style="enable-background:new 0 0 105 100;" xml:space="preserve"><style type="text/css">.st1{fill:%23662D91;}.st2{fill:%239E1F63;}.st3{fill:%23FF5000;}.st4{fill:%23FFFFFF;stroke:%23FF5000;stroke-width:0.83;stroke-miterlimit:10;}</style><title>BAT_icon</title><g id="Layer_2_1_"><g id="Layer_1-2"><polygon class="st1" points="94.8,82.6 47.4,55.4 0,82.9 "/><polygon class="st2" points="47.4,0 47.1,55.4 94.8,82.6 "/><polygon class="st3" points="0,82.9 47.2,55.9 47.4,0 "/><polygon class="st4" points="47.1,33.7 28,66.5 66.7,66.5 "/></g></g></svg>\')'
  braveTipIcon.style.display = 'inline-block'
  braveTipIcon.style.fontSize = '18px'
  braveTipIcon.style.fontStyle = 'normal'
  braveTipIcon.style.height = '16px'
  braveTipIcon.style.marginTop = '5px'
  braveTipIcon.style.position = 'relative'
  braveTipIcon.style.verticalAlign = 'baseline'
  braveTipIcon.style.width = '16px'
  braveTipIconContainer.appendChild(braveTipIcon)

  // Create the tip action count (typically used to present a counter
  // associated with the action, but we'll use it to display a static
  // action label)
  const braveTipActionCount = document.createElement('span')
  braveTipActionCount.className = tipActionCountClass
  braveTipActionCount.style.color = '#657786'
  braveTipActionCount.style.display = 'inline-block'
  braveTipActionCount.style.fontSize = '12px'
  braveTipActionCount.style.fontWeight = 'bold'
  braveTipActionCount.style.lineHeight = '1'
  braveTipActionCount.style.marginLeft = '1px'
  braveTipActionCount.style.position = 'relative'
  braveTipActionCount.style.verticalAlign = 'text-bottom'
  braveTipButton.appendChild(braveTipActionCount)

  // Create the tip action count presentation
  const braveTipActionCountPresentation = document.createElement('span')
  braveTipActionCountPresentation.className = 'SoundCloudTip-actionCountForPresentation'
  braveTipActionCountPresentation.textContent = getMessage('soundcloudTipsIconLabel')
  braveTipActionCount.appendChild(braveTipActionCountPresentation)

  // Create the shadow DOM root that hosts our injected DOM elements
  const shadowRoot = braveTipAction.attachShadow({ mode: 'open' })
  shadowRoot.appendChild(braveTipButton)

  // Create style element for hover color
  const style = document.createElement('style')
  style.innerHTML = '.SoundCloudTip-actionButton :hover { color: #FB542B }'
  shadowRoot.appendChild(style)

  return braveTipAction
}

const getListenEngagementMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  return {
    mediaType: 'soundcloud',
    userUrl: document.location.pathname.slice(1).split('/')[0]
  }
}

const listenEngagementInsertFunction = (parent: Element) => {
  const groupLoc = 'soundActions'
  const groupParent = parent.getElementsByClassName(groupLoc)
  if (groupParent.length) {
    const tipElem = createBraveTipAction(parent, getListenEngagementMetaData)
    tipElem.className += ' sc-button sc-button-medium sc-button-responsive'
    if (tipElem.shadowRoot) {
      const button = tipElem.shadowRoot.querySelectorAll(
          '.SoundCloudTip-actionButton')[0] as HTMLElement
      const tipActionCount = tipElem.shadowRoot.querySelectorAll(
          '.' + tipActionCountClass)[0] as HTMLElement
      const iconContainer = tipElem.shadowRoot.querySelectorAll(
          '.' + tipIconContainerClass)[0] as HTMLElement
      (iconContainer.children[0] as HTMLElement).style.marginTop = null
      button.style.fontSize = '14px'
      button.style.color = '#333'
      tipActionCount.style.fontWeight = '100'
      tipActionCount.style.color = '#333'
      tipActionCount.style.fontSize = '14px'
    }
    const container = groupParent[0].children && groupParent[0].children[0]
    if (container) {
      const pos = container.children.length - 1
      if (container.children) {
        container.insertBefore(tipElem, container.children[pos])
      }
    }
  }
}

const getSoundBodyMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const aTag = elem.getElementsByClassName('soundTitle__username')
  if (aTag.length) {
    const aElem = aTag[0] as HTMLAnchorElement
    const userUrl = aElem.href.split('/')[3]
    return {
      mediaType: 'soundcloud',
      userUrl
    }
  }
  return null
}

const soundBodyInsertFunction = (parent: Element) => {
  const groupLoc = 'soundActions'
  const groupParent = parent.getElementsByClassName(groupLoc)
  if (groupParent.length) {
    const tipElem = createBraveTipAction(parent, getSoundBodyMetaData)
    tipElem.className += ' sc-button sc-button-small sc-button-responsive'
    if (tipElem.shadowRoot) {
      const button = tipElem.shadowRoot.querySelectorAll(
          '.SoundCloudTip-actionButton')[0] as HTMLElement
      const tipActionCount = tipElem.shadowRoot.querySelectorAll(
          '.' + tipActionCountClass)[0] as HTMLElement
      const iconContainer = tipElem.shadowRoot.querySelectorAll(
          '.' + tipIconContainerClass)[0] as HTMLElement
      (iconContainer.children[0] as HTMLElement).style.marginTop = null
      button.style.fontSize = '11px'
      button.style.color = null
      tipActionCount.style.fontWeight = null
      tipActionCount.style.color = null
      tipActionCount.style.fontSize = '11px'
    }
    const container = groupParent[0].children && groupParent[0].children[0]
    if (container) {
      const pos = container.children.length - 1
      if (container.children) {
        container.insertBefore(tipElem, container.children[pos])
      }
    }
  }
}

const getPlayerMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const aTag = elem.getElementsByClassName('playbackSoundBadge__avatar')
  if (aTag.length) {
    const aElem = aTag[0] as HTMLAnchorElement
    const userUrl = aElem.href.split('/')[3]
    return {
      mediaType: 'soundcloud',
      userUrl
    }
  }
  return null
}

const playerInsertFunction = (parent: Element) => {
  const tipLoc = 'playbackSoundBadge__actions'
  const elemLoc = parent.getElementsByClassName(tipLoc)
  if (elemLoc.length) {
    const tipElem = createBraveTipAction(parent, getPlayerMetaData)
    const children = elemLoc[0].children
    elemLoc[0].insertBefore(tipElem, children[0])
  }
}

const configureSoundCloudTipAction = (tipLocationClass: string, tippingEnabled: boolean,
      insertFunction: (parent: Element) => void) => {
  const tipLocations = document.getElementsByClassName(tipLocationClass)
  for (let i = 0; i < tipLocations.length; ++i) {
    const parent = tipLocations[i]
    if (parent) {
      const braveTipActions = parent.getElementsByClassName(actionTipClass)
      if (tippingEnabled && braveTipActions.length === 0) {
        insertFunction(parent)
      } else if (!tippingEnabled && braveTipActions.length === 1) {
        const attachedParent = braveTipActions[0].parentElement
        if (attachedParent) {
          attachedParent.removeChild(braveTipActions[0])
        }
      }
    }
  }
}

const configureSoundCloudTipActions = (tippingEnabled: boolean) => {
  let tipLocationClasses = [
    'playbackSoundBadge',
    'sound__body',
    'listenEngagement'
  ]
  let insertFunctions = [
    playerInsertFunction,
    soundBodyInsertFunction,
    listenEngagementInsertFunction
  ]

  for (let i in tipLocationClasses) {
    configureSoundCloudTipAction(tipLocationClasses[i], tippingEnabled, insertFunctions[i])
  }
}

const configureBraveTipAction = () => {
  clearTimeout(timeout)
  chrome.runtime.sendMessage('rewardsEnabled', function (rewards) {
    const msg = {
      type: 'inlineTipSetting',
      key: 'soundcloud'
    }
    chrome.runtime.sendMessage(msg, function (inlineTip) {
      let tippingEnabled = rewards.enabled && inlineTip.enabled
      configureSoundCloudTipActions(tippingEnabled)
    })
  })
  timeout = setTimeout(configureBraveTipAction, 3000)
}

// In order to deal with infinite scrolling and overlays, periodically
// check if injection needs to occur (mitigate the performance cost
// by only running this when the foreground tab is active or visible)
document.addEventListener('visibilitychange', function () {
  clearTimeout(timeout)
  if (!document.hidden) {
    timeout = setTimeout(configureBraveTipAction, 3000)
  }
})

const getEventData = async (event: any, clientId: string): Promise<any | null>  => {
  if(event.event == 'audio') {
    const id = event.payload.track_owner.split(':').slice(-1)[0]
    const url = `https://api.soundcloud.com/users/${id}?client_id=${clientId}`
    const response = await fetch(url)
    console.log(`Get to :${url}`)
    console.log(`Response code: ${response.status}`)
    const eventUserData =  await response.json()
    event.payload.owner_data = eventUserData
    return event
  }
  return null
}

const getSoundCloudClientId = (): Promise<string> => {
  const msg = { type: 'getSoundCloudClientId' }
  return new Promise(resolve => chrome.runtime.sendMessage(msg, resolve))
}

const getSoundCloudEventsData = async (payload: string) => {
  let body = JSON.parse(payload)
  let events = body.events
  const clientId = await getSoundCloudClientId()

  if(!Array.isArray(events)) {
    return
  }

  var info: Array<soundcloudEvent> = []
  for (let e of events) {
    let event = await getEventData(e, clientId)
    if (event) {
      info.push(event)
    }
  }
  if (info.length) {
    const msg = {
      type: 'respondClientMediaMessage',
      body: {
        type: 'soundcloud',
        response: JSON.stringify({
          action: 'autoContributionEvent',
          message: info
        })
      }
    }
    chrome.runtime.sendMessage(msg)
  }
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const type = msg.type
  switch (type) {
    case 'soundcloud' : {
      let request = msg.body
      switch (request.action) {
        case 'getEventData' :
          getSoundCloudEventsData(request.payload)
          break
        default:
          break
      }
    }
    default:
      break
  }
})

configureBraveTipAction()

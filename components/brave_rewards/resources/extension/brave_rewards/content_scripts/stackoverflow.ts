/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { getMessage } from '../background/api/locale_api'

let timeout: any = null

const tipActionCountClass = 'StackOverflowTip-actionCount'
const tipIconContainerClass = 'IconContainer'
const actionTipClass = 'action-brave-tip'

const createBraveTipAction = (elem: Element, getMetaData: (elem: Element) => RewardsTip.MediaMetaData | null) => {
  const hoverClasses = ' tooltipped tooltipped-sw tooltipped-align-right-1'

  // Create the tip action
  const braveTipAction = document.createElement('div')
  braveTipAction.className = 'StackOverflowTip-action js-tooltip ' + actionTipClass + hoverClasses
  braveTipAction.style.display = 'inline-block'
  braveTipAction.style.minWidth = '40px'
  braveTipAction.setAttribute('aria-label', getMessage('githubTipsHoverText'))

  // Create the tip button
  const braveTipButton = document.createElement('button')
  braveTipButton.className = 'StackOverflowTip-actionButton u-textUserColorHover js-actionButton'
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
    const githubMetaData = getMetaData(elem)
    if (githubMetaData) {
      const msg = { type: 'tipInlineMedia', mediaMetaData: githubMetaData }
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
  braveTipActionCountPresentation.className = 'StackOverflowTip-actionCountForPresentation'
  braveTipActionCountPresentation.textContent = getMessage('githubTipsIconLabel')
  braveTipActionCount.appendChild(braveTipActionCountPresentation)

  // Create the shadow DOM root that hosts our injected DOM elements
  const shadowRoot = braveTipAction.attachShadow({ mode: 'open' })
  shadowRoot.appendChild(braveTipButton)

  // Create style element for hover color
  const style = document.createElement('style')
  style.innerHTML = '.StackOverflowTip-actionButton :hover { color: #FB542B }'
  shadowRoot.appendChild(style)

  return braveTipAction
}

const configureStackOverflowTipAction = (tipLocationClass: string, tippingEnabled: boolean,
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

const votingContainerClass = 'js-voting-container'
const getVotingContainerData = (elem: Element): RewardsTip.MediaMetaData | null => {
    console.log(elem.getAttribute('data-post-id'))
    if (elem.getAttribute('data-post-id')) {
      return {
          mediaType: 'stackoverflow',
          postId: elem.getAttribute('data-post-id') || ''
      }
    }
    return null
}

const votingContainerInsertFunction = (parent: Element) => {
    const tipElem = createBraveTipAction(parent, getVotingContainerData)
    parent.appendChild(tipElem)
}

const configureStackOverflowTipActions = (tippingEnabled: boolean) => {
  let tipLocationClasses = [votingContainerClass]
  let insertFunctions = [votingContainerInsertFunction]

  for (let i in tipLocationClasses) {
    configureStackOverflowTipAction(tipLocationClasses[i], tippingEnabled, insertFunctions[i])
  }
}

const configureBraveTipAction = () => {
  clearTimeout(timeout)
  chrome.runtime.sendMessage('rewardsEnabled', function (rewards) {
    const msg = {
      type: 'inlineTipSetting',
      key: 'stackoverflow'
    }
    chrome.runtime.sendMessage(msg, function (inlineTip) {
      let tippingEnabled = rewards.enabled && inlineTip.enabled
      configureStackOverflowTipActions(tippingEnabled)
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

configureBraveTipAction()
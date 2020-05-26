
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { getMessage } from '../background/api/locale_api'

let timeout: any = null

const tipActionCountClass = 'GitHubTip-actionCount'
const tipIconContainerClass = 'IconContainer'
const actionTipClass = 'action-brave-tip'

const createBraveTipAction = (elem: Element, getMetaData: (elem: Element) => RewardsTip.MediaMetaData | null) => {
  const hoverClasses = ' tooltipped tooltipped-sw tooltipped-align-right-1'

  // Create the tip action
  const braveTipAction = document.createElement('div')
  braveTipAction.className = 'GitHubTip-action js-tooltip ' + actionTipClass + hoverClasses
  braveTipAction.style.display = 'inline-block'
  braveTipAction.style.minWidth = '40px'
  braveTipAction.setAttribute('aria-label', getMessage('githubTipsHoverText'))

  // Create the tip button
  const braveTipButton = document.createElement('button')
  braveTipButton.className = 'GitHubTip-actionButton u-textUserColorHover js-actionButton'
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
  braveTipActionCountPresentation.className = 'GitHubTip-actionCountForPresentation'
  braveTipActionCountPresentation.textContent = getMessage('githubTipsIconLabel')
  braveTipActionCount.appendChild(braveTipActionCountPresentation)

  // Create the shadow DOM root that hosts our injected DOM elements
  const shadowRoot = braveTipAction.attachShadow({ mode: 'open' })
  shadowRoot.appendChild(braveTipButton)

  // Create style element for hover color
  const style = document.createElement('style')
  style.innerHTML = '.GitHubTip-actionButton :hover { color: #FB542B }'
  shadowRoot.appendChild(style)

  return braveTipAction
}

const getQueryStringVal = (key: string, queryString: string): string | null => {
  const match = queryString.match(`[\?|&]${key}=([^&]+)&?`)
  return match && match[1]
}

const isBlackListTab = (queryString: string) => {
  const blackList = ['repositories']
  const tab = getQueryStringVal('tab', queryString)
  if (!tab || blackList.includes(tab)) { // Don't inject on overview tab
    return true
  }
  return false
}

const getCommentMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  let ancestor = elem.closest('.timeline-comment-header')
  ancestor = ancestor ? ancestor : elem.closest('.review-comment')
  if (ancestor) {
    const authorCollection = ancestor.getElementsByClassName('author')
    if (authorCollection.length) {
      const author = authorCollection[0] as HTMLElement
      const userName = author.innerHTML
      return {
        mediaType: 'github',
        userName: userName || ''
      }
    }
  }
  return null
}

const commentInsertFunction = (parent: Element) => {
  const tipElem = createBraveTipAction(parent, getCommentMetaData)
  if (tipElem.shadowRoot) {
    let iconContainer = tipElem.shadowRoot.querySelectorAll('.' + tipIconContainerClass)[0] as HTMLElement
    let braveTipActionCount = tipElem.shadowRoot.querySelectorAll('.' + tipActionCountClass)[0] as HTMLElement
    iconContainer.style.paddingBottom = '5px'
    braveTipActionCount.style.paddingBottom = '2px'
    tipElem.style.marginRight = '2px'
  }
  const children = parent.childNodes
  if (children.length > 1) {
    const end = children[children.length - 2]
    parent.insertBefore(tipElem, end)
  }
}

const getReviewItemMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const ancestor = elem.closest('.discussion-item')
  if (ancestor) {
    const authorCollection = ancestor.getElementsByClassName('author')
    if (authorCollection.length) {
      const author = authorCollection[0] as HTMLElement
      const userName = author.innerHTML
      return {
        mediaType: 'github',
        userName: userName || ''
      }
    }
  }
  return null
}

const reviewItemInsertFunction = (parent: Element) => {
  const tipLocation = parent.getElementsByClassName('discussion-item-copy')[0]
  if (tipLocation) {
    const tipElem = createBraveTipAction(tipLocation, getReviewItemMetaData)
    if (tipElem.shadowRoot) {
      const iconContainerCollection = tipElem.shadowRoot.querySelectorAll('.' + tipIconContainerClass)
      const braveTipActionCountCollection = tipElem.shadowRoot.querySelectorAll('.' + tipActionCountClass)
      if (iconContainerCollection.length && braveTipActionCountCollection.length) {
        const iconContainer = iconContainerCollection[0] as HTMLElement
        const braveTipActionCount = braveTipActionCountCollection[0] as HTMLElement
        iconContainer.style.paddingBottom = '5px'
        braveTipActionCount.style.paddingBottom = '2px'
      }
    }
    tipLocation.appendChild(tipElem)
  }
}

const getCommitLinksMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const ancestor = elem.closest('.commit')
  if (ancestor) {
    const avatarCollection = ancestor.getElementsByClassName('avatar')
    if (avatarCollection.length) {
      const avatar = avatarCollection[0] as HTMLElement
      const avatarChildren = avatar.children
      if (avatarChildren.length) {
        const userName = (avatarChildren[0] as HTMLImageElement).alt.slice(1)
        return {
          mediaType: 'github',
          userName: userName || ''
        }
      }
    }
  }
  return null
}

const commitLinksInsertFunction = (parent: Element) => {
  const tipElem = createBraveTipAction(parent, getCommitLinksMetaData)
  tipElem.style.marginLeft = '9px'
  parent.appendChild(tipElem)
}

const getStarringContainerMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const ancestor = elem.closest('.d-block')
  if (ancestor) {
    const aTags = ancestor.getElementsByTagName('A')
    if (aTags.length) {
      const aTag = aTags[0] as HTMLAnchorElement
      if (aTag.href) {
        const split = aTag.href.split('/')
        if (split.length > 3) {
          const userName = split[3]
          return {
            mediaType: 'github',
            userName: userName || ''
          }
        }
      }
    }
  }
  return null
}

const starringContainerInsertFunction = (parent: Element) => {
  if (isBlackListTab(window.location.search)) {
    return
  }
  const correctSubclass = 'starring-container'
  if (!parent.getElementsByClassName(correctSubclass).length) {
    return
  }
  const subElem = parent.getElementsByClassName(correctSubclass)[0]
  const tipElem = createBraveTipAction(parent, getStarringContainerMetaData)
  tipElem.classList.add('d-inline-block')
  tipElem.style.minWidth = '60px'
  parent.insertBefore(tipElem, subElem)
}

const getPageHeadMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const elems = document.getElementsByClassName('gisthead')
  if (elems.length) {
    elem = elems[0]
    const authors = elem.getElementsByClassName('author')
    if (authors.length) {
      const author = authors[0]
      const aTags = author.getElementsByTagName('A')
      if (aTags.length) {
        const aTag = aTags[0] as HTMLAnchorElement
        const userName = aTag.innerHTML
        return {
          mediaType: 'github',
          userName: userName || ''
        }
      }
    }
  }
  return null
}

const pageheadInsertFunction = (parent: Element) => {
  const subdomain = window.location.host.split('.')[0]
  if (subdomain === 'gist' &&
      window.location.pathname.slice(1).split('/').length > 1) {
    const tipElem = createBraveTipAction(parent, getPageHeadMetaData)
    parent.appendChild(tipElem)
  }
}

const getMemberListItemMetaData = (elem: Element): RewardsTip.MediaMetaData | null => {
  const ancestor = elem.closest('.table-list-cell')
  if (ancestor) {
    const aTags = ancestor.getElementsByTagName('A')
    if (aTags.length) {
      const aTag = aTags[0] as HTMLAnchorElement
      if (aTag.href) {
        const split = aTag.href.split('/')
        if (split.length > 3) {
          const userName = split[3]
          return {
            mediaType: 'github',
            userName: userName || ''
          }
        }
      }
    }
  }
  return null
}

const memberListItemInsertFunction = (parent: Element) => {
  if (parent.children.length > 1) {
    const path = window.location.pathname
    const memberText = parent.children[1] as HTMLElement
    const tipElem = createBraveTipAction(memberText as Element, getMemberListItemMetaData)
    if (!memberText || path.startsWith('/orgs/')) {
      return
    }
    tipElem.style.paddingLeft = '5px'
    if (path.split('/').includes('teams')) { // Special case, different styling for same element
      memberText.appendChild(tipElem)
    } else {
      memberText.style.width = '250px'
      if (memberText.children.length) {
        memberText.insertBefore(tipElem, memberText.children[1])
      }
    }
  }
}

const commentParentClass = 'timeline-comment-actions'
const reviewItemParentClass = 'discussion-item-review'
const commitLinksParentClass = 'commit-links-cell'
const starringContainerParentClass = 'float-right'
const pageHeadParentClass = 'pagehead-actions'
const memberListItemParentClass = 'member-list-item'

const configureGitHubTipAction = (tipLocationClass: string, tippingEnabled: boolean,
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

const configureGitHubTipActions = (tippingEnabled: boolean) => {
  let tipLocationClasses = [commentParentClass,
    reviewItemParentClass,
    commitLinksParentClass,
    starringContainerParentClass,
    pageHeadParentClass,
    memberListItemParentClass
  ]

  let insertFunctions = [commentInsertFunction,
    reviewItemInsertFunction,
    commitLinksInsertFunction,
    starringContainerInsertFunction,
    pageheadInsertFunction,
    memberListItemInsertFunction
  ]

  for (let i in tipLocationClasses) {
    configureGitHubTipAction(tipLocationClasses[i], tippingEnabled, insertFunctions[i])
  }
}

const configureBraveTipAction = () => {
  clearTimeout(timeout)
  chrome.runtime.sendMessage('rewardsEnabled', function (rewards) {
    const msg = {
      type: 'inlineTippingPlatformEnabled',
      key: 'github'
    }
    chrome.runtime.sendMessage(msg, function (inlineTip) {
      let tippingEnabled = rewards.enabled && inlineTip.enabled
      configureGitHubTipActions(tippingEnabled)
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

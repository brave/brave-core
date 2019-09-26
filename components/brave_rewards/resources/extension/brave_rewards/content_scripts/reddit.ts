/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { getMessage } from '../background/api/locale_api'

let timeout: any = null

const isOldReddit = () => {
  const redditUrl = new URL(document.URL)
  return redditUrl.hostname.startsWith('old') ||
    redditUrl.hostname.startsWith('np')
}

const getOldRedditMetaData = (redditPost: Element): RewardsTip.MediaMetaData | null => {
  if (!redditPost) {
    return null
  }
  let postText = ''
  let postTextElements = redditPost.getElementsByClassName('md')
  let postTitleElement: HTMLAnchorElement | null = redditPost.querySelector('a[data-event-action="title"]')
  if (!postTextElements || postTextElements.length === 0) {
    postText = postTitleElement && postTitleElement.innerText ? postTitleElement.innerText : ''
  } else {
    const divPostTextElement = (postTextElements[0] as HTMLDivElement)
    if (divPostTextElement && divPostTextElement.innerText) {
      postText = divPostTextElement.innerText
    }
  }
  if (postText.length > 200) {
    postText = postText.substr(0, 200) + '...'
  }

  let userName = ''
  // it's possible that two author classes could show up on a comments feed.
  // The post author and an author commenting
  // Get 'entry' div first to id the author commenting
  let authorAnchorElement
  let entryDiv = redditPost.getElementsByClassName('entry')
  if (entryDiv) {
    authorAnchorElement = entryDiv[0].getElementsByClassName('author')
  }
  if (authorAnchorElement) {
    const divPostTextElement = (authorAnchorElement[0] as HTMLAnchorElement)
    if (divPostTextElement && divPostTextElement.textContent) {
      userName = divPostTextElement.textContent
    }
  }

  let postRelDate = ''
  let postDateElement = redditPost.getElementsByTagName('time')
  if (postDateElement && postDateElement.length > 0) {
    postRelDate = postDateElement[0].textContent || ''
  }

  return {
    mediaType: 'reddit',
    userName: userName,
    postText: postText || '',
    postRelDate: postRelDate || ''
  }
}

const getRedditMetaData = (redditPost: Element): RewardsTip.MediaMetaData | null => {
  if (!redditPost) {
    return null
  }

  let postText = ''
  let postTextElements = redditPost.getElementsByTagName('p')
  if (!postTextElements || postTextElements.length === 0) {
    postTextElements = redditPost.getElementsByTagName('h3')
    if (postTextElements && postTextElements.length > 0) {
      if (postTextElements[0].innerText) {
        postText = postTextElements[0].innerText
      }
    } else {
      postTextElements = redditPost.getElementsByTagName('h1')
      if (postTextElements[0].innerText) {
        postText = postTextElements[0].innerText
      }
    }
  } else {
    let postTextElement = (postTextElements[0] as HTMLElement).parentElement
    if (postTextElement && postTextElement.innerText) {
      postText = postTextElement.innerText
    }
  }
  if (postText.length > 200) {
    postText = postText.substr(0, 200) + '...'
  }

  const anchor: HTMLAnchorElement | null = redditPost.querySelector('a[href^="/user/"]:not([data-click-id="body"]):not([data-click-id="subreddit"])')
  let userName = ''
  if (anchor && anchor.textContent) {
    userName = anchor.textContent.startsWith('u/') ? anchor.textContent.split('/')[1] : anchor.textContent
  }

  if (userName.length == 0) {
    const spanUsername = redditPost.getElementsByClassName("lizQBHVukyun2S2babj-l _2tbHP6ZydRpjI44J3syuqC")[0]
    if (spanUsername && spanUsername.textContent) {
      userName = spanUsername.textContent.startsWith('u/') ? spanUsername.textContent.split('/')[1] : spanUsername.textContent
    }
  }

  let postRelDate = ''
  let postDateElement = redditPost.querySelector('a[data-click-id="timestamp"]')
  if (postDateElement) {
    postRelDate = postDateElement.textContent || ''
  } else {
    const commentPartElement = redditPost.querySelector('div[data-test-id="comment"]')
    if (commentPartElement) {
      const authorRowDiv = commentPartElement.previousElementSibling
      if (authorRowDiv) {
        const timeLink = authorRowDiv.getElementsByTagName('a')
        if (timeLink && timeLink.length > 1) {
          const timeSpan = timeLink[1].getElementsByTagName('span')
          if (timeSpan) {
            postRelDate = timeSpan[0].textContent || ''
          }
        }
      }
    }
  }

  return {
    mediaType: 'reddit',
    userName: userName,
    postText: postText || '',
    postRelDate: postRelDate
  }
}

const createOldRedditTipButton = () => {
  const braveTipButton = document.createElement('a')
  braveTipButton.className = 'reddit-actionButton'
  braveTipButton.href = 'javascript:void(0)'
  braveTipButton.textContent = getMessage('redditTipsIconLabel')
  return braveTipButton
}

const createRedditTipButton = () => {
  const braveTipButton = document.createElement('button')
  braveTipButton.className = 'reddit-actionButton'
  braveTipButton.style.display = 'inline-block'
  braveTipButton.style.transition = 'color 0.1s ease 0s'
  braveTipButton.style.background = 'transparent'
  braveTipButton.style.border = 'none'
  braveTipButton.style.color = 'inherit'
  braveTipButton.style.cursor = 'pointer'
  braveTipButton.style.padding = '2px 10px 0 10px'
  braveTipButton.style.borderRadius = '2px'
  braveTipButton.style.outline = 'none'
  braveTipButton.type = 'button'

  const style = document.createElement('style')
  style.innerHTML = '.reddit-actionButton :hover { color: #FB542B }'
  braveTipButton.appendChild(style)

  return braveTipButton
}

const createRedditIconContainer = () => {
  const braveTipIconContainer = document.createElement('div')
  braveTipIconContainer.className = 'IconContainer'
  braveTipIconContainer.style.display = 'inline-block'
  braveTipIconContainer.style.marginBottom = '-2px'
  braveTipIconContainer.style.position = 'relative'
  braveTipIconContainer.style.verticalAlign = 'middle'
  return braveTipIconContainer
}

const createOldRedditIconContainer = () => {
  const braveTipIconContainer = document.createElement('div')
  braveTipIconContainer.className = 'IconContainer'
  braveTipIconContainer.style.display = 'inline-block'
  braveTipIconContainer.style.marginBottom = '-2px'
  braveTipIconContainer.style.position = 'relative'
  braveTipIconContainer.style.verticalAlign = 'middle'
  return braveTipIconContainer
}

const createRedditTipAction = (isPost: boolean) => {
  const braveTipAction = document.createElement('div')
  braveTipAction.className = 'action-brave-tip'
  if (isPost) {
    braveTipAction.style.display = 'flex'
    braveTipAction.style.height = '25px'
    braveTipAction.style.outline = 'none'
    braveTipAction.style.borderRadius = '2px'
  }
  braveTipAction.setAttribute('data-original-title', getMessage('redditTipsHoverText'))
  return braveTipAction
}

const createOldRedditTipAction = () => {
  const braveTipAction = document.createElement('li')
  braveTipAction.className = 'action-brave-tip'
  return braveTipAction
}

const createRedditTipIcon = () => {
  const braveTipIcon = document.createElement('span')
  braveTipIcon.className = 'tip-icon--medium'
  braveTipIcon.style.background = 'transparent'
  braveTipIcon.style.content = 'url(\'data:image/svg+xml;utf8,<svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px" viewBox="0 0 105 100" style="enable-background:new 0 0 105 100;" xml:space="preserve"><style type="text/css">.st1{fill:%23662D91;}.st2{fill:%239E1F63;}.st3{fill:%23FF5000;}.st4{fill:%23FFFFFF;stroke:%23FF5000;stroke-width:0.83;stroke-miterlimit:10;}</style><title>BAT_icon</title><g id="Layer_2_1_"><g id="Layer_1-2"><polygon class="st1" points="94.8,82.6 47.4,55.4 0,82.9 "/><polygon class="st2" points="47.4,0 47.1,55.4 94.8,82.6 "/><polygon class="st3" points="0,82.9 47.2,55.9 47.4,0 "/><polygon class="st4" points="47.1,33.7 28,66.5 66.7,66.5 "/></g></g></svg>\')'
  braveTipIcon.style.display = 'inline-block'
  braveTipIcon.style.fontSize = '18px'
  braveTipIcon.style.fontStyle = 'normal'
  braveTipIcon.style.height = '16px'
  braveTipIcon.style.position = 'relative'
  braveTipIcon.style.verticalAlign = 'baseline'
  braveTipIcon.style.width = '16px'
  return braveTipIcon
}

const createRedditTipActionCount = () => {
  const braveTipActionCount = document.createElement('span')
  braveTipActionCount.className = 'reddit-actionCount'
  braveTipActionCount.style.color = 'inherit'
  braveTipActionCount.style.display = 'inline-block'
  braveTipActionCount.style.fontSize = '12px'
  braveTipActionCount.style.fontWeight = 'bold'
  braveTipActionCount.style.lineHeight = '1'
  braveTipActionCount.style.marginLeft = '3px'
  braveTipActionCount.style.position = 'relative'
  braveTipActionCount.style.verticalAlign = 'text-bottom'
  return braveTipActionCount
}

const createRedditTipActionCountPresentation = () => {
  const braveTipActionCountPresentation = document.createElement('span')
  braveTipActionCountPresentation.className = 'reddit-actionButton'
  braveTipActionCountPresentation.textContent = getMessage('redditTipsIconLabel')
  return braveTipActionCountPresentation
}

const createHoverStyleElement = (isPost: boolean) => {
  // Create style element for hover
  const style = document.createElement('style')
  style.innerHTML = isPost ? ':host { outline: none } :host(:hover) { background-color: var(--newRedditTheme-navIconFaded10) }' : '.reddit-actionButton { text-decoration: none; color: var(--newCommunityTheme-actionIcon); font-weight: bold; padding: 0px 1px; } .reddit-actionButton:hover { color: var(--newCommunityTheme-bodyText); text-decoration: underline }'
  return style
}

const createHoverStyleElementForOld = () => {
  const style = document.createElement('style')
  style.innerHTML = '.reddit-actionButton { color: #888; font-weight: bold; paddings: 0 1px; text-decoration: none } .reddit-actionButton:hover { text-decoration: underline }'
  return style
}

const getMoreActionCommentElement = (commentElement: Element) => {
  return commentElement.querySelector('button[aria-label="more options"]')
}

const getMoreActionPostElement = (postElement: Element) => {
  const element = postElement.querySelector('button[aria-label="more options"]')
  if (element) {
    return !element.nextElementSibling && !element.previousElementSibling && element.parentElement ? element.parentElement : element
  }
  return null
}

const getSaveElement = (commentElement: Element) => {
  return document.evaluate(".//button[text()='Save']", commentElement, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue as HTMLElement
}

const getPromotedSaveElement = (element: Element) => {
  const saveElement = document.evaluate(".//span[text()='save']", element, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue as HTMLElement
  if (saveElement && saveElement.parentElement) {
    return saveElement.parentElement
  }
  return null
}

const isUsersOwnPost = (commentElement: Element) => {
  const editElement: Node = document.evaluate(".//button[text()='edit']", commentElement, null, XPathResult.FIRST_ORDERED_NODE_TYPE, null).singleNodeValue
  return editElement
}

const createBraveTipActionForOld = (redditPost: Element) => {
  // Create the tip action
  const braveTipAction = createOldRedditTipAction()

  // Create a container that is eligible to attach shadow DOM
  const braveTipActionContainer = document.createElement('span')
  braveTipAction.appendChild(braveTipActionContainer)

  const braveTipIconContainer = createOldRedditIconContainer()

  const braveTipIcon = createRedditTipIcon()
  braveTipIconContainer.appendChild(braveTipIcon)

  // Create the tip button
  const braveTipButton = createOldRedditTipButton()
  braveTipButton.appendChild(braveTipIconContainer)
  // Create button event
  braveTipButton.onclick = function (event) {
    const redditMetaData = getOldRedditMetaData(redditPost)
    if (redditMetaData) {
      const msg = { type: 'tipInlineMedia', mediaMetaData: redditMetaData }
      chrome.runtime.sendMessage(msg)
    }
    event.stopPropagation()
  }

  const shadowRoot = braveTipActionContainer.attachShadow({ mode: 'open' })
  shadowRoot.appendChild(braveTipIconContainer)
  shadowRoot.appendChild(braveTipButton)
  shadowRoot.appendChild(createHoverStyleElementForOld())
  return braveTipAction
}

const createBraveTipAction = (redditPost: Element, isPost: boolean) => {
  // Create the tip action
  const braveTipAction = createRedditTipAction(isPost)

  // Create the tip button
  const braveTipButton = createRedditTipButton()

  // Create button event
  braveTipButton.onclick = function (event) {
    const redditMetaData = getRedditMetaData(redditPost)
    if (redditMetaData) {
      const msg = { type: 'tipInlineMedia', mediaMetaData: redditMetaData }
      chrome.runtime.sendMessage(msg)
    }
    event.stopPropagation()
  }

  // Create the tip icon container
  const braveTipIconContainer = createRedditIconContainer()
  braveTipButton.appendChild(braveTipIconContainer)

  // Create the tip icon
  const braveTipIcon = createRedditTipIcon()
  braveTipIconContainer.appendChild(braveTipIcon)

  // Create the tip action count
  const braveTipActionCount = createRedditTipActionCount()
  braveTipButton.appendChild(braveTipActionCount)

  // Create the tip action count presentation
  const braveTipActionCountPresentation = createRedditTipActionCountPresentation()
  braveTipActionCount.appendChild(braveTipActionCountPresentation)

  // Create the shadow DOM root
  const shadowRoot = braveTipAction.attachShadow({ mode: 'open' })
  shadowRoot.appendChild(braveTipButton)

  const hoverStyleElement = createHoverStyleElement(isPost)
  shadowRoot.appendChild(hoverStyleElement)

  return braveTipAction
}

const handleConfigureForSaveElement = (element: Element, isPost: boolean, isPromoPost: boolean) => {
  const saveElement = isPromoPost ? getPromotedSaveElement(element) : getSaveElement(element)
  if (saveElement) {
    saveElement.insertAdjacentElement(
      'afterend', createBraveTipAction(element, isPost))
  }
}

const handleConfigureForMoreInfoElement = (element: Element, lastElement: Element, isPost: boolean, isUsersPost: boolean) => {
  if (!isUsersPost) {
    lastElement.insertAdjacentElement(
      'beforebegin', createBraveTipAction(element, isPost))
  } else if (lastElement.parentElement && isUsersPost) {
    lastElement.parentElement.insertAdjacentElement(
      'beforebegin', createBraveTipAction(element, isPost))
  }
}

const configureForPosts = (rewardsEnabled: boolean, inlineTipEnabled: boolean, isForPosts: boolean, isForPromotedPosts: boolean) => {
  // special case -- use this for promoted content when user isn't logged in
  const postElements = isForPosts ? document.getElementsByClassName('Post') : document.getElementsByClassName('Comment')
  for (let ix = 0; ix < postElements.length; ix++) {
    const isUsersPost = isUsersOwnPost(postElements[ix])
    const actions = postElements[ix].querySelectorAll('div.action-brave-tip')
    const inEditModeElements = postElements[ix].querySelector('div[data-test-id="comment-submission-form-richtext"')
    if (rewardsEnabled && inlineTipEnabled && actions && actions.length === 0 && !inEditModeElements) {
      const lastElement = isForPosts ? getMoreActionPostElement(postElements[ix]) : getMoreActionCommentElement(postElements[ix])
      if (lastElement) {
        handleConfigureForMoreInfoElement(postElements[ix], lastElement, isForPosts, !!isUsersPost)
      } else {
        handleConfigureForSaveElement(postElements[ix], isForPosts, isForPromotedPosts)
      }
    }
  }
}

const configureForOldReddit = (rewardsEnabled: boolean, inlineTipEnabled: boolean, postType: string) => {
  const elements = document.querySelectorAll('div[data-type="' + postType + '"]')
  for (let ix = 0; ix < elements.length; ix++) {
    const actions = elements[ix].querySelectorAll('li.action-brave-tip')
    if (rewardsEnabled && inlineTipEnabled && actions && actions.length === 0) {
      const ulElement = elements[ix].getElementsByClassName('flat-list')
      if (ulElement && ulElement.length !== 0) {
        ulElement[0].insertAdjacentElement('beforeend', createBraveTipActionForOld(elements[ix]))
      }
    }
  }
}

const configureRedditTipAction = (rewardsEnabled: boolean, inlineTipEnabled: boolean, isOldReddit: boolean) => {
  if (isOldReddit) {
    // for comments, replies, etc
    configureForOldReddit(rewardsEnabled, inlineTipEnabled, 'comment')

    // for initial posts, etc
    configureForOldReddit(rewardsEnabled, inlineTipEnabled, 'link')
  } else {
    // for comments, replies, etc
    configureForPosts(rewardsEnabled, inlineTipEnabled, false, false)

    // for initial posts
    configureForPosts(rewardsEnabled, inlineTipEnabled, true, false)

    // for promoted posts (on feeds)
    configureForPosts(rewardsEnabled, inlineTipEnabled, true, true)
  }
}

const configureBraveTipAction = () => {
  clearTimeout(timeout)
  chrome.runtime.sendMessage('rewardsEnabled', function (rewards) {
    const msg = {
      type: 'inlineTipSetting',
      key: 'reddit'
    }
    chrome.runtime.sendMessage(msg, function (inlineTip) {
      configureRedditTipAction(rewards.enabled, inlineTip.enabled, isOldReddit())
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

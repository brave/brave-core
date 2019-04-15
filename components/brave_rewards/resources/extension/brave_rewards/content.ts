/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { getMessage } from './background/api/locale_api'

const getTweetMetaData = (tweet: Element): RewardsDonate.TweetMetaData | null => {
  if (!tweet) {
    return null
  }
  const tweetTextElements = tweet.getElementsByClassName('tweet-text')
  if (!tweetTextElements || tweetTextElements.length === 0) {
    return null
  }
  return {
    name: tweet.getAttribute('data-name') || '',
    screenName: tweet.getAttribute('data-screen-name') || '',
    userId: tweet.getAttribute('data-user-id') || '',
    tweetText: tweetTextElements[0].textContent || ''
  }
}

const createBraveTipAction = (tweet: Element) => {
  // Create the tip action
  const braveTipAction = document.createElement('div')
  braveTipAction.className = 'ProfileTweet-action action-brave-tip'

  // Create the tip button
  const braveTipButton = document.createElement('button')
  braveTipButton.className = 'ProfileTweet-actionButton u-textUserColorHover js-actionButton'
  braveTipButton.type = 'button'
  braveTipButton.onclick = function (event) {
    const tweetMetaData = getTweetMetaData(tweet)
    if (tweetMetaData) {
      const msg = { type: 'donateToTwitterUser', ...tweetMetaData }
      chrome.runtime.sendMessage(msg)
    }
  }
  braveTipAction.appendChild(braveTipButton)

  // Create the tip icon container
  const braveTipIconContainer = document.createElement('div')
  braveTipIconContainer.className = 'IconContainer js-tooltip'
  braveTipIconContainer.setAttribute('data-original-title', getMessage('twitterTipsHoverText'))
  braveTipButton.appendChild(braveTipIconContainer)

  // Create the tip icon
  const braveTipIcon = document.createElement('span')
  braveTipIcon.className = 'Icon Icon--medium brave-tip-icon'
  braveTipIconContainer.appendChild(braveTipIcon)

  // Create the tip action count (typically used to present a counter
  // associated with the action, but we'll use it to display a static
  // action label)
  const braveTipActionCount = document.createElement('span')
  braveTipActionCount.className = 'ProfileTweet-actionCount'
  braveTipButton.appendChild(braveTipActionCount)

  const braveTipActionCountPresentation = document.createElement('span')
  braveTipActionCountPresentation.className = 'ProfileTweet-actionCountForPresentation'
  braveTipActionCountPresentation.textContent = getMessage('twitterTipsIconLabel')
  braveTipActionCount.appendChild(braveTipActionCountPresentation)

  return braveTipAction
}

const configureBraveTipAction = () => {
  chrome.runtime.sendMessage('rewardsEnabled', function (response) {
    const tweets = document.getElementsByClassName('tweet')
    for (let i = 0; i < tweets.length; ++i) {
      const actions = tweets[i].getElementsByClassName('js-actions')[0]
      if (actions) {
        const braveTipActions = actions.getElementsByClassName('action-brave-tip')
        if (response.enabled) {
          if (braveTipActions.length === 0) {
            actions.appendChild(createBraveTipAction(tweets[i]))
          }
        } else {
          if (braveTipActions.length === 1) {
            actions.removeChild(braveTipActions[0])
          }
        }
      }
    }
  })
  setTimeout(configureBraveTipAction, 3000)
}

configureBraveTipAction()

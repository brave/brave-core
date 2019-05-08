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

  const tweetText = tweetTextElements[0] as HTMLElement

  const tweetTimestampElements = tweet.getElementsByClassName('js-short-timestamp')
  if (!tweetTimestampElements || tweetTimestampElements.length === 0) {
    return null
  }

  const tweetTimestamp = tweetTimestampElements[0].getAttribute('data-time') || ''

  return {
    name: tweet.getAttribute('data-name') || '',
    screenName: tweet.getAttribute('data-screen-name') || '',
    userId: tweet.getAttribute('data-user-id') || '',
    tweetId: tweet.getAttribute('data-tweet-id') || '',
    tweetTimestamp: parseInt(tweetTimestamp, 10) || 0,
    tweetText: tweetText.innerText || ''
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
      const msg = { type: 'donateToTwitterUser', tweetMetaData: tweetMetaData }
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
  braveTipIcon.className = 'Icon Icon--medium'
  braveTipIcon.style.content = 'url(\'data:image/svg+xml;utf8,<svg version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px" viewBox="0 0 105 100" style="enable-background:new 0 0 105 100;" xml:space="preserve"><style type="text/css">.st1{fill:%23662D91;}.st2{fill:%239E1F63;}.st3{fill:%23FF5000;}.st4{fill:%23FFFFFF;stroke:%23FF5000;stroke-width:0.83;stroke-miterlimit:10;}</style><title>BAT_icon</title><g id="Layer_2_1_"><g id="Layer_1-2"><polygon class="st1" points="94.8,82.6 47.4,55.4 0,82.9 "/><polygon class="st2" points="47.4,0 47.1,55.4 94.8,82.6 "/><polygon class="st3" points="0,82.9 47.2,55.9 47.4,0 "/><polygon class="st4" points="47.1,33.7 28,66.5 66.7,66.5 "/></g></g></svg>\')'
  braveTipIcon.style.marginTop = '5px'
  braveTipIcon.style.width = '16px'
  braveTipIcon.style.height = '16px'
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
  chrome.runtime.sendMessage('rewardsEnabled', function (rewards) {
    const msg = {
      type: 'inlineTipSetting',
      key: 'twitter'
    }
    chrome.runtime.sendMessage(msg, function (inlineTip) {
      const tweets = document.getElementsByClassName('tweet')
      for (let i = 0; i < tweets.length; ++i) {
        const actions = tweets[i].getElementsByClassName('js-actions')[0]
        if (actions) {
          const braveTipActions = actions.getElementsByClassName('action-brave-tip')
          if (rewards.enabled && inlineTip.enabled) {
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
  })
  setTimeout(configureBraveTipAction, 3000)
}

configureBraveTipAction()

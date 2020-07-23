import * as Background from '../../../../../common/Background'
import feedToData from './feedToData'

type RemoteData = BraveToday.ContentFromFeed[]

const feedUrl = 'https://pcdn.brave.software/brave-today/feed.json'

// TODO: db
let memoryTodayData: BraveToday.Feed | undefined

let readLock: Promise<void> | null

function updateFeed() {
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve) {
    try {
      const feedResponse = await fetch(feedUrl)
      if (feedResponse.ok) {
        const feedContents: RemoteData = await feedResponse.json()
        console.log('Got feed', feedContents)
        memoryTodayData = feedToData(feedContents)
      } else {
        console.error(`Not ok when fetching feed. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not process feed contents from ', feedUrl)
      throw e
    } finally {
      readLock = null
    }
  })
}



// function on(messageType: string)
// TODO: make this a common thing and do explicit types for payloads like
// we do with redux action payloads.
import MessageTypes = Background.MessageTypes.Today
import Messages = BraveToday.Messages

Background.setListener<void>(
  MessageTypes.indicatingOpen,
  async function (payload, sender) {
    console.log('indicatingOpen')
    updateFeed()
  }
)

Background.setListener<Messages.GetFeedResponse>(
  MessageTypes.getFeed,
  async function (req, sender, sendResponse) {
    console.log('asked to get feed')
    if (!memoryTodayData) {
      // Fetch but only once at a time, and wait. No point returning no data
      if (!readLock) {
        updateFeed()
      }
      await readLock
    }
    await Promise.resolve(true)
    // Only wait once. If there was an error or no data then return nothing.
    // TODO: return error status
    console.log('sending', memoryTodayData)

    sendResponse({
      feed: memoryTodayData
    })
  }
)





// chrome.runtime.onMessageExternal.addListener(function (req, sender, sendResponse) {
//   console.log('got something', req, sender);
//   sendResponse({ got: true });
// })

// TODO: schedule to update feed
// updateFeed()

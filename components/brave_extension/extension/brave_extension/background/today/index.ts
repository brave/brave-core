import * as Background from '../../../../../common/Background'
import feedToData from './feedToData'

type RemoteData = BraveToday.ContentFromFeed[]

const feedUrl = 'https://pcdn.brave.software/brave-today/feed.json'

// TODO: db
let memoryTodayData: BraveToday.Feed | undefined

let readLock: Promise<void> | null

const getLocalDataLock = new Promise<void>(resolve => {
  chrome.storage.local.get('today', (data) => {
    if (data && data.today) {
      memoryTodayData = data.today
    }
    resolve()
  })
})


function performUpdateFeed() {
  // Sanity check
  if (readLock) {
    console.error('Asked to update feed but already waiting for another update!')
    return
  }
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve, reject) {
    try {
      const feedResponse = await fetch(feedUrl)
      if (feedResponse.ok) {
        const feedContents: RemoteData = await feedResponse.json()
        console.log('Got feed', feedContents)
        memoryTodayData = await feedToData(feedContents)
        resolve()
        chrome.storage.local.set({ today: memoryTodayData })
      } else {
        throw new Error(`Not ok when fetching feed. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not process feed contents from ', feedUrl)
      reject(e)
    } finally {
      readLock = null
    }
  })
}


async function getOrFetchData() {
  await getLocalDataLock
  if (memoryTodayData) {
    return memoryTodayData
  } else {
    return await updateFeed()
  }
}

async function updateFeed() {
  // Fetch but only once at a time, and wait.
  if (!readLock) {
    performUpdateFeed()
  }
  await readLock
  return memoryTodayData
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
    const feed = await getOrFetchData()
    // Only wait once. If there was an error or no data then return nothing.
    // TODO: return error status
    console.log('sending', feed)
    sendResponse({
      feed
    })
  }
)

Background.setListener<Messages.GetFeedImageDataResponse, Messages.GetFeedImageDataPayload>(
  MessageTypes.getFeedImageData,
  async function (req, sender, sendResponse) {
    const blob = await fetch(req.url).then(r => r.blob());
    const dataUrl: string = await new Promise(resolve => {
      let reader = new FileReader();
      reader.onload = () => resolve(reader.result as string);
      reader.readAsDataURL(blob);
    });
    sendResponse({
      dataUrl
    })
  }
)

// TODO: schedule to update feed

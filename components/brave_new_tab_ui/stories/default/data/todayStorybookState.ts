import { boolean } from '@storybook/addon-knobs'
import { BraveTodayState } from '../../../reducers/today'
import { feed, publishers } from './mockBraveNewsController'

export default function getTodayState (): BraveTodayState {
  const hasDataError = boolean('Today data fetch error?', false)
  return {
    isFetching: boolean('Today is fetching?', false),
    hasInteracted: boolean('Today has interacted?', false),
    isUpdateAvailable: boolean('Is Today update available?', false),
    currentPageIndex: 10,
    cardsViewed: 0,
    cardsVisited: 0,
    publishers: hasDataError ? undefined : publishers,
    feed: hasDataError ? undefined : feed
  }
}

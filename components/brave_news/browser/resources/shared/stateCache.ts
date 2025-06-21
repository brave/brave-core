import {
  BraveNewsControllerRemote,
  ListenerInterface,
  ListenerReceiver,
  UserEnabled,
  State,
  Configuration
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import getBraveNewsController, { isDirectFeed } from './api'

import { CachingWrapper, valueToJS } from '$web-common/mojomCache'
import { Value } from 'gen/mojo/public/mojom/base/values.mojom.m'

export class StateCachingWrapper
  extends CachingWrapper<State> implements ListenerInterface {
  private receiver = new ListenerReceiver(this)
  private controller: BraveNewsControllerRemote

  constructor() {
    super({
      configuration: {
        isOptedIn: false,
        showOnNTP: false,
        openArticlesInNewTab: false
      },
      channels: {},
      publishers: {},
      suggestedPublisherIds: []
    })

    this.controller = getBraveNewsController()

    // We can't set up  the mojo pipe in the test environment.
    if (process.env.NODE_ENV !== 'test') {
      this.controller.addListener(
        this.receiver.$.bindNewPipeAndPassRemote()
      )
    }
  }

  changed(diff: Value): void {
    const parsed = valueToJS<Partial<State>>(diff)
    if (!parsed) return

    this.notifyChanged({
      ...this.cache,
      ...parsed,
    })
  }

  setPublisherFollowed(publisherId: string, enabled: boolean) {
    const copy = {
      ...this.cache,
      publishers: {
        ...this.cache.publishers,
      }
    }

    if (isDirectFeed(this.cache.publishers[publisherId]) && !enabled) {
      this.controller.setPublisherPref(publisherId, UserEnabled.DISABLED)
      delete copy.publishers[publisherId]
    } else {
      const status = enabled ? UserEnabled.ENABLED : UserEnabled.NOT_MODIFIED
      this.controller.setPublisherPref(publisherId, status)
      copy.publishers[publisherId] = {
        ...this.cache.publishers[publisherId],
        userEnabledStatus: status
      }
    }

    this.notifyChanged(copy)
  }

  setChannelSubscribed(locale: string, channelId: string, subscribed: boolean) {
    // While we're waiting for the new channels to come back, speculatively
    // update them, so the UI has instant feedback.
    // This will be overwritten when the controller responds.
    let subscribedLocales = [
      ...(this.cache.channels[channelId]?.subscribedLocales ?? [])
    ]
    if (subscribedLocales.includes(locale)) {
      // Remove this locale from the list of subscribed locales.
      subscribedLocales = subscribedLocales.filter((l) => l !== locale)
    } else {
      // Add this locale to the list of subscribed locales.
      subscribedLocales.push(locale)
    }

    this.notifyChanged({
      ...this.cache,
      channels: {
        ...this.cache.channels,
        [channelId]: {
          ...this.cache.channels[channelId],
          subscribedLocales
        }
      }
    })

    return this.controller.setChannelSubscribed(locale, channelId, subscribed)
  }

  setConfiguration(change: Partial<Configuration>) {
    const newValue = { ...this.cache.configuration, ...change }
    this.controller.setConfiguration(newValue)
    this.notifyChanged({
      ...this.cache,
      configuration: newValue
    })
  }
}

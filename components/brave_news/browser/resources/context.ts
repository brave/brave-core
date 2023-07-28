import {
  BraveNewsController, Channel, Publisher
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as React from 'react'

export interface InspectContext {
  publishers: { [key: string]: Publisher },
  channels: { [key: string]: Channel },
}

export const api = BraveNewsController.getRemote();

const Context = React.createContext<InspectContext>({
  publishers: {},
  channels: {}
})

export const useInspectContext = () => {
  return React.useContext(Context);
}

export default function InspectContext() {
  // TODO: Use promise
  const publishers = {} as any
  const channels = {} as any
  const context = React.useMemo<>(() => ({
    publishers,
    channels
  }), [publishers, channels])
  return <Context.Provider value={context}>
  </Context.Provider>
}

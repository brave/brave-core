import {
  BraveNewsController, Channel, Publisher
} from 'gen/brave/components/brave_news/common/brave_news.mojom.m'
import * as React from 'react'
import usePromise from '../../../brave_new_tab_ui/hooks/usePromise'


export const pages = ['feed', 'signals'] as const;
export type Page = (typeof pages)[number]

export interface InspectContext {
  publishers: { [key: string]: Publisher },
  channels: { [key: string]: Channel },
  page: Page,
  setPage: (page: Page) => void
}

export const api = BraveNewsController.getRemote();

const Context = React.createContext<InspectContext>({
  publishers: {},
  channels: {},
  page: 'feed',
  setPage: () => { }
})

export const useInspectContext = () => {
  return React.useContext(Context);
}

export default function InspectContext(props: React.PropsWithChildren<{}>) {
  const { result: publishers } = usePromise(() => api.getPublishers().then(p => p.publishers as { [key: string]: Publisher }), [])
  const { result: channels } = usePromise(() => api.getChannels().then(c => c.channels as { [key: string]: Channel }), [])
  const [page, setPage] = React.useState<Page>('feed')

  const context = React.useMemo<InspectContext>(() => ({
    publishers: publishers ?? {},
    channels: channels ?? {},
    page,
    setPage
  }), [publishers, channels, page])

  return <Context.Provider value={context}>
    {props.children}
  </Context.Provider>
}

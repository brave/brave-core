import { useMemo, useState } from 'react'
import * as React from 'react'
import Modal from './Modal'

// Leave possibility for more pages open.
type NewsPage = null
    | 'news'

interface BraveNewsContext {
    page: NewsPage
    setPage: (page: NewsPage) => void
}

export const BraveNewsContext = React.createContext<BraveNewsContext>({
    page: null,
    setPage: () => {}
})

export function BraveNewsContextProvider (props: { children: React.ReactNode }) {
    const [page, setPage] = useState<NewsPage>('news')
    const context = useMemo(() => ({
        page,
        setPage
    }), [page])
    return <BraveNewsContext.Provider value={context}>
        {props.children}
        <Modal/>
    </BraveNewsContext.Provider>
}

export const useBraveNews = () => {
    return React.useContext(BraveNewsContext)
}

import { useMemo, useState } from 'react'
import * as React from 'react'

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
    </BraveNewsContext.Provider>
}

export const useBraveNews = () => {
    return React.useContext(BraveNewsContext)
}

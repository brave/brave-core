import Dropdown from '@brave/leo/react/dropdown';
import * as React from 'react';
import { pages, useInspectContext } from './context';

interface Props {
}

export default function PageInfo(props: Props) {
  const { page, setPage, publishers } = useInspectContext();
  const [locale, setLocale] = React.useState('en_US')
  const locales = React.useMemo(() => Array.from(new Set(Object.values(publishers).flatMap(p => p.locales.map(l => l.locale)))), [publishers])

  return <div>
    <div>
      <Dropdown value={page} onChange={e => setPage(e.detail.value)}>
        <span slot="label">Page</span>
        {pages.map(p => <leo-option key={p}>{p}</leo-option>)}
      </Dropdown>
    </div>
    <div>
      <Dropdown value={locale} onChange={e => setLocale(e.detail.value)}>
        <span slot="label">Locale</span>
        {locales.map(l => <leo-option key={l}>{l}</leo-option>)}
      </Dropdown>
    </div>
  </div>
}

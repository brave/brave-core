import * as React from 'react'
import styled from 'styled-components'
import { useBraveNews } from './Context'

const Configure = React.lazy(() => import('./Configure'))

const Dialog = styled.dialog`
  border-radius: 8px;
  border: none;
  width: min(100vw, 1092px);
  height: min(100vh, 712px);
  z-index: 1000;
  background: white;
  overflow: hidden;
  padding: 0;
  background-color: ${p => p.theme.color.contextMenuBackground};
  color:  ${p => p.theme.color.contextMenuForeground};
`

export default function BraveNewsModal () {
  const { customizePage } = useBraveNews()
  const dialogRef = React.useRef<HTMLDialogElement & { showModal: () => void, close: () => void, open: boolean }>()

  const shouldRender = !!customizePage

  // Note: There's no attribute for open modal, so we need
  // to call showModal instead.
  React.useEffect(() => {
    dialogRef.current?.showModal?.()
  }, [customizePage, dialogRef])

  // Only render the dialog if it should be shown, since
  // it is a complex view.
  return shouldRender ? <Dialog ref={dialogRef as any}>
    <React.Suspense fallback={<span>Loading...</span>}>
      <Configure />
    </React.Suspense>
  </Dialog> : null
}

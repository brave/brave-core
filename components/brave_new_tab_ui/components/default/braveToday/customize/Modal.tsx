import * as React from 'react'
import styled from 'styled-components'
import Configure from './Configure'
import { useBraveNews } from './Context'

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
  const { customizePage: page } = useBraveNews()
  const dialogRef = React.useRef<HTMLDialogElement & { showModal: () => void, close: () => void, open: boolean }>()

  // Note: There's no attribute for open modal, so we need
  // to do this instead.
  React.useEffect(() => {
    if (dialogRef.current?.open && !page) { dialogRef.current?.close?.() }
    if (!dialogRef.current?.open && page) { dialogRef.current?.showModal?.() }
  }, [page])
  return page && <Dialog ref={dialogRef as any}>
    <Configure />
  </Dialog>
}

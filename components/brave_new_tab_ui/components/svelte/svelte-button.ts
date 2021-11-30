import 'svelte'
import type { SvelteComponentTyped } from 'svelte'
import toReact from 'svelte-adapter/react'

import SvelteButton from './button.svelte'

// type MyType = {
//   buttonText: string
// }

// export default app
// type IWantToAcceptAComponent<T> = {
//   component: new (...args: any) => SvelteComponentTyped<{ record: T }>;
// }

// const componentPropObject: IWantToAcceptAComponent<MyType> = {
//   component: AComponent // error here
// }

// // const TypedSvelteButton = SvelteButton as SvelteComponentTyped<Props>

type ExtractProps<T> = T extends SvelteComponentTyped<infer P> ? P : never

export type Props = ExtractProps<SvelteButton>

const a: Props = {
  buttonText: 4
}

console.log(a)

const ReactSvelteButton = toReact(SvelteButton, {}, 'div')

export default ReactSvelteButton


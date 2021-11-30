import { SvelteComponentTyped } from 'svelte';

export interface Props {
    name: string
    // ...
}

export interface Events {
    // click: MouseEvent;
    // customEvent: CustomEvent<boolean>;
}

export interface Slots {
    // default: { slotValue: string };
    // named: { slotValue: string };
}

export default class SvelteButton extends SvelteComponentTyped<Props, Events, Slots> {}
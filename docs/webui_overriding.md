# Overriding Upstream WebUIs

Currently, there are a number of ways to override upstream WebUIs. Which one
you want will depend on what you're trying to do, and how the upstream WebUI is
implemented.

## chromium_src overrides

This is probably the simplest approach. Similarly to our C++ overrides just add
a new file at the same path but in the `chromium_src` directory and it will
replace the upstream version of the file. The upstream version can be imported
via `./<filename>-chromium.<ext>` if you need to reexport something from it.

**Note:** You don't need to reexport upstream stuff if you're completely
replacing it.

Similarly to C++ `chromium_src` overrides should be as minimal as possible to
provide the hooks Brave needs. When you need to do something more substantial
you should [add a new target](#adding-new-files) and make most of your changes
there. This makes code easier to follow, and means you don't have to jump back
and forward between `chromium_src` and `brave` files. As a bonus, it means you
won't need `chromium_src` approvals most of the time!

### Example Typescript

```ts
// in chromium_src/path/to/file.ts
import { frob as frobChromium } from './file-chromium.js'

// export everything from upstream
export * from './file-chromium.js'

// in Brave, all frobs are twice as good.
export function frob() {
  return frobChromium() * 2
}
```

### Example CSS

Note: This is slightly different depending on whether you're overriding a Lit
CSS file or a Polymer CSS file (you need to make sure `type` is correct in the
`css_wrapper_metadata`)

```css
/* in chromium_src/path/to/file.css */

/* #css_wrapper_metadata_start
 * #type=style-lit
 * #import=./file-chromium.css.js
 * #scheme=relative
 * #css_wrapper_metadata_end */

:host {
  background: hotpink;
}
```

## Lit Mangling

Unfortunately, the above strategies don't work for modifying Lit HTML. To modify
a Lit HTML template we need a way to directly modify the source file. This is
done with a Lit mangler.

Create a file in `chromium_src` for `path/to/your/file.html.ts` at `chromium_src/path/to/your/file.html.ts`

```ts
import mangle from 'lit-mangler'

mangle(element => {
  element.textContent = element.textContent.replaceAll("Chrome", "Brave")
}, literal => literal.text.includes("id='thing-i-want-to-edit'"))
```

The `mangle` function extracts all templates from the `.html.ts` file and loads
them into an HTMLElement, so you can manipulate them using the DOM APIs.

The second argument to this function is optional but lets you select what
template you want to edit (in the case of multiple nested templates).

For example:
```ts
// this template literal from upstream:
html`<div class="container">
  ${this.children.map(c => html`<li class="chrome-item">${c.text}</li>`)}
</div>`

// would have two templates
// 1. For the container
// 2. For the children, iterated over the array.


// in our mangler:
// We'll make the following changes with a Lit Mangler:
// 1. Add a `.brave` class to the `.container` element
// 2. Add an `id` attribute to the `li` elements
// 3. Wrap the child text in a span

import mangle from 'lit-mangler'

mangle(e => {
  const container = e.querySelector('.container')

  // Its good practise to throw an error when the mangler can't find something
  // - it will cause the build to fail, so we'll know quickly that something
  // went wrong.
  if (!container) throw new Error("Couldn't find the container")
// Note: We use this predicate to select the right template.
}, t => t.text.includes('class="container"'))

mangle(e => {
  // Note: The element passed to the mangler is a document fragment, as there
  // could be multiple root elements in a template.
  const li = e.querySelector('li')

  // Assuming our children have an id element. Note: We set this to a string
  // literal, because we're editing the source text of the .html.ts file.
  li.setAttribute('id', '${c.id}');

  // Similarly, we need to escape the template interpolation here, because we
  // want it to happen at runtime, not now!
  // If we mess something up we'll get a typescript error (as the mangled source
  // file still runs through the type checker)!
  li.innerHTML = `<span>
    \${c.text}
  </span>`
// select the list items
}, t => t.text.startsWith('<li'))
```

These overrides have an automatically generated test which checks to see whether
the mangler still applies. To generate (or update the test) run
`npm run test-unit -- "mangled files should have up to date snapshots" -u`.

If the test fails it indicates that upstream has changed and we should check the
override still applies. If it does, then it is safe to update the snapshot.

## Polymer Template Modifications

Polymer templates are just HTML Template elements, so you can easily modify them
with a `chromium_src` override (for the JS file) or more easily via the
`polymer_overriding` utils in `//brave/ui/webui/resources/polymer_overriding.ts`

## Adding New Files

Often, you won't only be changing upstream elements but will also need to add
new elements to the page to get things looking right.

In this scenario, its best to add a new build target to Brave and patch it into
the upstream build for that WebUI.

See `//brave/browser/resources/settings/BUILD.gn` and
`//brave/browser/resources/settings/settings.gni` for how to get this setup.

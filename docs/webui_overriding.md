# Overriding Upstream WebUIs

Currently, there are a number of ways to override upstream WebUIs. Which one you
want will depend on what you're trying to do, and how the upstream WebUI is
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

## Lit Functions

Adding a new function to a Lit component can be complicated because the types
for the HTML template depend on what's exported, so naively super classing the
upstream component does not work. The best approach is to use
[declaration merging](https://www.typescriptlang.org/docs/handbook/declaration-merging.html)
and modify the prototype of the upstream class.

```ts
import { FancyElement } from './fancy-chromium.js'

declare module './fancy-chromium.js' {
  interface FancyElement {
    isBraveAndFancy: () => boolean
  }
}

FancyElement.prototype.isBraveAndFancy = () => true

export * from './fancy-chromium.js'
```

See [this PR](https://github.com/brave/brave-core/pull/29598/files) for a real
example.

## Lit Properties

Similarly to the above, adding a new property to a Lit component isn't
straightforward. The easiest way requires patching out the
`customElements.define` call in the upstream component and replacing the
component:

```ts
import { FancyElement as FancyElementChromium } from './fancy-chromium.js'

declare module './fancy-chromium.js' {
  // Note: This one is still `FancyElement` to get declaration merging to work.
  // This is required so that the `getHtml` function gets the correct `this` type.
  interface FancyElement {
    isBraveAndFancy: boolean
  }
}

// Override the upstream element
class FancyElement extends FancyElementChromium {
  static override get properties() {
    return {
      ...super.properties,
      // Make sure we mark the property as reactive
      isBraveAndFancy: { type: Boolean },
    }
  }

  // Note: Even though the accessor isn't overriding something from
  // upstream we need to mark it as overriding because in our
  // declaration merge we've said that this property exists there.
  override accessor isBraveAndFancy: boolean = true
}

export { FancyElement }
export * from './fancy-chromium.js'

// Define the element as our element. Note: You'll need to patch this
// line out of the upstream file.
customElements.define(FancyElement.is, FancyElement)
```

## Lit Mangling

Unfortunately, the above strategies don't work for modifying Lit HTML. To modify
a Lit HTML template we need a way to directly modify the source file. This is
done with a Lit mangler.

A Lit mangler is a [`plaster`](./plaster.md) backend: it generates a committed
patch that is applied to the upstream source at `npm run sync`, just like any
other patch. You author the mangler, run a command to (re)generate the patch,
and commit both.

Create a file next to the other `rewrite/` plasters, mirroring the upstream
path. For `path/to/your/file.html.ts` that is
`rewrite/path/to/your/file.html.ts.lit_mangler.ts`:

```ts
import { mangle } from 'lit_mangler'

mangle(
  (element) => {
    element.textContent = element.textContent.replaceAll('Chrome', 'Brave')
  },
  (literal) => literal.text.includes("id='thing-i-want-to-edit'"),
)
```

Then generate the patch:

```sh
# Regenerates patches/<dashed-source-path>.patch from the mangler.
tools/cr/plaster.py apply

# Dry-run: fails if the committed patch is out of date with the mangler.
tools/cr/plaster.py check
```

Commit the `rewrite/*.lit_mangler.ts` file together with the generated
`patches/*.patch`. A source may have at most one generator — you cannot have
both a `.yaml` plaster and a `.lit_mangler.ts` targeting the same file.

The `mangle` function extracts all templates from the `.html.ts` file and loads
them into an HTMLElement, so you can manipulate them using the DOM APIs.

The second argument to this function is optional but lets you select what
template you want to edit (in the case of multiple nested templates).

For example:

```ts
// this template literal from upstream:
html`<div class="container">
  ${this.children.map((c) => html`<li class="chrome-item">${c.text}</li>`)}
</div>`

// would have two templates
// 1. For the container
// 2. For the children, iterated over the array.

// in our mangler:
// We'll make the following changes with a Lit Mangler:
// 1. Add a `.brave` class to the `.container` element
// 2. Add an `id` attribute to the `li` elements
// 3. Wrap the child text in a span

import { mangle } from 'lit_mangler'

mangle(
  (e) => {
    const container = e.querySelector('.container')

    // Its good practise to throw an error when the mangler can't find something
    // - it will cause the build to fail, so we'll know quickly that something
    // went wrong.
    if (!container) throw new Error("Couldn't find the container")
    // Note: We use this predicate to select the right template.
  },
  (t) => t.text.includes('class="container"'),
)

mangle(
  (e) => {
    // Note: The element passed to the mangler is a document fragment, as there
    // could be multiple root elements in a template.
    const li = e.querySelector('li')

    // Assuming our children have an id element. Note: We set this to a string
    // literal, because we're editing the source text of the .html.ts file.
    li.setAttribute('id', '${c.id}')

    // Similarly, we need to escape the template interpolation here, because we
    // want it to happen at runtime, not now!
    // If we mess something up we'll get a typescript error (as the mangled source
    // file still runs through the type checker)!
    li.innerHTML = `<span>
    \${c.text}
  </span>`
    // select the list items
  },
  (t) => t.text.startsWith('<li'),
)
```

### Keeping manglers up to date

Because the mangler produces a committed patch, staleness is caught the same way
as for any plaster:

- `tools/cr/plaster.py check` (run in presubmit via `CheckPlasterFiles`) fails
  if the committed patch no longer matches what the mangler produces.
- `npm run update_patches` will not clobber a mangler-owned patch; it errors and
  tells you to run `tools/cr/plaster.py apply` if the patch is out of date.

If `check` fails after a Chromium bump, upstream has changed. Confirm the
override still makes sense, then run `tools/cr/plaster.py apply` to regenerate
the patch and commit the result.

### Behavioral note: manglers run on the raw upstream source

The mangler now transforms the **raw** upstream `.html.ts` (with its copyright
header present and grit directives such as `<if expr>` / `<include>`
unresolved), and the resulting patch is applied before grit preprocessing runs
at build time. This differs from the old build-time mangler, which ran on the
already-preprocessed output.

Two consequences to be aware of:

- Avoid mangling inside an `<if expr>` / `<include>` region. The transform
  round-trips the template through the DOM, and reserializing an unresolved grit
  directive can corrupt it. Target elements outside those regions.
- The DOM round-trip reformats the whole template it touches (collapsing
  multi-line attributes, dropping the leading copyright comment). The generated
  patch is therefore larger than the logical change and may need regenerating
  after upstream whitespace changes. This is expected — the patch is
  machine-generated, never hand-edited; just re-run `tools/cr/plaster.py apply`.

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

## Strings

When using translated strings you should follow the guidance in the
[webui strings explainer](./webui_strings_explainer.md) to reduce boilerplate
and ensure we catch misspelt strings.

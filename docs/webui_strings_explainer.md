## WebUI Strings

We have extended grit with the ability to automatically generate front end
bindings for grit strings, and an associated Typescript enum.

To expose a string to WebUI add a `formatter_data="webui=MyUi"` attribute
in grit:

In `my_strings.grdp`
```diff
- <message name="IDS_MY_UI_MESSAGE_NAME">
+ <message name="IDS_MY_UI_MESSAGE_NAME" formatter_data="webui=MyUi">
  Message name
</message>
```

You can add the strings in your webUI via the generated bindings file. For
`brave_components_strings` this is `components/grit/brave_components_webui_strings.h`

In `my_ui.cc`

```diff
+ #include "components/grit/brave_components_webui_strings.h"

MyUi::MyUi(WebUI* ui) {
  content::WebUIDataSource* source = ...;
- source->AddLocalizedString("messageName", IDS_MY_UI_MESSAGE_NAME);
+ source->AddLocalizedStrings(webui::kMyUiStrings);
}
```

In `my_component.tsx`

```diff
+ import { MyUiStrings } from 'gen/components/grit/brave_components_webui_strings'

export default function Component() {
  return <div>
-   {getLocale('messageName')}
+   {getLocale(MyUiStrings.MY_UI_MESSAGE_NAME)}
  </div>
}
```

By convention (and so we can do less typing) we generally alias the
`MyUiStrings` enum to be in a global enum `S` in the WebUI. There's a bit of
boilerplate here which is an unfortunate side effect of grits "one output per
type" design. This only needs to be done once per WebUI.

1. Create a new file called `strings.ts` in your webuis resources directory.
    ```ts
    import '$web-common/strings'

    import { MyUiStrings } from 'gen/components/grit/brave_components_webui_strings'

    declare global {
      interface Strings {
        MyUiStrings: typeof MyUiStrings
      }
    }
    ```

2. Import the `strings.ts` file from the entry point to your UI

    `my_ui.tsx`

    ```diff
    + import './strings'
    ```

3. Now, you can use the enum from any file on the UI without importing it.

   `my_component.tsx`

    ```diff
    export default function Component() {
      return <div>
    -   {getLocale('messageName')}
    +   {getLocale(S.MY_UI_MESSAGE_NAME)}
      </div>
    }
    ```

For an example PR converting a large number of strings in AIChat see:
https://github.com/brave/brave-core/pull/29505

## Upstream UIs

This system should work for upstream UIs too - unfortunately getting it working
is slightly hacky - you need to add the generated strings file (i.e.
`gen/component/grit/brave_components_webui_strings.ts`) to the list of mojo
files, as that's the only mechanism upstream provides for including generated
files in its webui build process.

Afterwards everything should work the same as it does in our UIs.

## Storybook

Storybook will automatically have the generated strings available.

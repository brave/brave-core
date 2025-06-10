## WebUI Strings

We have extended grit with the ability to automatically generate front end
bindings for grit strings, and an associated Typescript enum.

To expose a string to WebUI add a `formatter_data="webui=YourUi"` attribute
in grit:

In `your_strings.grdp`
```diff
- <message name="IDS_YOUR_UI_MESSAGE_NAME">
+ <message name="IDS_YOUR_UI_MESSAGE_NAME" formatter_data="webui=YourUi">
  Message name
</message>
```

You can add the strings in your webUI via the generated bindings file. For
`brave_components_strings` this is `components/grit/brave_components_webui_strings.h`

In `your_ui.cc`

```diff
+ #include "components/grit/brave_components_webui_strings.h"

YourUI::YourUI(WebUI* ui) {
  content::WebUIDataSource* source = ...;
- source->AddLocalizedString("messageName", IDS_YOUR_UI_MESSAGE_NAME);
+ source->AddLocalizedStrings(webui::kYourUiStrings);
}
```

In `your_component.tsx`

```diff
+ import { YourUiStrings } from 'gen/components/grit/brave_components_webui_strings'

export default function Component() {
  return <div>
-   {getLocale('messageName')}
+   {getLocale(YourUi.YOUR_UI_MESSAGE_NAME)}
  </div>
}
```

By convention (and so we can do less typing) we generally alias the
`YourUiStrings` enum to be in a global enum `S` in the WebUI. There's a bit of
boilerplate here which is an unfortunate side effect of grits "one output per
type" design. This only needs to be done once per WebUI.

1. Create a new file called `strings.ts` in your webuis resources directory.
    ```ts
    import { YourUiStrings } from 'gen/components/grit/brave_components_webui_strings'

    declare global {
      const S: typeof YourUiStrings
    }
    ```

2. Import the `strings.ts` file from the entry point to your UI

    `your_ui.tsx`

    ```diff
    + import './strings'
    ```

3. Now, you can use the enum from any file on the UI without importing it.

   `your_component.tsx`

    ```diff
    export default function Component() {
      return <div>
    -   {getLocale('messageName')}
    +   {getLocale(S.YOUR_UI_MESSAGE_NAME)}
      </div>
    }
    ```

## Storybook

We're looking into generating Storybook strings too, but that will be done as a
followup.

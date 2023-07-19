# Education-Enhanced Pages

Certain pages at `brave.com` can be enhanced by the browser to provide the user
with the ability to trigger in-product help or other education-related actions
from within the webpage itself. For example, the "What's New" page at
`brave.com` can be enhanced to provide links directly to the Settings page
(instead of relying on the user to copy and paste a `brave:` URL).

When initiated, the Brave education page enhancer will perform the following
actions:

* For any elements in the document that match the CSS class
`brave-education-ui`:
  * Read the value of the `data-brave-education-action` attribute.
  * Parse the value as JSON.
  * If the parsed data is valid, then:
    * If the parsed data contains a `trigger` property, then perform a
    `querySelector` on the element, using the value of the `trigger` property as
    the selector.
    * If there is a matching element, then attach an `"onclick"` event handler
    that will send an education request to the browser.
  * Otherwise:
    * Attach an `"onclick"` event handler that will send an education request to
    the browser.
  * Add the `brave-education-active` CSS class to the element.

The page enhancer is initiated by dispatching the event
`brave-education-content-ready` to the `window` object.

Example: Adding a button that opens the settings page in a new tab.

```html

<style>

  /* Hide all enhanced UI elements by default */
  .brave-education-ui {
    display: none;
  }

  /* Show supported and active enhanced UI elements */
  .brave-education-ui.brave-education-active {
    display: initial;
  }

</style>

<button
    class='brave-education-ui'
    data-brave-education-action=
      '{"action": "show-settings-page", "url": "appearance"}'>
  Open Settings Page
</button>

<script>

  // When elements are attached to the document, initiate the page enhancer.
  window.dispatchEvent(new Event('brave-education-content-ready'))

</script>

```

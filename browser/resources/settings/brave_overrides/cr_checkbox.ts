import { RegisterStyleOverride, RegisterPolymerPrototypeModification, RegisterPolymerTemplateModifications } from 'chrome://resources/brave/polymer_overriding.js'
import { html } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import 'chrome://resources/brave/leo.bundle.js'

RegisterStyleOverride('cr-checkbox', html`
  <style>
  </style>
`)

RegisterPolymerTemplateModifications({
  'cr-checkbox': (template: HTMLTemplateElement) => {
    template.querySelector('#checkbox')
      ?.replaceWith(html`<leo-checkbox
        id="checkbox"
        size="small"
        checked="{{checked}}"
        on-change="nalaChange_"
        disabled="[[disabled]]">
        <slot></slot>
      </leo-checkbox>`.content)
  }
})

RegisterPolymerPrototypeModification({
  'cr-checkbox': prototype => {
    // We wrap the onChange_ function so we can emit the same event as the CrToggle
    prototype.nalaChange_ = function (e: CustomEvent<{ checked: boolean }>) {
      prototype.onChange_.apply(this, [new CustomEvent(e.type, { detail: e.detail.checked })])
    }
  }
})

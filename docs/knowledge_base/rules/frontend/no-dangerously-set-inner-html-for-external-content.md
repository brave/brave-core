---
name: brave/no-dangerously-set-inner-html-for-external-content
description:
  Never render HTML from external or untrusted sources with
  dangerouslySetInnerHTML; parse the data and render known safe elements with
  React components.
rubric: security
severity: blocking
---

**Rule.** NEVER pass HTML that comes from an external or untrusted source to
`dangerouslySetInnerHTML`. Parse the data and render the known safe elements
with React components. If rendering raw HTML is unavoidable, sanitize it first
and request a security review.

**Why.** `dangerouslySetInnerHTML` inserts the markup as is, scripts and event
handler attributes included, so untrusted HTML becomes a cross-site scripting
hole in the page that renders it.

## Examples

Bad:

```tsx
<div dangerouslySetInnerHTML={{ __html: article.html }} />
```

Good:

```tsx
<div>
  {article.paragraphs.map((text, i) => (
    <p key={i}>{text}</p>
  ))}
</div>
```

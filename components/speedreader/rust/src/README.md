# Brave SpeedReader

**Private Prototype Repository**

This is the beginning of a prototype SpeedReader implementation that is intended to work across different environments of Brave.

At a high level, SpeedReader:

- Distills text-focused document content from a suitable HTML
- Works on HTML documents before rendering them
- Generates HTML output with no external styling or scripting
- Content styled with Brave-designed themes

## Structure

SpeedReader comes in two distinct components: classifier and mapper. The former decides whether reader mode transformation is applicable to the HTML document, and the latter performs the transformation.

---

**C++ PoC version**: there is a PoC version of the feature extraction and
classifier in C++ in the [archive folder](./_cpp_archive).

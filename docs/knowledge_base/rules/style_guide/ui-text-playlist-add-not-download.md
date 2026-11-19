---
name: brave/ui-text-playlist-add-not-download
description:
  Brave Playlist text says media is "added" to a playlist, never "downloaded" or
  "saved", and never claims it works for "any" video or audio.
rubric: readability
severity: should_fix
---

**Rule.** In Playlist strings, say that files are "added" to a playlist, and
that added media is ad-free. Do not say "download" or "save", and do not claim
Playlist works for "any" video or audio.

**Why.** Playlist does not work on every site, and "download" implies a file
users can take elsewhere, which is not what Playlist offers.

## Examples

Bad:

```xml
<message name="IDS_PLAYLIST_ADD_BUTTON" desc="Button that adds the current video to a playlist">
  Download any video to watch offline
</message>
```

Good:

```xml
<message name="IDS_PLAYLIST_ADD_BUTTON" desc="Button that adds the current video to a playlist">
  Add to playlist
</message>
```

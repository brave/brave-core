# Playlist: network-based media detection (V2)

Playlist saves media from web pages. The original implementation (V1) detects
media by injecting a JS detector script that scrapes `<video>`/`<audio>`
elements out of the DOM. V2 replaces that with network-layer observation and
adds real HLS/DASH support. Both implementations ship in the same binary,
selected by the `kPlaylistServiceV2` feature flag
(`components/playlist/core/common/features.h`), off by default. With the flag
off, V1 is unchanged.

This document explains the V2 design so a first-time reviewer can follow the
commit sequence without reconstructing the architecture from the diff.

## Why V1 is being replaced

- Site-specific detectors are spliced into the base script by **literal string
  search-and-replace** on `const siteSpecificDetector = null`
  (`media_detector_component_manager.cc`); a minifier change silently breaks
  this to a `LOG(ERROR)`.
- Detection polls the DOM for 20s and then stops; media that appears later is
  missed, and anything that isn't a `<video>`/`<audio>` element is invisible.
- **MSE has no real support.** The only strategy is deleting
  `window.MediaSource` in an offscreen WebContents and hoping the site falls
  back to a plain progressive file.
- **Desktop has zero HLS/DASH support.** A `.m3u8` is saved as a single text
  file, which doesn't play.

## Design

### Detect: observe the network, don't scrape the DOM

`BraveProxyingURLLoaderFactory` is the one browser-process seam that sees every
subresource response, across every URLLoaderFactory type. V2 hooks
`ContinueToResponseStarted` there
([`browser/net/brave_proxying_url_loader_factory.cc`](../browser/net/brave_proxying_url_loader_factory.cc))
to classify each response by URL/MIME type/`network::mojom::RequestDestination`
(`MediaStreamClassifier`,
[`components/playlist/content/browser/media_stream_classifier.h`](../components/playlist/content/browser/media_stream_classifier.h))
and forward a POD observation to a per-`BrowserContext`
`PlaylistNetworkObserver`
([`playlist_network_observer.h`](../components/playlist/content/browser/playlist_network_observer.h)).
This only reads response headers already in hand — no response body is touched,
so there's no data-pipe involvement and no extra buffering on the hot path.

Manifest URLs are deduplicated by full URL; segment URLs are deduplicated by
origin (there can be thousands of segments per stream, and only knowing "this
origin serves segments" is what downstream logic needs).

### Metadata: MediaSession, JS as a fallback

`services/media_session/public/mojom` already exposes title, artist, artwork and
duration with no injected script (`PlaylistMediaSessionObserver`,
[`playlist_media_session_observer.h`](../components/playlist/content/browser/playlist_media_session_observer.h)).
Sites that don't implement MediaSession still fall back to the existing V1
detector JS for metadata only.

### Join: per-tab detector

`PlaylistNetworkMediaDetector`
([`playlist_network_media_detector.h`](../components/playlist/content/browser/playlist_network_media_detector.h))
is a `WebContentsUserData` that subscribes to both the network observer and the
MediaSession observer for its tab, debounces on new media or new metadata (sites
often emit hundreds of segment responses in a burst, and MediaSession metadata
frequently arrives _after_ the first media byte), and emits through the
**existing** detector callback signature
(`void(GURL, std::vector<mojom::PlaylistItemPtr>)`). Nothing downstream of that
callback — `PlaylistTabHelper`, `PlaylistService` — needed to change.

### Download: repackage instead of mux

DASH (and separated-track HLS) deliver audio and video as different files. The
natural instinct is to mux them into one output file, but that doesn't work
here:

- Chromium's bundled ffmpeg is decode-only (`CONFIG_MUXERS 0`).
- `media/muxers/Mp4Muxer` exists for MediaRecorder output and has no
  composition-time-offset support, so B-frame-coded video (which is what real
  sites serve) would play back in the wrong order.

Instead, Chromium already ships a full HLS **demuxer**
(`enable_hls_demuxer = proprietary_codecs`, which Brave sets), and it can play
audio and video **from separate files** via `EXT-X-MEDIA` rendition groups. So
V2 never re-encodes or muxes: it downloads segments as-is and writes a local
`.m3u8` that points at them.

| Source                   | Stored as                                                                                           | Played via           |
| ------------------------ | --------------------------------------------------------------------------------------------------- | -------------------- |
| Progressive mp4/webm/mp3 | verbatim file (V1 behavior, unchanged)                                                              | `FFmpegDemuxer`      |
| HLS (TS or fMP4)         | segments verbatim + a rewritten local manifest                                                      | built-in HLS demuxer |
| DASH / separated A+V     | per-track segments concatenated + a synthesized multivariant manifest with an audio rendition group | built-in HLS demuxer |

Concretely:

- `PlaylistStreamDownloader`
  ([`playlist_stream_downloader.h`](../components/playlist/content/browser/playlist_stream_downloader.h))
  fetches the top-level manifest, sniffs it to decide HLS vs. DASH, and
  downloads segments concurrently with `SimpleURLLoader::DownloadToFile`.
  Encrypted streams (HLS `#EXT-X-KEY` other than `NONE`, DASH
  `ContentProtection`) and byte-range-addressed segments are refused rather than
  silently producing undecryptable or malformed output.
- `PlaylistDashParser`
  ([`playlist_dash_parser.h`](../components/playlist/content/browser/playlist_dash_parser.h))
  parses the MPD. MPDs are untrusted XML, so parsing runs out-of-process via
  `services/data_decoder`, never an in-process XML parser.
- `PlaylistManifestWriter`
  ([`playlist_manifest_writer.h`](../components/playlist/content/browser/playlist_manifest_writer.h))
  writes the local HLS manifest(s) that Chromium's demuxer will actually play.
- `PlaylistMediaFileDownloadManager` dispatches to this stream path or to the
  existing single-file `PlaylistMediaFileDownloader`, keyed on whether
  `MediaStreamClassifier` identified the item as a manifest.
- `PlaylistService` tags the resulting item with `hls_media_path` (an existing
  field in `playlist.mojom`, previously Android-only; V2 populates it on desktop
  too) to tell playback it must load a manifest, not a plain file.

### Playback: serve the local manifest

`PlaylistDataSource`
([`browser/playlist/playlist_data_source.cc`](../browser/playlist/playlist_data_source.cc))
gained a `kHls` route: `chrome-untrusted://playlist-data/<item id>/hls/<file>`,
path-traversal-checked against the item's directory, with range-request support
for segments. The player UI's CSP already allows
`media-src ... chrome-untrusted://playlist-data`, so no CSP or CORS change was
needed — saved streams just need to be played from the player UI, which is where
playback already happens.

## What's still V1-only

- **YouTube.** It drives MSE from `adaptiveFormats` without ever serving an MPD,
  so generic DASH support doesn't reach it; V2 keeps the JS detector for
  `youtube.com` (`IsYoutubeLegacyPlaylistSite` in
  [`playlist_constants.h`](../components/playlist/content/browser/playlist_constants.h)).
- **HLS AES-128** and **Android** are out of scope for the current phase; see
  the plan for follow-up phases.

## Commit sequence

The implementation lands as one file/concept per commit, in dependency order,
each with its own BUILD.gn/DEPS entries and unit test where applicable:

1. Feature flag
2. `MediaStreamClassifier`
3. `PlaylistNetworkObserver` + the proxy hook
4. `PlaylistMediaSessionObserver`
5. `PlaylistNetworkMediaDetector` (joins 3 and 4)
6. `PlaylistManifestWriter`
7. `PlaylistDashParser`
8. `PlaylistStreamDownloader` + download-manager/service wiring
9. `PlaylistDataSource` `kHls` route
10. Wiring V2 detection into `PlaylistBackgroundWebContentsHelper`, behind the
    flag
11. HLS/DASH playback browser test and fixtures

Steps 2-9 are additive and compile standalone without changing behavior; step 10
is the only commit that changes what runs, and only when `kPlaylistServiceV2` is
enabled.

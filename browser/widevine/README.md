# Widevine in Brave

Widevine is used to decrypt DRM-protected content, which is served from
streaming services such as Netflix. Widevine is integrated in Chromium as a
component.

## Signature files (brave.exe.sig, chrome.dll.sig, ...)

Streaming services only offer high definition content to clients that are
trusted. Widevine has mechanisms to ensure the integrity of the client. One of
these mechanisms are .sig files. They prove to Widevine that the browser has not
been tampered with. In order for Brave's users to see high-definition content,
the browser must generate and ship with those .sig files.

## Licensing

Brave's licensing agreement for Widevine forbids distribution of Widevine's
binaries. This entails several workarounds, some of which are listed below.

## Workarounds

### Sequential component updates: SequentialUpdateChecker

Brave has its own components and thus uses its own component update server. This
server also gets polled by the browser for Widevine. To comply with the
licensing restriction described above, Brave's component update server responds
with a redirect to Google's server in this case. This works as long as a single
update check request only polls for Widevine, and not also for any other
components. We have a special class, `SequentialUpdateChecker`, that makes sure
that this is the case.

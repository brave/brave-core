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

### Widevine in Arm64 Brave on Windows: WIDEVINE_ARM64_DLL_FIX

As of this writing, Google does not offer Arm64 binaries for Widevine on
Windows. When Arm64 Brave polls Google's component update server for Widevine,
then it receives a "noupdate" response, meaning that no version could be found.
In effect, out of the box, Widevine cannot be installed in Arm64 Brave on
Windows.

However, there is a workaround: When we poll for an x64 version of the
component, then we get the necessary base files, plus some x64-specific
binaries. The remaining necessary binaries for Arm64 are available in a Zip file
on Google's servers. By placing them into the
`WidevineCdm\<version>\_platform_specific\win_arm64` directory of the component,
we can obtain a copy of Widevine that works in Arm64 Brave on Windows.

Brave's `WIDEVINE_ARM64_DLL_FIX` implements the above workaround in code. When
an update check for Widevine returns "noupdate", then the workaround repeats the
update check, pretending to be from the `x64` architecture. It installs the x64
version of the component and then additionally downloads the Arm64 zip file from
Google's server. This gives us a working copy of Widevine in Arm64 Brave.

The facts that upstream can be compiled for Arm64 on Windows and that Google
publicly serves Arm64 binaries of Widevine make it seem likely that a native
Widevine component for this architecture will also be available in the future.
For this reason, our workaround is guarded by a build flag. Once the workaround
is no longer required, the build flag makes it very easy to clean up the
associated code.

**Further details about the present implementation of WIDEVINE_ARM64_DLL_FIX:**

The details below were compiled at the time of the initial implementation, to
simplify the associated code review. They may be wholly or partially out of
date by the time you read this.

The first entry point into the implementation is in `update_checker.cc`, in
`SequentialUpdateChecker::UpdateResultAvailable`. It handles the symptom of the
current limitation: The browser asks the component update server "Do you have
the Widevine component?", implicitly sending its architecture, which is Arm64.
The server responds `"noupdate"`, which means there is no such version. When
this happens, `UpdateResultAvailable` repeats the update check with a fake
`x64` architecture.

There is some additional code in `UpdateResultAvailable` that handles the case
when the server responds `noupdate` not because there is no Arm64 version of
Widevine at all, but because there is just no _new_ version. If we ever receive
an Arm64 version of Widevine from the update server, then we disable our
workaround to not fall back to `x64`. This information is persisted via
`SequentialUpdateChecker::SetPersistedFlag` and `...:GetPersistedFlag`.

The fake architecture is passed from `update_checker.cc` to
`protocol_serializer.cc` via the `additional_attributes` parameter of
`MakeProtocolRequest`.

When the browser receives the response from the update server, it installs the
Widevine component. Upstream has some custom logic for this in
`widevine_cdm_component_installer.cc`. We extend this logic to in particular
overwrite `WidevineCdmComponentInstallerPolicy::OnCustomInstall`. This gets
called by upstream when the "normal" installation of the component is complete.
Our additional code downloads and unpacks the external zip file with the
necessary Arm64 binaries. It also applies some patches to make upstream accept
the foreign implementation.

To be able to download the Arm64 binaries, the `OnCustomInstall` method
mentioned above needs a `SharedURLLoaderFactory`. Several sections of our code
make sure that it receives such an instance in our and upstream's calls of
`RegisterWidevineCdmComponent`.

Brave intercepts network requests to certain domains for privacy reasons. One
such domain is `dl.google.com`, which Brave redirects to `redirector.brave.com`.
Our attempt to download the Zip file with additional Arm64 binaries would
normally be prevented by this mechanism. We add an exception to
`brave_static_redirect_network_delegate_helper.cc` so that the Zip file can in
fact be downloaded.

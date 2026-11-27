# Tor credits

The iOS GN defaults enable `generate_about_credits`, including local builds.
`script/brave_license_helper.py` registers the two metadata directories here
with Chromium's standard credits generator. Each component has its own entry
and its own license file under `Resources/Credits`. These files are also
packaged as local Swift package resources; opening credits needs no network.

`embedded/README.chromium` contains separate metadata records for Tor,
Tor.framework, OpenSSL, libevent, and liblzma. `transports/README.chromium`
contains separate records for IPtProxy, Go, Lyrebird, Snowflake, and each
transitive module. Chromium's dependency divider separates the records.
The aggregate notice resources are retained for the native notices viewer,
but the credits page does not reference them or repeat IPtProxy under Tor.

After updating IPtProxy, update the pinned revisions and Go version in
`generate_transport_notices.py`, then run it with Python 3. It downloads only
source manifests, licenses, and module archives; it does not execute them.
Review the generated diff. The inventory intentionally covers the complete
upstream module manifest, including tooling and transports Brave does not
enable, rather than trying to infer binary license coverage from UI choices.
Archive URLs, versions, and SHA-256 hashes are recorded with each module.
No downloads or generation are performed at application runtime.

To regenerate the individual entries from the existing aggregate resources
without network access, run `python3 generate_credit_entries.py`. Pass `--check`
to verify that the entries match those resources. Transport regeneration also
updates these entries automatically. Per-module `LicenseRef` identifiers refer
to the exact upstream license texts, including bundled subsidiary notices.

After rebuilding the native core and application, verify `brave://credits`
shows separate Tor, Tor.framework, OpenSSL, libevent, liblzma, IPtProxy, Go,
Lyrebird, and Snowflake entries. Check that full license text is visible offline.

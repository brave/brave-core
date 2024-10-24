# Updating test browser profiles for perf tests

Profile updating is semi-automated. While CI bots are able to generate new profiles, some actions should be done manually.
The recommended update interval is once a month.

## Important details

* The profiles are stored as zip archives on the cloud storage.
* `brave-core` stores only sha1 hashes of these profiles.
* Profiles have versions. After updating, the old browser versions continue to use the old profiles (using git history to rewind the time). This ensures build reproducibility.

## Launching update CI jobs

* To generate an updated profile, launch the job with `EXTRA_ARGS` = `--mode update-profile --upload-branch <branch>`.
* Chromium and Brave use different profiles, so two builds for each platforms `win`/`mac-arm64`/`android` should be launched. For details, check the configurations in `configs/ci/`.
* For CI URLS check [README.md](./README.md)
* CI builds will upload the updated profiles to the cloud storage and make a PR from the branch.

## Reviewing the PR

* Check all the profiles are updated (currently 6 profiles).
* Review the changes in `.sizes` files.

## Manual (fallback) way

### Preconditions

* Checkout `brave`, run `npm run sync`
* Obtain write access to the cloud storage (ask @devops-team).
* Remove old downloaded profiles: `rm -rf ./profiles/*.zip`

### Step 1: Upload to the cloud storage

* Download `artifacts.zip` (the URL in the Console Output) from all the builds and extract them.
* Each unpacked artifact contains a zip archive with the updated profile (e.g. `brave-typical-win.zip`) and `.size` file (e.g. `brave-typical-win.zip.sizes`). Copy these files to `./profiles/`.
* Upload the new archives to the cloud storage: see [README.md](./README.md)

### Step 2: Commit changes and make a PR

* The `./profiles/*.sha1` should be updated during the upload. Stage and commit changes in `.sha1` and `.size` files.
* DO NOT commit zip files.
* Make a PR to `brave-core`.

## Testing before merge

If you run a perf CI build on the resulting branch, the old profiles will still be used (because of versioning).
To check new profiles before merging the PR, run a custom build with `"profile" : <sha1_profile_hash_to_check>` in the config.

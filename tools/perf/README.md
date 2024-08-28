# Brave performance tests

## Job URLs

* [Window](<https://ci.brave.com/job/brave-browser-test-perf-windows/>)
* [MacOS](<https://ci.brave.com/job/brave-browser-test-perf-macos/>)
* [Android](<https://ci.brave.com/job/brave-browser-test-perf-android/>)

## S3 migration

Perf data is currently moving to AWS S3 `brave-perf-data` bucket.
The data is accessible via <https://perf-data.s3.brave.com/{path}>.
The current structure:
`./perf-profiles/`: test perf profiles
`./telemetry-perf-data/`: WPR files and other data are used by telemetry and
                          catapult code

## S3 manual upload

Make sure that you setup aws cli tools.

1. Calculate SHA1 hash
2. Upload to `s3://brave-perf-data/<folder>/<sha1>`
3. Store the hash as `<filename>.sha1`

A sh command to run all 3 steps:
`FILE=<filename>; \
SHA1=$(shasum -a 1 BUILD.gn | head -c 40); \
  aws s3 cp $FILE s3://brave-perf-data/<folder>/$SHA1 \
    --profile go-translate-dev --acl bucket-owner-full-control --sse AES256 \
  echo $SHA1 > $FILE.sha1;`

## GSC manual upload (legacy)

`upload_to_google_storage.py --bucket=brave-telemetry <file>`
the script will produce `.sha1` automatically.

## How to update or record WPR

Use `npm run perf_tests -- --mode record-wpr` instead of chromium `update_wpr` or `record_wpr`. It:

* downloads and runs both Brave and Chromium, combine .wpr files (to capture all browser-specific requests);
* adds pre-initialied profiles and Griffin/Finch experiments;
* does some pre runs to ensure that everything is initialized (aka online profile rebase)
* removes unwanted URLs from the final .wpr file.

### Workflow

* Prepare a config (use configs/record-wpr.json5 as a base). One benchmark or storySet = one .wpr.
* Run `npm run perf_tests -- <config>.json5 --mode record-wpr --working-directory=.. --variations-repo-dir=..`;
* Run the matching benchmark locally to tests the created .wpr;
* Upload wpr files to the cloud storage: `ls ./brave/tools/perf/page_sets/data/*.wprgo | xargs <upload_cmd>`;
* Commit the changes, including a new `.sha1` files, to brave-core.

## Updating profiles

[Instruction](./updating_test_profiles.md)

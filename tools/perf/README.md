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

## How to update wpr

* `cd src/`;
* Remove old downloaded wprs: `rm -rf ./brave/tools/perf/page_sets/data/*.wprgo`;
* Record new wprs: `vpython3 tools/perf/record_wpr <benchmark_name> --browser=system  --story-filter <story-filter>`;
* Upload the archives to the cloud storage: `ls ./brave/tools/perf/page_sets/data/*.wprgo | xargs <upload_cmd>`.
* `cd brave`;
* Review and commit new `.sha1` files (not `.wprgo`) plus new entries in
  `./brave/tools/perf/page_sets_data/*.json`.

## Updating profiles

[Instruction](./updating_test_profiles.md)

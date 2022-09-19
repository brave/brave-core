### How to update wpr

* Get write access to `brave-telemetry` GCS bucket (ask @devops-team);
* `cd src/`;
* Remove old downloaded wprs: `rm -rf ./brave/tools/perf/page_sets/data/*.wprgo`;
* Record new wprs: `vpython3 tools/perf/record_wpr <benchmark_name> --browser=system  --story-filter <story-filter>`;
* Upload the archives to GCS: `ls ./brave/tools/perf/page_sets/data/*.wprgo | xargs upload_to_google_storage.py --bucket=brave-telemetry`.
  `*.sha1` files will be generated;
* `cd brave`;
* Review and commit new `.sha1` files (not `.wprgo`) plus new entries in
  `./brave/tools/perf/page_sets_data/*.json`.

/*!
Including the milliseconds elapsed since the logger was initialized in each
record, similar to `Log4J`'s relative time pattern.

Before running this example, try setting the `MY_LOG_LEVEL` environment variable to `info`:

```no_run,shell
$ export MY_LOG_LEVEL='info'
```
*/

use std::io::Write;
use std::time::{Duration, Instant};

use env_logger::{Builder, Env};

fn init_logger() {
    let env = Env::default().filter("MY_LOG_LEVEL");

    let start = Instant::now();
    Builder::from_env(env)
        .format(move |buf, record| {
            let elapsed = start.elapsed().as_millis();
            writeln!(
                buf,
                "[{elapsed:>6} ms] {}: {}",
                record.level(),
                record.args()
            )
        })
        .init();
}

fn main() {
    init_logger();

    log::info!("a log from `MyLogger`");
    std::thread::sleep(Duration::from_millis(250));
    log::warn!("another log, a moment later");
}

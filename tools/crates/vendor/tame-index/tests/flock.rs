#![allow(missing_docs)]

use std::time::Duration;
use tame_index::utils::flock::LockOptions;

mod utils;

enum LockKind {
    Exclusive,
    Shared,
}

impl LockKind {
    fn as_str(&self) -> &'static str {
        match self {
            Self::Exclusive => "exclusive",
            Self::Shared => "shared",
        }
    }
}

fn spawn(kind: LockKind, path: &tame_index::Path) -> std::process::Child {
    let mut cmd = std::process::Command::new("cargo");
    cmd.env("RUST_BACKTRACE", "1")
        .args([
            "run",
            "-q",
            "--manifest-path",
            "tests/flock/Cargo.toml",
            "--",
        ])
        .stdout(std::process::Stdio::piped())
        .arg(kind.as_str())
        .arg(path);

    let mut child = cmd.spawn().expect("failed to spawn flock");

    // Wait for the child to actually take the lock
    {
        use std::io::Read;
        let mut output = child.stdout.take().unwrap();

        let mut buff = [0u8; 4];
        output.read_exact(&mut buff).expect("failed to read output");

        assert_eq!(
            '🔒',
            char::from_u32(u32::from_le_bytes(buff)).expect("invalid char")
        );
    }

    child
}

fn kill(mut child: std::process::Child) {
    child.kill().expect("failed to kill child");
    child.wait().expect("failed to wait for child");
}

/// Validates we can take a lock we know is uncontended
#[test]
fn can_take_lock() {
    let td = utils::tempdir();
    let ctl = td.path().join("can-take-lock");

    let lo = LockOptions::new(&ctl).exclusive(false);

    let _lf = lo
        .lock(|_p| unreachable!("lock is uncontested"))
        .expect("failed to acquire lock");
}

/// Validates we can create parent directories for a lock file if they don't exist
#[test]
fn can_take_lock_in_non_existant_directory() {
    let td = utils::tempdir();
    let ctl = td.path().join("sub/dir/can-take-lock");

    let lo = LockOptions::new(&ctl).exclusive(false);

    let _lf = lo.try_lock().expect("failed to acquire lock");
}

/// Validates we can take multiple shared locks of the same file
#[test]
fn can_take_shared_lock() {
    let td = utils::tempdir();
    let ctl = td.path().join("can-take-shared-lock");

    let _ = std::fs::OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(&ctl)
        .expect("failed to create lock file");

    let child = spawn(LockKind::Shared, &ctl);

    let mut lo = LockOptions::new(&ctl);

    lo = lo.shared();
    lo.try_lock().expect("failed to acquire shared lock");

    lo = lo.exclusive(false);
    if lo.try_lock().is_ok() {
        panic!("we acquired an exclusive lock but we shouldn't have been able to");
    }

    kill(child);

    lo.try_lock().expect("failed to acquire exclusive lock");
}

/// Validates we can wait for a lock to be released
#[test]
fn waits_lock() {
    let td = utils::tempdir();
    let ctl = td.path().join("waits-lock");

    let _ = std::fs::OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(&ctl)
        .expect("failed to create lock file");

    let child = spawn(LockKind::Exclusive, &ctl);

    std::thread::scope(|s| {
        s.spawn(|| {
            LockOptions::new(&ctl)
                .lock(|_p| {
                    println!("waiting on lock");
                    Some(Duration::from_millis(200))
                })
                .expect("failed to acquire shared lock");
        });
        s.spawn(|| {
            std::thread::sleep(Duration::from_millis(100));
            kill(child);
        });
    });
}

/// Ensures we can timeout if it takes too long to acquire the lock
#[test]
fn wait_lock_times_out() {
    let td = utils::tempdir();
    let ctl = td.path().join("wait-lock-times-out");

    let _ = std::fs::OpenOptions::new()
        .create(true)
        .truncate(true)
        .write(true)
        .open(&ctl)
        .expect("failed to create lock file");

    let child = spawn(LockKind::Exclusive, &ctl);

    if let Err(err) = LockOptions::new(&ctl).shared().lock(|_p| {
        println!("waiting on lock");
        Some(Duration::from_millis(100))
    }) {
        let tame_index::Error::Lock(le) = err else {
            panic!("unexpected error type {err:#?}");
        };

        assert!(matches!(
            le.source,
            tame_index::utils::flock::LockError::TimedOut
        ));
    } else {
        panic!("we should not be able to take the lock");
    }

    kill(child);
}

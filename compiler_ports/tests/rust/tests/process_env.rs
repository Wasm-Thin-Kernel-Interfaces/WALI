//! Process-level services: environment, working directory, clocks and stdio.

use std::env;
use std::io::Write;
use std::thread;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

#[test]
fn env_vars_round_trip() {
    // SAFETY: single-threaded access to the environment within this test.
    unsafe { env::set_var("WALI_RUST_TEST", "set-from-rust") };
    assert_eq!(env::var("WALI_RUST_TEST").unwrap(), "set-from-rust");
    assert!(env::vars().any(|(k, _)| k == "WALI_RUST_TEST"));

    unsafe { env::remove_var("WALI_RUST_TEST") };
    assert!(env::var("WALI_RUST_TEST").is_err());
}

#[test]
fn args_include_the_test_binary() {
    let args: Vec<String> = env::args().collect();
    assert!(!args.is_empty(), "argv[0] should be present");
    assert!(args[0].contains("process_env"), "unexpected argv[0]: {}", args[0]);
}

#[test]
fn current_dir_is_readable() {
    let cwd = env::current_dir().unwrap();
    assert!(cwd.is_absolute(), "cwd should be absolute: {}", cwd.display());
}

#[test]
fn monotonic_clock_advances_across_sleep() {
    let start = Instant::now();
    thread::sleep(Duration::from_millis(50));
    let elapsed = start.elapsed();
    assert!(elapsed >= Duration::from_millis(40), "elapsed too short: {elapsed:?}");
    assert!(elapsed < Duration::from_secs(10), "elapsed implausibly long: {elapsed:?}");
}

#[test]
fn wall_clock_is_past_the_epoch() {
    let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap();
    // Sanity floor: 2020-01-01. Catches a clock stuck at zero.
    assert!(now.as_secs() > 1_577_836_800, "implausible wall clock: {now:?}");
}

#[test]
fn stdout_and_stderr_are_writable() {
    writeln!(std::io::stdout(), "hello from wali rust (stdout)").unwrap();
    writeln!(std::io::stderr(), "hello from wali rust (stderr)").unwrap();
    std::io::stdout().flush().unwrap();
}

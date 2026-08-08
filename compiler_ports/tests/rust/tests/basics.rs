//! Target configuration and core/alloc functionality that must work before
//! anything else is worth testing.

use std::collections::{BTreeMap, HashMap};

#[test]
fn target_cfg_is_wali() {
    assert!(cfg!(target_arch = "wasm32"), "expected a wasm32 target");
    assert!(cfg!(target_os = "linux"), "expected target_os = linux");
    assert!(cfg!(target_env = "musl"), "expected target_env = musl");
    assert!(cfg!(target_family = "wasm"), "expected target_family = wasm");
    assert_eq!(std::mem::size_of::<usize>(), 4);
}

/// The target spec pins `panic_strategy: abort`; `#[should_panic]` tests are
/// silently ignored as a result, so they are avoided in this suite.
#[test]
fn panic_strategy_is_abort() {
    assert!(cfg!(panic = "abort"), "expected the target to default to panic=abort");
}

#[test]
fn atomics_and_threading_are_enabled() {
    assert!(cfg!(target_feature = "atomics"), "expected the atomics feature");
    assert!(cfg!(target_feature = "bulk-memory"), "expected the bulk-memory feature");
}

#[test]
fn formatting_and_iterators() {
    let sum: u32 = (1..=10).sum();
    assert_eq!(sum, 55);
    assert_eq!(format!("{sum:>5}"), "   55");

    let mut v = vec![5u32, 3, 8, 1, 2];
    v.sort();
    assert_eq!(v, [1, 2, 3, 5, 8]);

    let evens: Vec<u32> = v.iter().copied().filter(|n| n % 2 == 0).collect();
    assert_eq!(evens, [2, 8]);
    assert_eq!(["a", "b", "c"].join("-"), "a-b-c");
}

#[test]
fn collections_and_hashing() {
    let mut map = HashMap::new();
    for (i, word) in ["zero", "one", "two"].iter().enumerate() {
        map.insert(word.to_string(), i);
    }
    assert_eq!(map["two"], 2);
    assert_eq!(map.get("three"), None);

    // HashMap seeds itself from the OS RNG, so this also exercises getrandom.
    let ordered: BTreeMap<_, _> = map.into_iter().collect();
    assert_eq!(ordered.keys().cloned().collect::<Vec<_>>(), ["one", "two", "zero"]);
}

#[test]
fn floats_and_math() {
    let x: f64 = 2.0;
    assert_eq!(x.sqrt().powi(2).round(), 2.0);
    assert!((std::f64::consts::PI.sin()).abs() < 1e-9);
    assert_eq!((-3.5f32).abs(), 3.5);
}

#[test]
fn i128_arithmetic() {
    let big: i128 = i64::MAX as i128 * 3;
    assert_eq!(big / 3, i64::MAX as i128);
    assert_eq!(u64::MAX.checked_add(1), None);
}

//! `std::thread` on top of the Wasm threads proposal: spawn/join, shared state,
//! channels and thread-local storage.

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Barrier, Mutex, RwLock, mpsc};
use std::thread;

#[test]
fn spawn_and_join_returns_values() {
    let handles: Vec<_> = (0..4u32).map(|i| thread::spawn(move || i * 2)).collect();
    let mut results: Vec<u32> = handles.into_iter().map(|h| h.join().unwrap()).collect();
    results.sort();
    assert_eq!(results, [0, 2, 4, 6]);
}

#[test]
fn shared_state_across_threads() {
    let counter = Arc::new(AtomicUsize::new(0));
    let log = Arc::new(Mutex::new(Vec::new()));

    let handles: Vec<_> = (0..4usize)
        .map(|i| {
            let counter = Arc::clone(&counter);
            let log = Arc::clone(&log);
            thread::spawn(move || {
                counter.fetch_add(i + 1, Ordering::SeqCst);
                log.lock().unwrap().push(i);
            })
        })
        .collect();
    for h in handles {
        h.join().unwrap();
    }

    assert_eq!(counter.load(Ordering::SeqCst), 10);
    let mut seen = log.lock().unwrap().clone();
    seen.sort();
    assert_eq!(seen, [0, 1, 2, 3]);
}

#[test]
fn channels_transfer_ownership() {
    let (tx, rx) = mpsc::channel();
    let producer = thread::spawn(move || {
        for i in 0..10 {
            tx.send(format!("msg-{i}")).unwrap();
        }
    });

    let received: Vec<String> = rx.iter().collect();
    producer.join().unwrap();

    assert_eq!(received.len(), 10);
    assert_eq!(received[0], "msg-0");
    assert_eq!(received[9], "msg-9");
}

#[test]
fn barrier_synchronizes_threads() {
    let barrier = Arc::new(Barrier::new(3));
    let stage = Arc::new(RwLock::new(0u32));

    let handles: Vec<_> = (0..3)
        .map(|_| {
            let barrier = Arc::clone(&barrier);
            let stage = Arc::clone(&stage);
            thread::spawn(move || {
                *stage.write().unwrap() += 1;
                barrier.wait();
                *stage.read().unwrap()
            })
        })
        .collect();

    // Every thread observes all three increments once the barrier releases.
    for h in handles {
        assert_eq!(h.join().unwrap(), 3);
    }
}

#[test]
fn thread_local_storage_is_per_thread() {
    thread_local! {
        static SLOT: std::cell::Cell<u32> = const { std::cell::Cell::new(0) };
    }

    SLOT.with(|s| s.set(42));
    let observed = thread::spawn(|| SLOT.with(|s| s.get())).join().unwrap();

    assert_eq!(observed, 0, "thread-local should start fresh in a new thread");
    assert_eq!(SLOT.with(|s| s.get()), 42);
}

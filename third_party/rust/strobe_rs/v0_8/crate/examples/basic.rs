use strobe_rs::{SecParam, Strobe};

fn main() {
    let mut rx = Strobe::new(b"correctnesstest", SecParam::B256);
    let mut tx = Strobe::new(b"correctnesstest", SecParam::B256);

    rx.key(b"the-combination-on-my-luggage", false);
    tx.key(b"the-combination-on-my-luggage", false);

    let mut msg = b"Attack at dawn".to_vec();
    rx.send_enc(msg.as_mut_slice(), false);

    // Rename for clarity. `msg` has been encrypted in-place.
    let mut ciphertext = msg;

    tx.recv_enc(ciphertext.as_mut_slice(), false);

    // And back again.
    let round_trip_msg = ciphertext;

    assert_eq!(&round_trip_msg, b"Attack at dawn");
}

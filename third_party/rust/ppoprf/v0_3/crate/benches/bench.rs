use criterion::{criterion_group, criterion_main, Criterion};

use curve25519_dalek::ristretto::RistrettoPoint;
use rand::{rngs::OsRng, Rng};

use ppoprf::ggm::GGM;
use ppoprf::ppoprf::{Client, Point, Server};
use ppoprf::PPRF;

fn criterion_benchmark(c: &mut Criterion) {
  benchmark_ggm(c);
  benchmark_ppoprf(c);
  benchmark_server(c);
  benchmark_client(c);
}

fn benchmark_ggm(c: &mut Criterion) {
  c.bench_function("GGM setup", |b| {
    b.iter(GGM::setup);
  });

  c.bench_function("GGM eval 1 bit", |b| {
    let ggm = GGM::setup();
    let mut out = vec![0u8; 32];
    let input = b"x";
    b.iter(|| {
      ggm.eval(input, &mut out).unwrap();
    });
  });

  c.bench_function("GGM setup & puncture 1 input", |b| {
    let input = b"x";
    b.iter(|| {
      let mut ggm = GGM::setup();
      ggm.puncture(input).unwrap();
    });
  });

  c.bench_function("GGM setup & puncture all inputs", |b| {
    let inputs: Vec<u8> = (0..=255).collect();
    b.iter(|| {
      let mut ggm = GGM::setup();
      for &x in &inputs {
        ggm.puncture(&[x]).unwrap();
      }
    });
  });
}

fn benchmark_ppoprf(c: &mut Criterion) {
  let mds = vec![0u8];
  let server = Server::new(mds.clone()).unwrap();
  let c_input = b"a_random_client_input";
  c.bench_function("PPOPRF end-to-end evaluation", |b| {
    b.iter(|| {
      end_to_end_evaluation(&server, c_input, 0, false, &mut [0u8; 32])
    });
  });

  c.bench_function("PPOPRF end-to-end evaluation verifiable", |b| {
    b.iter(|| end_to_end_evaluation(&server, c_input, 0, true, &mut [0u8; 32]));
  });

  c.bench_function("PPOPRF setup & puncture 1 input", |b| {
    b.iter(|| {
      let mut server = Server::new(mds.clone()).unwrap();
      server.puncture(mds[0]).unwrap();
    });
  });

  c.bench_function("PPOPRF setup & puncture all inputs", |b| {
    let inputs: Vec<u8> = (0..=255).collect();
    b.iter(|| {
      let mut server = Server::new(inputs.clone()).unwrap();
      for &md in inputs.iter() {
        server.puncture(md).unwrap();
      }
    });
  });
}

fn benchmark_server(c: &mut Criterion) {
  let mds: Vec<u8> = (0..=7).collect();
  c.bench_function("Server setup", |b| {
    b.iter(|| {
      Server::new(mds.clone()).unwrap();
    })
  });

  c.bench_function("Server puncture 1 input", |b| {
    b.iter(|| {
      let mut server = Server::new(mds.clone()).unwrap();
      server.puncture(0u8).unwrap();
    })
  });

  c.bench_function("Server eval", |b| {
    b.iter(|| {
      let server = Server::new(mds.clone()).unwrap();
      let point = Point::from(RistrettoPoint::random(&mut OsRng));
      server.eval(&point, 0, false).unwrap();
    })
  });

  c.bench_function("Server verifiable eval", |b| {
    b.iter(|| {
      let server = Server::new(mds.clone()).unwrap();
      let point = Point::from(RistrettoPoint::random(&mut OsRng));
      server.eval(&point, 0, true).unwrap();
    })
  });
}

fn benchmark_client(c: &mut Criterion) {
  let mds: Vec<u8> = (0..=7).collect();
  let mut input = [0u8; 32];
  OsRng.fill(&mut input);
  let server = Server::new(mds.clone()).unwrap();

  c.bench_function("Client blind", |b| {
    b.iter(|| {
      Client::blind(input.as_ref());
    })
  });

  c.bench_function("Client verify", |b| {
    let (blinded_point, _) = Client::blind(input.as_ref());
    let eval = server.eval(&blinded_point, 0, true).unwrap();
    b.iter(|| {
      Client::verify(&server.get_public_key(), &blinded_point, &eval, 0)
    })
  });

  c.bench_function("Client unblind", |b| {
    let (blinded_point, r) = Client::blind(input.as_ref());
    b.iter(|| {
      Client::unblind(&blinded_point, &r);
    })
  });

  c.bench_function("Client finalize", |b| {
    let random_point = RistrettoPoint::random(&mut OsRng);
    b.iter(|| {
      Client::finalize(
        input.as_ref(),
        mds[0],
        &Point::from(random_point),
        &mut [0u8; 32],
      );
    })
  });
}

// The `end_to_end_evaluation` helper function for performs a full
// PPOPRF protocol evaluation.
fn end_to_end_evaluation(
  server: &Server,
  input: &[u8],
  md: u8,
  verify: bool,
  out: &mut [u8],
) {
  let (blinded_point, r) = Client::blind(input);
  let evaluated = server.eval(&blinded_point, md, verify).unwrap();
  if verify
    && !Client::verify(&server.get_public_key(), &blinded_point, &evaluated, md)
  {
    panic!("Verification failed")
  }
  let unblinded = Client::unblind(&evaluated.output, &r);
  Client::finalize(input, md, &unblinded, out);
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);

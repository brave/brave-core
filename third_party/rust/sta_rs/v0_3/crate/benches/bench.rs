use core::iter;
use criterion::{
  criterion_group, criterion_main, Criterion, PlotConfiguration,
};
use rand::Rng;

#[cfg(feature = "star2")]
use ppoprf::ppoprf::Server as PPOPRFServer;
use sta_rs::{AssociatedData, Message};
use star_test_utils::*;

fn criterion_benchmark(c: &mut Criterion) {
  benchmark_client_randomness_sampling(c);
  benchmark_client_triple_generation(c);
  benchmark_server_retrieval(c);
  benchmark_end_to_end(c);
}

fn benchmark_client_randomness_sampling(c: &mut Criterion) {
  c.bench_function("Client local randomness", |b| {
    let client = client_zipf(10000, 1.03, 2, b"t");
    let mut out = vec![0u8; 32];
    b.iter(|| {
      client.sample_local_randomness(&mut out);
    });
  });

  #[cfg(feature = "star2")]
  c.bench_function("Client ppoprf randomness", |b| {
    let client = client_zipf(10000, 1.03, 2, b"t");
    let ppoprf_server = PPOPRFServer::new(&[b"t".to_vec()]);
    let mut out = vec![0u8; 32];
    b.iter(|| {
      client.sample_oprf_randomness(&ppoprf_server, &mut out);
    });
  });
}

fn benchmark_client_triple_generation(c: &mut Criterion) {
  c.bench_function("Client generate triple (local)", |b| {
    let mg = client_zipf(10000, 1.03, 2, b"t");
    let mut rnd = [0u8; 32];
    mg.sample_local_randomness(&mut rnd);
    b.iter(|| Message::generate(&mg, &rnd, None).unwrap());
  });

  #[cfg(feature = "star2")]
  c.bench_function("Client generate triple (ppoprf)", |b| {
    let mg = client_zipf(10000, 1.03, 2, b"t");
    let ppoprf_server = PPOPRFServer::new(&[b"t".to_vec()]);
    let mut rnd = [0u8; 32];
    mg.sample_oprf_randomness(ppoprf_server, &mut rnd);
    b.iter(|| {
      Message::generate(&mg, &rnd, None).unwrap();
    });
  });

  c.bench_function("Client generate triple (local, aux)", |b| {
    let random_bytes = rand::thread_rng().gen::<[u8; 32]>();
    let mg = client_zipf(10000, 1.03, 2, b"t");
    let mut rnd = [0u8; 32];
    mg.sample_local_randomness(&mut rnd);
    b.iter(|| {
      Message::generate(&mg, &rnd, Some(AssociatedData::new(&random_bytes)))
        .unwrap();
    });
  });

  #[cfg(feature = "star2")]
  c.bench_function("Client generate triple (ppoprf, aux)", |b| {
    let random_bytes = rand::thread_rng().gen::<[u8; 32]>();
    let mg = client_zipf(10000, 1.03, 2, b"t");
    let ppoprf_server = PPOPRFServer::new(&[b"t".to_vec()]);
    let mut rnd = [0u8; 32];
    mg.sample_oprf_randomness(ppoprf_server, &mut rnd);
    b.iter(|| {
      Message::generate(&mg, &rnd, Some(AssociatedData::new(&random_bytes)))
        .unwrap();
    });
  });
}

fn benchmark_server_retrieval(c: &mut Criterion) {
  let mg = client_zipf(10000, 1.03, 50, b"t");
  let mut rnd = [0u8; 32];
  mg.sample_local_randomness(&mut rnd);
  let messages: Vec<Message> =
    iter::repeat_with(|| Message::generate(&mg, &rnd, None).unwrap())
      .take(1000)
      .collect();
  c.bench_function("Server retrieve outputs", |b| {
    let agg_server = AggregationServer::new(50, "t");
    b.iter(|| {
      let _o = agg_server.retrieve_outputs(&messages);
    });
  });
}

struct Params {
  n: usize,
  s: f64,
  clients: usize,
  threshold: u32,
  local: bool,
  aux_data: bool,
}

fn benchmark_end_to_end(c: &mut Criterion) {
  let plot_config = PlotConfiguration::default();
  let mut group = c.benchmark_group("end-to-end");
  group.plot_config(plot_config);
  group.sample_size(10);
  [
        Params { n: 10000, s: 1.03, clients: 100000, threshold: 100, local: true, aux_data: false },
        Params { n: 10000, s: 1.03, clients: 250000, threshold: 250, local: true, aux_data: false },
        Params { n: 10000, s: 1.03, clients: 500000, threshold: 500, local: true, aux_data: false },
        Params { n: 10000, s: 1.03, clients: 1000000, threshold: 1000, local: true, aux_data: false },
    ].iter().for_each(|params| {
        let epoch = "t";
        let messages = get_messages(params, epoch);
        group.bench_function(&format!("E2E server (n={}, s={}, clients={}, threshold={}, local_randomness={}, aux_data={})", params.n, params.s, params.clients, params.threshold, params.local, params.aux_data), |b| {
            let agg_server = AggregationServer::new(params.threshold, epoch);
            b.iter(|| {
                let _o = agg_server.retrieve_outputs(&messages);
            });
        });
    });
}

fn get_messages(params: &Params, epoch: &str) -> Vec<Message> {
  let mg = client_zipf(params.n, params.s, params.threshold, epoch.as_bytes());
  let mut rnd = [0u8; 32];
  if params.local {
    iter::repeat_with(|| {
      Message::generate(&mg, &rnd, get_aux_data(params.aux_data)).unwrap()
    })
    .take(params.clients)
    .collect()
  } else {
    mg.sample_local_randomness(&mut rnd);
    #[cfg(not(feature = "star2"))]
    unimplemented!();
    #[cfg(feature = "star2")]
    {
      let mut ppoprf_server = PPOPRFServer::new(&[b"t".to_vec()]);
      let messages = iter::repeat_with(|| {
        sample_oprf_randomness(&ppoprf_server, &mut rnd);
        Message::generate(&mg, &rnd, get_aux_data(params.aux_data)).unwrap()
      })
      .take(params.clients)
      .collect();
      ppoprf_server.puncture(epoch.as_bytes());
      messages
    }
  }
}

fn get_aux_data(do_it: bool) -> Option<AssociatedData> {
  if do_it {
    return Some(AssociatedData::new(&rand::thread_rng().gen::<[u8; 8]>()));
  }
  None
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);

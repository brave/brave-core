#[cfg(feature = "rand_distr")]
mod tests {
    use float8::*;
    use rand::{rng, Rng};
    use rand_distr::{StandardNormal, StandardUniform, Uniform};

    #[test]
    fn test_sample_f8e4m3() {
        let mut rng = rng();
        let _: F8E4M3 = rng.sample(StandardUniform);
        let _: F8E4M3 = rng.sample(StandardNormal);
        let _: F8E4M3 =
            rng.sample(Uniform::new(F8E4M3::from_f32(0.0), F8E4M3::from_f32(1.0)).unwrap());
        #[cfg(feature = "num-traits")]
        let _: F8E4M3 = rng
            .sample(rand_distr::Normal::new(F8E4M3::from_f32(0.0), F8E4M3::from_f32(1.0)).unwrap());
    }

    #[test]
    fn test_sample_f8e5m2() {
        let mut rng = rng();
        let _: F8E5M2 = rng.sample(StandardUniform);
        let _: F8E5M2 = rng.sample(StandardNormal);
        let _: F8E5M2 =
            rng.sample(Uniform::new(F8E5M2::from_f32(0.0), F8E5M2::from_f32(1.0)).unwrap());
        #[cfg(feature = "num-traits")]
        let _: F8E5M2 = rng
            .sample(rand_distr::Normal::new(F8E5M2::from_f32(0.0), F8E5M2::from_f32(1.0)).unwrap());
    }
}

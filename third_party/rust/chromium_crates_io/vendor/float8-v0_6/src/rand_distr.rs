use crate::{F8E4M3, F8E5M2};

use rand::{distr::Distribution, Rng};
use rand_distr::uniform::UniformFloat;

macro_rules! impl_distribution_via_f32 {
    ($Ty:ty, $Distr:ty) => {
        impl Distribution<$Ty> for $Distr {
            fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> $Ty {
                <$Ty>::from_f32(<Self as Distribution<f32>>::sample(self, rng))
            }
        }
    };
}

impl_distribution_via_f32!(F8E4M3, rand_distr::StandardUniform);
impl_distribution_via_f32!(F8E4M3, rand_distr::StandardNormal);
impl_distribution_via_f32!(F8E4M3, rand_distr::Exp1);
impl_distribution_via_f32!(F8E4M3, rand_distr::Open01);
impl_distribution_via_f32!(F8E4M3, rand_distr::OpenClosed01);

impl_distribution_via_f32!(F8E5M2, rand_distr::StandardUniform);
impl_distribution_via_f32!(F8E5M2, rand_distr::StandardNormal);
impl_distribution_via_f32!(F8E5M2, rand_distr::Exp1);
impl_distribution_via_f32!(F8E5M2, rand_distr::Open01);
impl_distribution_via_f32!(F8E5M2, rand_distr::OpenClosed01);

#[derive(Debug, Clone, Copy)]
pub struct Float16Sampler(UniformFloat<f32>);

impl rand_distr::uniform::SampleUniform for F8E5M2 {
    type Sampler = Float16Sampler;
}

impl rand_distr::uniform::UniformSampler for Float16Sampler {
    type X = F8E5M2;
    fn new<B1, B2>(low: B1, high: B2) -> Result<Self, rand_distr::uniform::Error>
    where
        B1: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
        B2: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
    {
        Ok(Self(UniformFloat::new(
            low.borrow().to_f32(),
            high.borrow().to_f32(),
        )?))
    }
    fn new_inclusive<B1, B2>(low: B1, high: B2) -> Result<Self, rand_distr::uniform::Error>
    where
        B1: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
        B2: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
    {
        Ok(Self(UniformFloat::new_inclusive(
            low.borrow().to_f32(),
            high.borrow().to_f32(),
        )?))
    }
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Self::X {
        F8E5M2::from_f32(self.0.sample(rng))
    }
}

#[derive(Debug, Clone, Copy)]
pub struct BFloat16Sampler(UniformFloat<f32>);

impl rand_distr::uniform::SampleUniform for F8E4M3 {
    type Sampler = BFloat16Sampler;
}

impl rand_distr::uniform::UniformSampler for BFloat16Sampler {
    type X = F8E4M3;
    fn new<B1, B2>(low: B1, high: B2) -> Result<Self, rand_distr::uniform::Error>
    where
        B1: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
        B2: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
    {
        Ok(Self(UniformFloat::new(
            low.borrow().to_f32(),
            high.borrow().to_f32(),
        )?))
    }
    fn new_inclusive<B1, B2>(low: B1, high: B2) -> Result<Self, rand_distr::uniform::Error>
    where
        B1: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
        B2: rand_distr::uniform::SampleBorrow<Self::X> + Sized,
    {
        Ok(Self(UniformFloat::new_inclusive(
            low.borrow().to_f32(),
            high.borrow().to_f32(),
        )?))
    }
    fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Self::X {
        F8E4M3::from_f32(self.0.sample(rng))
    }
}

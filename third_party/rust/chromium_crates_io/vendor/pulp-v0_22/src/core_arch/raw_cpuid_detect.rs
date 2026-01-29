use core::sync::atomic;

static FEATURES: [atomic::AtomicU32; 4] = [const { atomic::AtomicU32::new(0) }; 4];
static FEATURES_DETECTED: atomic::AtomicBool = atomic::AtomicBool::new(false);

#[macro_export]
#[doc(hidden)]
macro_rules! feature_idx {
	("aes") => {
		0
	};
	("pclmulqdq") => {
		1
	};
	("rdrand") => {
		2
	};
	("rdseed") => {
		3
	};
	("sse") => {
		4
	};
	("sse2") => {
		5
	};
	("sse3") => {
		6
	};
	("ssse3") => {
		7
	};
	("sse4.1") => {
		8
	};
	("sse4.2") => {
		9
	};
	("sse4a") => {
		10
	};
	("sha") => {
		11
	};
	("avx") => {
		12
	};
	("avx2") => {
		13
	};
	("f16c") => {
		14
	};
	("fma") => {
		15
	};
	("bmi1") => {
		16
	};
	("bmi2") => {
		17
	};
	("lzcnt") => {
		18
	};
	("tbm") => {
		19
	};
	("popcnt") => {
		20
	};
	("fxsr") => {
		21
	};
	("xsave") => {
		22
	};
	("xsaveopt") => {
		23
	};
	("xsaves") => {
		24
	};
	("xsavec") => {
		25
	};
	("cmpxchg16b") => {
		26
	};
	("adx") => {
		27
	};
	("rtm") => {
		28
	};
	("avx512vl") => {
		29
	};
	("avx512f") => {
		30
	};
	("avx512cd") => {
		31
	};
	("avx512er") => {
		32
	};
	("avx512pf") => {
		33
	};
	("avx512bw") => {
		34
	};
	("avx512dq") => {
		35
	};
	("avx512ifma") => {
		36
	};
	("avx512vbmi") => {
		37
	};
	("avx512vpopcntdq") => {
		38
	};
	("avx512vbmi2") => {
		39
	};
	("gfni") => {
		40
	};
	("vaes") => {
		41
	};
	("vpclmulqdq") => {
		42
	};
	("avx512vnni") => {
		43
	};
	("avx512bitalg") => {
		44
	};
	("avx512bf16") => {
		45
	};
	("avx512vp2intersect") => {
		46
	};
}

#[cold]
fn detect_features() {
	let mut local = 0u128;
	let cpuid = raw_cpuid::CpuId::new();
	if let Some(cpuid) = cpuid.get_feature_info() {
		local |= (cpuid.has_aesni() as u128) << feature_idx!("aes");
		local |= (cpuid.has_pclmulqdq() as u128) << feature_idx!("pclmulqdq");
		local |= (cpuid.has_rdrand() as u128) << feature_idx!("rdrand");
		local |= (cpuid.has_sse() as u128) << feature_idx!("sse");
		local |= (cpuid.has_sse2() as u128) << feature_idx!("sse2");
		local |= (cpuid.has_sse3() as u128) << feature_idx!("sse3");
		local |= (cpuid.has_ssse3() as u128) << feature_idx!("ssse3");
		local |= (cpuid.has_sse41() as u128) << feature_idx!("sse4.1");
		local |= (cpuid.has_sse42() as u128) << feature_idx!("sse4.2");
		local |= (cpuid.has_avx() as u128) << feature_idx!("avx");
		local |= (cpuid.has_f16c() as u128) << feature_idx!("f16c");
		local |= (cpuid.has_fma() as u128) << feature_idx!("fma");
		local |= (cpuid.has_popcnt() as u128) << feature_idx!("popcnt");
		local |= (cpuid.has_fxsave_fxstor() as u128) << feature_idx!("fxsr");
		local |= (cpuid.has_xsave() as u128) << feature_idx!("xsave");
		local |= (cpuid.has_cmpxchg16b() as u128) << feature_idx!("cmpxchg16b");
	}
	if let Some(cpuid) = cpuid.get_extended_feature_info() {
		local |= (cpuid.has_rdseed() as u128) << feature_idx!("rdseed");
		local |= (cpuid.has_sha() as u128) << feature_idx!("sha");
		local |= (cpuid.has_avx2() as u128) << feature_idx!("avx2");
		local |= (cpuid.has_bmi1() as u128) << feature_idx!("bmi1");
		local |= (cpuid.has_bmi2() as u128) << feature_idx!("bmi2");

		local |= (cpuid.has_avx512vl() as u128) << feature_idx!("avx512vl");
		local |= (cpuid.has_avx512f() as u128) << feature_idx!("avx512f");
		local |= (cpuid.has_avx512cd() as u128) << feature_idx!("avx512cd");
		local |= (cpuid.has_avx512er() as u128) << feature_idx!("avx512er");
		local |= (cpuid.has_avx512pf() as u128) << feature_idx!("avx512pf");
		local |= (cpuid.has_avx512bw() as u128) << feature_idx!("avx512bw");
		local |= (cpuid.has_avx512dq() as u128) << feature_idx!("avx512dq");
		local |= (cpuid.has_avx512_ifma() as u128) << feature_idx!("avx512ifma");
		local |= (cpuid.has_avx512vbmi() as u128) << feature_idx!("avx512vbmi");
		local |= (cpuid.has_avx512vpopcntdq() as u128) << feature_idx!("avx512vpopcntdq");
		local |= (cpuid.has_avx512vbmi2() as u128) << feature_idx!("avx512vbmi2");
		local |= (cpuid.has_gfni() as u128) << feature_idx!("gfni");
		local |= (cpuid.has_vaes() as u128) << feature_idx!("vaes");
		local |= (cpuid.has_vpclmulqdq() as u128) << feature_idx!("vpclmulqdq");
		local |= (cpuid.has_avx512vnni() as u128) << feature_idx!("avx512vnni");
		local |= (cpuid.has_avx512bitalg() as u128) << feature_idx!("avx512bitalg");
		local |= (cpuid.has_avx512_bf16() as u128) << feature_idx!("avx512bf16");
		local |= (cpuid.has_avx512_vp2intersect() as u128) << feature_idx!("avx512vp2intersect");
		local |= (cpuid.has_adx() as u128) << feature_idx!("adx");
		local |= (cpuid.has_rtm() as u128) << feature_idx!("rtm");
	}
	if let Some(cpuid) = cpuid.get_extended_processor_and_feature_identifiers() {
		local |= (cpuid.has_sse4a() as u128) << feature_idx!("sse4a");
		local |= (cpuid.has_lzcnt() as u128) << feature_idx!("lzcnt");
		local |= (cpuid.has_tbm() as u128) << feature_idx!("tbm");
	}
	if let Some(cpuid) = cpuid.get_extended_state_info() {
		local |= (cpuid.has_xsaveopt() as u128) << feature_idx!("xsaveopt");
		local |= (cpuid.has_xsaves_xrstors() as u128) << feature_idx!("xsaves");
		local |= (cpuid.has_xsavec() as u128) << feature_idx!("xsavec");
	}

	if local != 0 {
		let local: [u32; 4] = cast!(local);
		FEATURES[0].store(local[0], atomic::Ordering::Relaxed);
		FEATURES[1].store(local[1], atomic::Ordering::Relaxed);
		FEATURES[2].store(local[2], atomic::Ordering::Relaxed);
		FEATURES[3].store(local[3], atomic::Ordering::Relaxed);
	}
	FEATURES_DETECTED.store(true, atomic::Ordering::Release);
}

fn features() -> u128 {
	if !FEATURES_DETECTED.load(atomic::Ordering::Acquire) {
		detect_features();
	}
	cast!([
		FEATURES[0].load(atomic::Ordering::Relaxed),
		FEATURES[1].load(atomic::Ordering::Relaxed),
		FEATURES[2].load(atomic::Ordering::Relaxed),
		FEATURES[3].load(atomic::Ordering::Relaxed),
	])
}

#[doc(hidden)]
pub fn feature(i: u32) -> bool {
	(features() >> i) & 1 == 1
}

pub use feature_idx;

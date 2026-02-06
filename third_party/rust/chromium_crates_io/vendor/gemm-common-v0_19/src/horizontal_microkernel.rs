#[macro_export]
macro_rules! horizontal_kernel {
    ($([$target: tt])?, $name: ident, $m: tt, $n: tt $(,)?) => {
        $(#[target_feature(enable = $target)])?
        pub unsafe fn $name(
            k: usize,
            dst: *mut T,
            lhs: *const T,
            rhs: *const T,
            dst_cs: isize,
            dst_rs: isize,
            lhs_rs: isize,
            rhs_cs: isize,
            alpha: T,
            beta: T,
            alpha_status: u8,
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
        ) {
            let mut accum = [[splat(::core::mem::zeroed()); $m]; $n];
            seq_macro::seq!(M_ITER in 0..$m {
               let lhs~M_ITER = lhs.wrapping_offset(M_ITER * lhs_rs);
            });
            seq_macro::seq!(N_ITER in 0..$n {
               let rhs~N_ITER = rhs.wrapping_offset(N_ITER * rhs_cs);
            });

            let mut depth = 0;
            while depth < k / N * N {
                seq_macro::seq!(M_ITER in 0..$m {
                   let lhs~M_ITER = *(lhs~M_ITER.add(depth) as *const [T; N]);
                });
                seq_macro::seq!(N_ITER in 0..$n {
                   let rhs~N_ITER = *(rhs~N_ITER.add(depth) as *const [T; N]);
                });

                seq_macro::seq!(M_ITER in 0..$m {
                    seq_macro::seq!(N_ITER in 0..$n {
                        accum[N_ITER][M_ITER] = mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                    });
                });

                depth += N;
            }

            if depth < k {
                seq_macro::seq!(M_ITER in 0..$m {
                   let lhs~M_ITER = partial_load(lhs~M_ITER.add(depth), k - depth);
                });
                seq_macro::seq!(N_ITER in 0..$n {
                   let rhs~N_ITER = partial_load(rhs~N_ITER.add(depth), k - depth);
                });

                seq_macro::seq!(M_ITER in 0..$m {
                    seq_macro::seq!(N_ITER in 0..$n {
                        accum[N_ITER][M_ITER] = mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                    });
                });
            }

            let mut accum_reduced: [[T; $m]; $n] = core::mem::zeroed();
            seq_macro::seq!(M_ITER in 0..$m {
                seq_macro::seq!(N_ITER in 0..$n {
                    accum_reduced[N_ITER][M_ITER] = reduce_sum(accum[N_ITER][M_ITER]);
                });
            });

            if alpha_status == 0 {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = scalar_mul(beta, accum_reduced[N_ITER][M_ITER]);
                    }});
                }});
            } else if alpha_status == 1 {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = scalar_mul_add(
                            beta,
                            accum_reduced[N_ITER][M_ITER],
                            *dst,
                        );
                    }});
                }});
            } else {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = scalar_add(
                            scalar_mul(beta, accum_reduced[N_ITER][M_ITER]),
                            scalar_mul(alpha, *dst),
                        );
                    }});
                }});
            }
        }
    };
}

#[macro_export]
macro_rules! horizontal_cplx_kernel {
    ($([$target: tt])?, $name: ident, $m: tt, $n: tt $(,)?) => {
        $(#[target_feature(enable = $target)])?
        pub unsafe fn $name(
            k: usize,
            dst: *mut num_complex::Complex<T>,
            lhs: *const num_complex::Complex<T>,
            rhs: *const num_complex::Complex<T>,
            dst_cs: isize,
            dst_rs: isize,
            lhs_rs: isize,
            rhs_cs: isize,
            alpha: num_complex::Complex<T>,
            beta: num_complex::Complex<T>,
            alpha_status: u8,
            _conj_dst: bool,
            conj_lhs: bool,
            conj_rhs: bool,
        ) {
            let mut accum = [[splat(::core::mem::zeroed()); $m]; $n];
            seq_macro::seq!(M_ITER in 0..$m {
               let lhs~M_ITER = lhs.wrapping_offset(M_ITER * lhs_rs);
            });
            seq_macro::seq!(N_ITER in 0..$n {
               let rhs~N_ITER = rhs.wrapping_offset(N_ITER * rhs_cs);
            });

            let (conj_lhs, conj_all) = match (conj_lhs, conj_rhs) {
                (true, true) => (false, true),
                (false, true) => (true, true),
                (true, false) => (true, false),
                (false, false) => (false, false),
            };

            if conj_lhs {
                let mut depth = 0;
                while depth < k / CPLX_N * CPLX_N {
                    seq_macro::seq!(M_ITER in 0..$m {
                       let lhs~M_ITER = *(lhs~M_ITER.add(depth) as *const Pack);
                    });
                    seq_macro::seq!(N_ITER in 0..$n {
                       let rhs~N_ITER = *(rhs~N_ITER.add(depth) as *const Pack);
                    });

                    seq_macro::seq!(M_ITER in 0..$m {
                        seq_macro::seq!(N_ITER in 0..$n {
                            accum[N_ITER][M_ITER] = conj_mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                        });
                    });

                    depth += CPLX_N;
                }

                if depth < k {
                    seq_macro::seq!(M_ITER in 0..$m {
                       let lhs~M_ITER = partial_load(lhs~M_ITER.add(depth), k - depth);
                    });
                    seq_macro::seq!(N_ITER in 0..$n {
                       let rhs~N_ITER = partial_load(rhs~N_ITER.add(depth), k - depth);
                    });

                    seq_macro::seq!(M_ITER in 0..$m {
                        seq_macro::seq!(N_ITER in 0..$n {
                            accum[N_ITER][M_ITER] = conj_mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                        });
                    });
                }
            } else {
                let mut depth = 0;
                while depth < k / CPLX_N * CPLX_N {
                    seq_macro::seq!(M_ITER in 0..$m {
                       let lhs~M_ITER = *(lhs~M_ITER.add(depth) as *const Pack);
                    });
                    seq_macro::seq!(N_ITER in 0..$n {
                       let rhs~N_ITER = *(rhs~N_ITER.add(depth) as *const Pack);
                    });

                    seq_macro::seq!(M_ITER in 0..$m {
                        seq_macro::seq!(N_ITER in 0..$n {
                            accum[N_ITER][M_ITER] = mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                        });
                    });

                    depth += CPLX_N;
                }

                if depth < k {
                    seq_macro::seq!(M_ITER in 0..$m {
                       let lhs~M_ITER = partial_load(lhs~M_ITER.add(depth), k - depth);
                    });
                    seq_macro::seq!(N_ITER in 0..$n {
                       let rhs~N_ITER = partial_load(rhs~N_ITER.add(depth), k - depth);
                    });

                    seq_macro::seq!(M_ITER in 0..$m {
                        seq_macro::seq!(N_ITER in 0..$n {
                            accum[N_ITER][M_ITER] = mul_add(lhs~M_ITER, rhs~N_ITER, accum[N_ITER][M_ITER]);
                        });
                    });
                }
            }

            if conj_all {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        accum[N_ITER][M_ITER] = conj(accum[N_ITER][M_ITER]);
                    }});
                }});
            }

            let mut accum_reduced: [[num_complex::Complex<T>; $m]; $n] = core::mem::zeroed();
            seq_macro::seq!(M_ITER in 0..$m {
                seq_macro::seq!(N_ITER in 0..$n {
                    accum_reduced[N_ITER][M_ITER] = reduce_sum(accum[N_ITER][M_ITER]);
                });
            });

            if alpha_status == 0 {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = beta * accum_reduced[N_ITER][M_ITER];
                    }});
                }});
            } else if alpha_status == 1 {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = (beta * accum_reduced[N_ITER][M_ITER]) + *dst;
                    }});
                }});
            } else {
                seq_macro::seq!(M_ITER in 0..$m {{
                    seq_macro::seq!(N_ITER in 0..$n {{
                        let dst = dst.offset(dst_cs * N_ITER + dst_rs * M_ITER);
                        *dst = (beta * accum_reduced[N_ITER][M_ITER]) + (alpha * *dst);
                    }});
                }});
            }
        }
    };
}

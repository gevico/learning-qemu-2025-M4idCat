/*
 * RISC-V translation routines for the sort instruction (G233 Board)
 *
 * Copyright (c) 2025 M4idCat
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

static bool trans_sort(DisasContext *ctx, arg_sort *a)
{
    TCGv src_addr = get_gpr(ctx, a->rs1, EXT_NONE);
    TCGv size = get_gpr(ctx, a->rs2, EXT_NONE);
    TCGv involve_size = get_gpr(ctx, a->rd, EXT_NONE);

    gen_helper_sort(tcg_env, src_addr, size, involve_size);
    return true;
}
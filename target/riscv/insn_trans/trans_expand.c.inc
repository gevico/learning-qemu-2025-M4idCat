/* Auto-generated translation fragment for custom EXPAND instruction */
/* expand rd, rs1, rs2  -- split each byte into two 4-bit values stored as bytes */
static bool
trans_expand(DisasContext *ctx, arg_expand *a)
{
    TCGv src_addr = get_gpr(ctx, a->rs1, EXT_NONE);
    TCGv dst_addr = get_gpr(ctx, a->rd, EXT_NONE);
    TCGv cnt = get_gpr(ctx, a->rs2, EXT_NONE);

    gen_helper_expand(tcg_env, dst_addr, src_addr, cnt);
    return true;
}

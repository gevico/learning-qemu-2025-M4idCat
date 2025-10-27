/* Auto-generated translation fragment for custom CRUSH instruction */
/* crush rd, rs1, rs2  -- pack low 4 bits of bytes from source into dst */
static bool
trans_crush(DisasContext *ctx, arg_crush *a)
{
    TCGv src_addr = get_gpr(ctx, a->rs1, EXT_NONE);
    TCGv dst_addr = get_gpr(ctx, a->rd, EXT_NONE);
    TCGv cnt = get_gpr(ctx, a->rs2, EXT_NONE);

    gen_helper_crush(tcg_env, dst_addr, src_addr, cnt);
    return true;
}

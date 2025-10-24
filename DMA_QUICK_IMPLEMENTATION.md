# DMA 指令实现快速参考

本文档包含实现 DMA 转置指令所需的所有代码片段。

---

## 📁 文件 1: target/riscv/insn32.decode

**位置**: 在文件末尾添加

```decode
# Custom DMA instruction for G233 Board
dma       0000110  ..... ..... 110 ..... 1111011 @r
```

---

## 📁 文件 2: target/riscv/insn_trans/trans_dma.c.inc (新建)

**完整文件内容**:

```c
/*
 * RISC-V translation routines for the DMA instruction (G233 Board)
 *
 * Copyright (c) 2025 Learning QEMU
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

static bool trans_dma(DisasContext *ctx, arg_dma *a)
{
    TCGv src_addr, dst_addr, grain_size;
    
    src_addr = tcg_temp_new();
    dst_addr = tcg_temp_new();
    grain_size = tcg_temp_new();
    
    gen_get_gpr(ctx, src_addr, a->rs1);
    gen_get_gpr(ctx, dst_addr, a->rd);
    gen_get_gpr(ctx, grain_size, a->rs2);
    
    gen_helper_dma(tcg_env, dst_addr, src_addr, grain_size);
    
    return true;
}
```

---

## 📁 文件 3: target/riscv/helper.h

**位置**: 在文件末尾或 RV32/RV64 指令 helper 区域添加

```c
/* Custom DMA instruction for G233 Board */
DEF_HELPER_4(dma, void, env, tl, tl, tl)
```

---

## 📁 文件 4: target/riscv/op_helper.c

**位置**: 在文件末尾添加

```c
/*
 * DMA 转置 Helper 函数
 */
void helper_dma(CPURISCVState *env, target_ulong dst_addr,
                target_ulong src_addr, target_ulong grain_size)
{
    int M, N;
    M = N = 8 << grain_size;
    
    uintptr_t ra = GETPC();
    
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            target_ulong src_offset = (j * N + i) * 4;
            uint32_t value = cpu_ldl_data_ra(env, src_addr + src_offset,
                                             GETPC());
            
            target_ulong dst_offset = (i * M + j) * 4;
            cpu_stl_data_ra(env, dst_addr + dst_offset, value, ra);
        }
    }
}
```

---

## 📁 文件 5: target/riscv/translate.c

**位置**: 在包含其他 trans_*.c.inc 文件的区域添加

找到类似这样的代码块:
```c
#include "insn_trans/trans_rvi.c.inc"
#include "insn_trans/trans_rvm.c.inc"
// ... 其他 include
```

在其中添加:
```c
#include "insn_trans/trans_dma.c.inc"
```

**建议**: 放在 `#include "insn_trans/trans_rvi.c.inc"` 之后

---

## 📁 文件 6: disas/riscv.c (可选 - 反汇编支持)

### 6.1 添加操作码枚举

**位置**: 在 `rv_op` 枚举中

```c
typedef enum {
    // ... 现有枚举 ...
    rv_op_dma,
    // ... 更多枚举 ...
} rv_opcode;
```

### 6.2 添加操作码名称

**位置**: 在 `rv_op_name` 数组中（位置要和枚举对应）

```c
static const char *rv_op_name[] = {
    // ... 现有名称 ...
    "dma",
    // ... 更多名称 ...
};
```

### 6.3 添加解码逻辑

**位置**: 在 `decode_inst_opcode()` 函数中

找到或创建 opcode 0x7b 的处理:

```c
static void decode_inst_opcode(rv_decode *dec, rv_isa isa)
{
    rv_inst inst = dec->inst;
    rv_opcode op = rv_op_illegal;
    
    switch ((inst >> 0) & 0b11) {
    // ... 其他 case ...
    
    case 3:  // 32-bit instructions
        switch ((inst >> 2) & 0b11111) {
        // ... 其他 case ...
        
        case 30:  // opcode = 1111011 (0x7b), bits[6:2] = 30
            switch ((inst >> 12) & 0b111) {  // funct3
            case 6:  // funct3 = 110
                switch ((inst >> 25) & 0b1111111) {  // funct7
                case 6:  // funct7 = 0000110
                    op = rv_op_dma;
                    break;
                }
                break;
            }
            break;
        
        // ... 其他 case ...
        }
        break;
    }
    
    dec->op = op;
}
```

### 6.4 添加格式化输出

**位置**: 在 `format_inst()` 函数中

```c
static void format_inst(char *buf, size_t buflen, rv_decode *dec)
{
    // ... 其他 case ...
    
    case rv_op_dma:
        format_r(buf, buflen, "dma", dec);
        break;
    
    // ... 其他 case ...
}
```

---

## 🔨 编译命令

```bash
# 配置 (首次或修改配置时)
./configure --target-list=riscv64-softmmu,riscv64-linux-user

# 编译 QEMU
make -j$(nproc)

# 编译测试程序
cd tests/gevico/tcg
make -C riscv64
```

---

## 🧪 测试命令

```bash
# 运行测试
./build/qemu-riscv64 tests/gevico/tcg/riscv64/test-insn-dma

# 调试：查看反汇编
./build/qemu-riscv64 -d in_asm tests/gevico/tcg/riscv64/test-insn-dma 2>&1 | less

# 调试：查看 TCG IR
./build/qemu-riscv64 -d op tests/gevico/tcg/riscv64/test-insn-dma 2>&1 | less

# 调试：查看执行流程
./build/qemu-riscv64 -d exec tests/gevico/tcg/riscv64/test-insn-dma 2>&1 | less
```

---

## ✅ 实现检查清单

### 必需的修改:
- [ ] `target/riscv/insn32.decode` - 添加 DMA 指令定义
- [ ] `target/riscv/insn_trans/trans_dma.c.inc` - 创建翻译函数
- [ ] `target/riscv/helper.h` - 声明 helper 函数
- [ ] `target/riscv/op_helper.c` - 实现 helper 函数
- [ ] `target/riscv/translate.c` - 包含 trans_dma.c.inc

### 可选的修改:
- [ ] `disas/riscv.c` - 添加反汇编支持（强烈推荐）

---

## 📊 预期测试结果

成功实现后，运行测试程序应输出:

```
A = 
   0    1    2    3    4    5    6    7 
   8    9   10   11   12   13   14   15 
  ...

Grain: 8x8, compare sucessful!
Grain: 16x16, compare sucessful!
Grain: 32x32, compare sucessful!
```

---

## 🐛 常见问题排查

### 问题 1: 编译错误 "unknown type name 'arg_dma'"

**检查**:
- `insn32.decode` 中是否正确添加了 dma 指令定义
- 格式是否使用了 `@r`
- 是否重新运行了 make（decodetree 会重新生成）

### 问题 2: 链接错误 "undefined reference to 'helper_dma'"

**检查**:
- `helper.h` 中是否添加了 `DEF_HELPER_4(dma, void, env, tl, tl, tl)`
- `op_helper.c` 中是否实现了 `helper_dma()` 函数
- 函数签名是否完全匹配

### 问题 3: 运行时段错误

**检查**:
- helper 函数中的地址计算是否正确
- 内存访问是否越界
- grain_size 的处理是否正确 (8 << grain_size)

### 问题 4: 测试结果不正确

**检查**:
- 转置逻辑：`src_offset = (j * N + i) * 4`, `dst_offset = (i * M + j) * 4`
- 是否使用了 FP32 (4字节)
- 矩阵大小计算是否正确

---

## 💡 实现提示

### 1. 指令格式说明

```
dma rd, rs1, rs2

等价于汇编:
.insn r 0x7b, 6, 6, rd, rs1, rs2

对应测试代码中的:
asm volatile (
   ".insn r 0x7b, 6, 6, %0, %1, %2"
    : :"r"(dst), "r"(src), "r"(grain_size));
```

### 2. 参数映射

| 指令字段 | 测试代码 | helper 函数参数 | 含义 |
|---------|---------|----------------|------|
| rd | dst | dst_addr | 目标地址 |
| rs1 | src | src_addr | 源地址 |
| rs2 | grain_size | grain_size | 矩阵规模 |

### 3. 矩阵大小映射

| grain_size | 矩阵大小 | 元素总数 | 内存占用 |
|-----------|---------|---------|---------|
| 0 | 8×8 | 64 | 256 字节 |
| 1 | 16×16 | 256 | 1024 字节 |
| 2 | 32×32 | 1024 | 4096 字节 |

### 4. 地址计算示例

对于 8×8 矩阵，转置 A[3][5] 到 C[5][3]:

```c
// 读取 A[3][5]
src_offset = (3 * 8 + 5) * 4 = 29 * 4 = 116 字节

// 写入 C[5][3]  
dst_offset = (5 * 8 + 3) * 4 = 43 * 4 = 172 字节
```

---

## 🎯 下一步

实现完成后，可以尝试:

1. **性能测试**: 对比软件转置和 DMA 指令的性能
2. **扩展功能**: 添加对其他矩阵运算的支持
3. **错误处理**: 添加地址对齐检查、越界检查等
4. **优化**: 使用批量内存访问优化性能

祝实现顺利！🚀

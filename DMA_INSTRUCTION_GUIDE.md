# G233 Board DMA 转置指令实现教学指南

## 📋 任务概述

实现一个自定义的 RISC-V 指令 `dma`，用于将一个 FP32 精度的二维矩阵从源地址转置后搬运到目标地址。

### 指令格式

```
31      25 24  20 19    15 14     12  11                7 6     0
+---------+--------+--------+----------+-------------------+-------+
| 0000110 |  rs2   |  rs1   |    110   |         rd        |1111011| dma
+---------+--------+--------+----------+-------------------+-------+
```

- **opcode**: `0x7b` (1111011)
- **funct3**: `6` (110)
- **funct7**: `6` (0000110)
- **rs1**: 源矩阵在内存中的起始地址
- **rs2**: 矩阵规模 (0=8x8, 1=16x16, 2=32x32)
- **rd**: 目标矩阵在内存中的地址

---

## 🎯 实现步骤

### 步骤 1: 定义指令格式 (insn32.decode)

**文件位置**: `target/riscv/insn32.decode`

**需要添加的内容**:

在文件末尾添加 DMA 指令的解码定义。首先需要找到合适的位置，通常在文件的最后。

```decode
# Custom DMA instruction for G233 Board
dma       0000110  ..... ..... 110 ..... 1111011 @r
```

**解释**:
- `dma`: 指令名称
- `0000110`: funct7 字段 (二进制)
- `.....`: rs2 字段 (5位，由 @r 格式处理)
- `.....`: rs1 字段 (5位，由 @r 格式处理)
- `110`: funct3 字段 (二进制)
- `.....`: rd 字段 (5位，由 @r 格式处理)
- `1111011`: opcode 字段 (二进制 = 0x7b)
- `@r`: 使用 R-type 格式 (已在文件开头定义)

**提示**: 
- 打开 `target/riscv/insn32.decode` 文件
- 滚动到最后
- 添加上面的定义

---

### 步骤 2: 实现指令翻译函数 (trans_dma.c.inc)

**文件位置**: `target/riscv/insn_trans/trans_dma.c.inc` (新建文件)

**需要理解的知识点**:

#### TCG (Tiny Code Generator)
- QEMU 使用 TCG 将目标架构指令翻译为中间表示 (IR)
- 翻译函数将 RISC-V 指令转换为 TCG 操作
- TCG 操作最终会被编译为主机代码执行

#### Helper 函数
- 复杂的操作通过 helper 函数实现
- Helper 函数在运行时被调用，执行实际的操作
- Helper 函数可以访问 CPU 状态和内存

#### 创建文件内容:

```c
/*
 * RISC-V translation routines for the DMA instruction (G233 Board)
 *
 * Copyright (c) 2025 Learning QEMU
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * DMA 转置指令翻译函数
 * 
 * 这个函数在指令解码后被调用，负责生成对应的 TCG 代码
 * 
 * @ctx: 反汇编上下文，包含当前指令的信息
 * @a: 指令参数，包含 rd, rs1, rs2 的值
 * @return: true 表示指令处理成功
 */
static bool trans_dma(DisasContext *ctx, arg_dma *a)
{
    // 步骤 1: 声明 TCG 临时变量
    // TCGv 是 TCG 变量类型，用于存储寄存器值
    TCGv src_addr, dst_addr, grain_size;
    
    // 步骤 2: 分配 TCG 临时变量
    // 这些变量会在 TCG 代码生成时使用
    src_addr = tcg_temp_new();
    dst_addr = tcg_temp_new();
    grain_size = tcg_temp_new();
    
    // 步骤 3: 从 RISC-V 寄存器读取值到 TCG 变量
    // gen_get_gpr() 生成读取通用寄存器的 TCG 代码
    // a->rs1 包含源地址
    // a->rd 包含目标地址
    // a->rs2 包含矩阵规模参数
    gen_get_gpr(ctx, src_addr, a->rs1);
    gen_get_gpr(ctx, dst_addr, a->rd);
    gen_get_gpr(ctx, grain_size, a->rs2);
    
    // 步骤 4: 生成调用 helper 函数的 TCG 代码
    // gen_helper_dma() 会生成一个函数调用
    // 这个 helper 函数会在运行时执行实际的 DMA 转置操作
    gen_helper_dma(tcg_env, dst_addr, src_addr, grain_size);
    
    // 步骤 5: 返回 true 表示指令处理成功
    return true;
}
```

**关键点解释**:

1. **arg_dma**: 这个结构体会由 QEMU 的 decodetree 工具自动生成，包含 rd, rs1, rs2 字段

2. **gen_get_gpr()**: 生成从 RISC-V 通用寄存器读取值的 TCG 代码
   - 在翻译时生成代码，但实际读取在运行时发生

3. **gen_helper_dma()**: 调用 helper 函数
   - `tcg_env`: CPU 环境指针，用于访问 CPU 状态
   - 三个参数按顺序传递给 helper 函数

---

### 步骤 3: 声明 Helper 函数 (helper.h)

**文件位置**: `target/riscv/helper.h`

**需要添加的内容**:

在文件中找到合适的位置（通常在文件末尾），添加 helper 函数声明。

```c
/* Custom DMA instruction for G233 Board */
DEF_HELPER_4(dma, void, env, tl, tl, tl)
```

**解释**:
- `DEF_HELPER_4`: 定义一个有 4 个参数的 helper 函数
  - 参数数量包括返回值和所有输入参数
  - 4 = 1 (env) + 3 (dst_addr, src_addr, grain_size)
  
- `dma`: helper 函数名称（会自动加上 `helper_` 前缀）

- `void`: 返回类型

- `env, tl, tl, tl`: 参数类型列表
  - `env`: CPU 环境指针
  - `tl`: target_long，目标架构的长整型（在 RISC-V 64 上是 64 位）
  
**提示**: 在 `target/riscv/helper.h` 中搜索其他 `DEF_HELPER` 定义作为参考。

---

### 步骤 4: 实现 Helper 函数 (op_helper.c)

**文件位置**: `target/riscv/op_helper.c`

**需要理解的知识点**:

#### 内存访问
- QEMU 提供了安全的内存访问函数
- `cpu_ldl_data_ra()`: 从客户机内存读取 32 位数据
- `cpu_stl_data_ra()`: 向客户机内存写入 32 位数据
- `ra` 参数用于异常处理时的返回地址

#### 矩阵转置算法
```
原矩阵 A (M×N):     转置矩阵 C (N×M):
[a00 a01 a02]       [a00 a10 a20]
[a10 a11 a12]  =>   [a01 a11 a21]
[a20 a21 a22]       [a02 a12 a22]

C[i][j] = A[j][i]
C[i * M + j] = A[j * N + i]
```

#### 实现代码:

```c
/*
 * DMA 转置 Helper 函数
 * 
 * 这个函数在运行时被调用，执行实际的矩阵转置和数据搬运
 * 
 * @env: RISC-V CPU 环境，用于访问内存
 * @dst_addr: 目标地址 (来自 rd 寄存器)
 * @src_addr: 源地址 (来自 rs1 寄存器)  
 * @grain_size: 矩阵规模参数 (来自 rs2 寄存器)
 *              0 = 8x8, 1 = 16x16, 2 = 32x32
 */
void helper_dma(CPURISCVState *env, target_ulong dst_addr,
                target_ulong src_addr, target_ulong grain_size)
{
    // 步骤 1: 根据 grain_size 确定矩阵大小
    // 使用位移运算: size = 8 << grain_size
    // grain_size=0: 8<<0 = 8
    // grain_size=1: 8<<1 = 16
    // grain_size=2: 8<<2 = 32
    int M, N;
    M = N = 8 << grain_size;
    
    // 步骤 2: 获取返回地址，用于异常处理
    // GETPC() 获取当前的程序计数器，用于异常时回溯
    uintptr_t ra = GETPC();
    
    // 步骤 3: 执行矩阵转置
    // 双重循环遍历矩阵的每个元素
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            // 步骤 3.1: 计算源地址偏移
            // 源矩阵是 M×N，按行存储
            // A[j][i] 的偏移 = (j * N + i) * sizeof(uint32_t)
            // sizeof(uint32_t) = 4 字节 (FP32)
            target_ulong src_offset = (j * N + i) * 4;
            
            // 步骤 3.2: 从源地址读取数据
            // cpu_ldl_data_ra() 读取 32 位数据 (little-endian)
            // 参数: CPU环境, 地址, 内存访问权限, 返回地址
            uint32_t value = cpu_ldl_data_ra(env, src_addr + src_offset,
                                             GETPC());
            
            // 步骤 3.3: 计算目标地址偏移
            // 目标矩阵是 N×M，按行存储
            // C[i][j] 的偏移 = (i * M + j) * sizeof(uint32_t)
            target_ulong dst_offset = (i * M + j) * 4;
            
            // 步骤 3.4: 向目标地址写入数据
            // cpu_stl_data_ra() 写入 32 位数据 (little-endian)
            // 参数: CPU环境, 地址, 数据, 内存访问权限, 返回地址
            cpu_stl_data_ra(env, dst_addr + dst_offset, value, ra);
        }
    }
}
```

**关键点解释**:

1. **内存访问函数**:
   - `cpu_ldl_data_ra()`: load 32-bit little-endian
   - `cpu_stl_data_ra()`: store 32-bit little-endian
   - 这些函数会处理内存映射、权限检查等

2. **地址计算**:
   - 二维数组在内存中是连续存储的
   - `A[i][j]` 在内存中的位置 = base + (i * 列数 + j) * 元素大小

3. **转置逻辑**:
   - 读取 `A[j][i]`，写入 `C[i][j]`
   - 这样就实现了矩阵转置

---

### 步骤 5: 包含翻译文件 (translate.c)

**文件位置**: `target/riscv/translate.c`

**需要添加的内容**:

在文件中找到其他 `#include "insn_trans/trans_*.c.inc"` 的位置，添加：

```c
#include "insn_trans/trans_dma.c.inc"
```

**建议位置**: 在 `#include "insn_trans/trans_rvi.c.inc"` 之后

**解释**:
- 这样可以将 DMA 指令的翻译函数包含到主翻译器中
- QEMU 会在解码到 `dma` 指令时调用 `trans_dma()` 函数

---

### 步骤 6: 添加反汇编支持 (可选但推荐)

**文件位置**: `disas/riscv.c`

这一步是可选的，但可以让 QEMU 在反汇编时正确显示 DMA 指令。

#### 6.1 添加操作码枚举

在 `rv_op` 枚举中添加:

```c
typedef enum {
    // ... 现有的枚举值 ...
    rv_op_dma,          // 添加这一行
} rv_opcode;
```

#### 6.2 添加操作码名称

在操作码名称数组中添加:

```c
static const char *rv_op_name[] = {
    // ... 现有的名称 ...
    "dma",              // 添加这一行，位置要和枚举对应
};
```

#### 6.3 添加解码逻辑

在 `decode_inst_opcode()` 函数中添加解码逻辑。找到 opcode `0x7b` 的处理部分:

```c
case 0x7b:  // custom-3 opcode
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
```

#### 6.4 添加格式化输出

在 `format_inst()` 函数中添加:

```c
case rv_op_dma:
    format_r(buf, sizeof(buf), "dma", dec);
    break;
```

**解释**: 这样当使用 QEMU 的反汇编功能时，会正确显示 `dma rd, rs1, rs2`

---

## 🔧 编译和测试

### 编译步骤

```bash
# 1. 配置 (如果还没有配置)
./configure --target-list=riscv64-softmmu,riscv64-linux-user

# 2. 编译
make -j$(nproc)

# 3. 编译测试程序
cd tests/gevico/tcg
make -C riscv64
```

### 测试方法

```bash
# 方式 1: 使用 linux-user 模式测试
./build/qemu-riscv64 tests/gevico/tcg/riscv64/test-insn-dma

# 方式 2: 使用 system 模式 (需要完整的系统镜像)
./build/qemu-system-riscv64 -M g233 -kernel <kernel-image> -nographic
```

### 预期输出

如果实现正确，应该看到类似的输出:

```
A = 
   0    1    2    3    4    5    6    7 
   8    9   10   11   12   13   14   15 
  16   17   18   19   20   21   22   23 
  24   25   26   27   28   29   30   31 
  32   33   34   35   36   37   38   39 
  40   41   42   43   44   45   46   47 
  48   49   50   51   52   53   54   55 
  56   57   58   59   60   61   62   63 

Grain: 8x8, compare sucessful!
Grain: 16x16, compare sucessful!
Grain: 32x32, compare sucessful!
```

---

## 🐛 调试技巧

### 1. 检查指令是否被正确解码

```bash
# 使用 -d in_asm 查看翻译的指令
./build/qemu-riscv64 -d in_asm tests/gevico/tcg/riscv64/test-insn-dma
```

应该能看到类似:
```
IN: 
0x... : 06c5e7bb  dma a5, a1, a2
```

### 2. 检查 TCG 代码生成

```bash
# 使用 -d op 查看生成的 TCG IR
./build/qemu-riscv64 -d op tests/gevico/tcg/riscv64/test-insn-dma
```

### 3. 追踪内存访问

```bash
# 使用 -d cpu,exec 查看 CPU 状态和执行流程
./build/qemu-riscv64 -d cpu,exec tests/gevico/tcg/riscv64/test-insn-dma
```

### 4. 常见错误

#### 错误 1: 编译错误 "unknown type name 'arg_dma'"
**原因**: decodetree 没有生成对应的结构体
**解决**: 检查 `insn32.decode` 中的定义是否正确

#### 错误 2: 链接错误 "undefined reference to `helper_dma'"
**原因**: helper 函数没有正确实现或链接
**解决**: 检查 `op_helper.c` 和 `helper.h` 中的定义

#### 错误 3: 运行时段错误
**原因**: 内存访问越界
**解决**: 检查地址计算逻辑，确保不超出矩阵范围

---

## 📚 扩展学习

### 1. 理解 QEMU 的翻译流程

```
客户机指令 -> 解码 -> TCG IR -> 主机代码
   dma    -> decode -> gen_helper_dma -> x86/ARM 代码
```

### 2. TCG 操作类型

- **tcg_gen_***: 生成 TCG 操作的函数
- **gen_helper_***: 生成调用 helper 函数的代码
- **gen_get_gpr/gen_set_gpr**: 读写通用寄存器

### 3. 内存访问 API

| 函数 | 用途 |
|------|------|
| `cpu_ldub_data` | 读取 8 位无符号数 |
| `cpu_lduw_data` | 读取 16 位无符号数 |
| `cpu_ldl_data` | 读取 32 位数 |
| `cpu_ldq_data` | 读取 64 位数 |
| `cpu_stb_data` | 写入 8 位数 |
| `cpu_stw_data` | 写入 16 位数 |
| `cpu_stl_data` | 写入 32 位数 |
| `cpu_stq_data` | 写入 64 位数 |

### 4. 优化建议

当前实现是教学版本，实际应用可以优化：

1. **批量内存访问**: 使用 `address_space_rw()` 进行批量读写
2. **SIMD 优化**: 利用主机的 SIMD 指令加速转置
3. **异步 DMA**: 实现真正的异步 DMA 操作
4. **缓存优化**: 考虑缓存行对齐，提高性能

---

## ✅ 完成检查清单

- [ ] 在 `insn32.decode` 中添加 DMA 指令定义
- [ ] 创建 `trans_dma.c.inc` 并实现翻译函数
- [ ] 在 `helper.h` 中声明 helper 函数
- [ ] 在 `op_helper.c` 中实现 helper 函数
- [ ] 在 `translate.c` 中包含 `trans_dma.c.inc`
- [ ] (可选) 在 `disas/riscv.c` 中添加反汇编支持
- [ ] 编译 QEMU
- [ ] 运行测试程序并验证结果

---

## 🎓 总结

通过实现这个 DMA 转置指令，你学习了:

1. **RISC-V 指令格式**: R-type 指令的编码方式
2. **QEMU 指令解码**: decodetree 工具的使用
3. **TCG 翻译**: 从指令到 TCG IR 的转换
4. **Helper 函数**: 复杂操作的实现方式
5. **内存访问**: QEMU 中安全的客户机内存访问
6. **矩阵运算**: 转置算法的实现

这些知识可以帮助你:
- 添加更多自定义指令
- 理解 QEMU 的工作原理
- 进行 CPU 模拟和验证
- 开发硬件加速器

继续加油！🚀

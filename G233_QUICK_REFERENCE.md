# G233 Board 代码补全快速参考

## 需要补全的四个位置

### 📍 位置 1: 头文件引用（第 38 行附近）
```c
/* TODO: you need include some header files */
#include "qom/object.h"
```

---

### 📍 位置 2: g233_soc_init() 函数（第 48-57 行）
```c
static void g233_soc_init(Object *obj)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    G233SoCState *s = RISCV_G233_SOC(obj);
    
    object_initialize_child(obj, "cpus", &s->cpus, TYPE_RISCV_HART_ARRAY);
    object_property_set_int(OBJECT(&s->cpus), "num-harts", ms->smp.cpus,
                            &error_abort);
    object_property_set_int(OBJECT(&s->cpus), "resetvec", 0x1004, &error_abort);
    object_initialize_child(obj, "riscv.g233.gpio0", &s->gpio,
                            TYPE_SIFIVE_GPIO);
}
```

---

### 📍 位置 3: g233_soc_realize() 中的 CPU realize（第 67 行附近）
```c
    /* CPUs realize */
    object_property_set_str(OBJECT(&s->cpus), "cpu-type", ms->cpu_type,
                            &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&s->cpus), &error_fatal);
```

---

### 📍 位置 4: g233_machine_init() 函数（第 167-172 行）
```c
    /* Initialize SoC */
    object_initialize_child(OBJECT(machine), "soc", &s->soc,
                            TYPE_RISCV_G233_SOC);
    qdev_realize(DEVICE(&s->soc), NULL, &error_fatal);

    /* Data Memory(DDR RAM) */
    memory_region_add_subregion(get_system_memory(),
                                memmap[G233_DEV_DRAM].base,
                                machine->ram);
```

---

## 关键 API 函数说明

| 函数 | 用途 | 关键参数 |
|------|------|---------|
| `object_initialize_child()` | 初始化子对象 | 父对象、名称、子对象指针、类型 |
| `object_property_set_int()` | 设置整数属性 | 对象、属性名、值、错误处理 |
| `object_property_set_str()` | 设置字符串属性 | 对象、属性名、值、错误处理 |
| `sysbus_realize()` | Realize 系统总线设备 | 设备、错误处理 |
| `qdev_realize()` | Realize 设备 | 设备、总线、错误处理 |
| `memory_region_add_subregion()` | 添加内存子区域 | 父区域、偏移、子区域 |

---

## 编译和测试命令

```bash
# 1. 配置（只需首次或修改配置时）
./configure --target-list=riscv32-softmmu

# 2. 编译
make -j$(nproc)

# 3. 测试（列出可用的机器类型，应该能看到 g233）
./build/qemu-system-riscv32 -M help | grep g233

# 4. 运行（如果有 kernel 镜像）
./build/qemu-system-riscv32 -M g233 -nographic -kernel <your-kernel-image>
```

---

## 调试技巧

### 如果编译错误
1. 检查头文件是否正确引用
2. 确认所有函数调用的参数顺序正确
3. 验证类型转换宏使用是否正确（如 `OBJECT()`, `DEVICE()`）

### 如果运行时错误
1. 使用 `-d guest_errors` 查看客户机错误
2. 使用 `-d in_asm` 查看执行的指令
3. 检查内存映射是否正确

### 常见问题
- **忘记 realize**: 对象创建后必须 realize 才能使用
- **顺序错误**: 先 init，后 realize
- **类型转换**: 注意使用正确的类型转换宏

---

## 学习路径建议

1. ✅ 阅读完整指南 `G233_COMPLETION_GUIDE.md`
2. ✅ 理解 QOM 对象模型基础
3. ✅ 补全代码（按位置 1→2→3→4 的顺序）
4. ✅ 编译测试
5. ✅ 对比 `hw/riscv/sifive_e.c` 理解相似之处
6. ✅ 尝试添加新设备（进阶）

祝学习愉快！🚀

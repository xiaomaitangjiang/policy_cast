# Policy Cast Library

一个现代化的C++类型转换库，提供安全、灵活的类型转换机制，支持多种转换策略和编译时检查。

## 特性

### 🛡️ 安全第一
- **编译时检查**：在编译阶段阻止不安全的类型转换
- **策略驱动**：通过策略模板控制允许的转换类型
- **多态安全**：对多态类型使用`dynamic_cast`进行运行时类型检查
- **可扩展策略**：支持继承现有策略进行自定义

### 🔧 灵活的转换策略
- **安全模式（默认）**：平衡安全性和实用性
- **不安全模式**：允许所有转换，包括`reinterpret_cast`
- **严格模式**：最严格的转换限制，禁止潜在危险操作
- **自定义策略**：支持继承基础策略实现自定义规则

### 📚 丰富的转换支持
- 指针和引用类型转换
- 多态类型向上/向下转换
- 标准类型转换
- const限定符处理
- 指针与整数类型转换
- 函数指针转换
- 成员指针转换

### 🎯 便捷的接口
- **模板函数**：`policy_cast<To, Policy, From>()`
- **便捷别名**：`policy_cast_safe`, `policy_cast_unsafe`, `policy_cast_strict`
- **错误处理版本**：`try_policy_cast`（返回`std::optional`）
- **宏模式支持**：可通过宏保证类型转换路径绝对正确

## 快速开始

### 基本用法
```cpp
// 只需要包含一个头文件
include "policy_cast.hpp"

// 安全向上转换
Derived* derived = new Derived();
Base base = policy_cast<Base>(derived);

// 多态向下转换（运行时检查）
Base* poly_base = new Derived();
if (auto result = try_policy_cast<Derived*>(poly_base)) {
    result->foo(); // 安全调用
}
```

## 策略示例

```cpp
// 默认安全策略
int x = 42;
uintptr_t int_ptr = policy_cast<uintptr_t>(&x);

// 不安全策略（允许reinterpret_cast）
int ptr = policy_cast_unsafe<int>(int_ptr);

// 严格策略（禁止危险转换）
const int y = 100;
// int& ref = policy_cast_strict<int&>(y); // 编译错误：禁止去const
```

## 转换策略对比

| 转换类型 | 安全模式 | 不安全模式 | 严格模式 |
|---------|---------|-----------|---------|
| 相同类型转换 | ✅ | ✅ | ✅ |
| const去除 | ✅ | ✅ | ❌ |
| 多态向上转换 | ✅ | ✅ | ✅ |
| 多态向下转换 | ✅（dynamic_cast） | ✅（dynamic_cast） | ❌ |
| 非多态向下转换 | ❌ | ✅ | ❌ |
| 标准转换 | ✅ | ✅ | ✅ |
| reinterpret_cast | ❌ | ✅ | ❌ |
| 指针-整数转换 | ✅（仅标准整数） | ✅ | ❌ |
| 函数指针转换 | ❌ | ✅ | ❌ |

## 详细用法

### 1. 基本类型转换
```cpp
int a = 42;
double b = policy_cast<double>(a); // 标准转换

const char* str = "hello";
char mutable_str = policy_cast<char>(str); // 安全模式允许去const
```

### 2. 类继承转换
```cpp
class Base { virtual ~Base() = default; };
class Derived : public Base {};

// 向上转换 - 总是安全
Derived* d = new Derived();
Base b = policy_cast<Base>(d);

// 向下转换 - 多态类型使用dynamic_cast
Base* base_ptr = new Derived();
Derived derived_ptr = policy_cast<Derived>(base_ptr); // 运行时检查
```

### 3. 指针与整数转换
```cpp
int value = 42;
int* ptr = &value;

// 指针到整数（安全模式允许，仅标准整数）
uintptr_t int_val = policy_cast<uintptr_t>(ptr);

// 整数到指针（需要不安全模式）
int ptr2 = policy_cast_unsafe<int>(int_val);

```

### 4. 错误处理
```cpp
Base* base = new Base(); // 不是Derived类型

// 返回std::optional，不会抛出异常
if (auto result = try_policy_cast<Derived*>(base)) {
    // 转换成功
    result->some_method();
} else {
    // 转换失败
    std::cout << "转换失败\n";
}

```

### 5. 宏模式（精确类型控制）
```cpp
// 启用宏模式
define MACRO_MODE true

// 使用宏进行转换，确保类型精确匹配
int x = 42;
auto ptr = POLICY_CAST_SAFE(int*, &x);  // 明确指定From类型

```

## 自定义策略
```cpp
// 继承现有策略并自定义
struct my_custom_policy : policy_cast::safe_policy {
    // 允许非多态向下转换
    template<typename Subcategory>
    static constexpr bool allow_subcategory() {
        if constexpr (std::is_same_v<Subcategory, 
                   policy_cast::util::type_category::subcategory::down_cast_non_polymorphic_tag>) {
            return true;  // 允许非多态向下转换
        }
        return policy_cast::safe_policy::allow_subcategory<Subcategory>();
    }
    
    // 自定义其他规则
    static constexpr bool allow_user_explicit = false;  // 禁止用户显式转换
};

// 使用自定义策略
int result = policy_cast<int, my_custom_policy>(some_value);

```

## 编译要求
- C++14 或更高版本
- 支持标准库 `<type_traits>` `<optional>` `<cstdint>`

## 编译器支持
- GCC 5.0+（C++14模式）
- Clang 3.4+（C++14模式）
- MSVC 2017+

## 项目结构

policy_cast/
├── include/
│   ├── policy_cast.hpp          # 主头文件
│   └── policy_cast_util.hpp     # 类型工具
├── examples/
│   └── main.cpp                 # 使用示例
├── tests/
│   └── test_policy_cast.cpp     # 单元测试
└── README.md


## 构建和使用

### 1. 头文件包含
```cpp
// 只需要包含主头文件
include "policy_cast.hpp"
```


### 2. 编译选项
bash
GCC/Clang

g++ -std=c++14 -I./include your_program.cpp

MSVC

cl /std:c++14 /Iinclude your_program.cpp


### 3. 配置选项
可以通过定义以下宏来配置库的行为：
```cpp
define MACRO_MODE true  // 启用宏模式

```


## 注意事项
1. **多态类型要求**：向下转换需要基类有虚函数（多态类型）
2. **引用类型**：引用转换失败会抛出`std::bad_cast`
3. **编译时错误**：违反策略的转换会在编译时报错
4. **性能考虑**：多态向下转换有运行时开销
5. **宏模式**：启用宏模式可确保类型完全匹配，但语法稍显复杂
6. **C++版本**：C++14下错误处理功能受限

## 许可证
本项目采用MIT许可证。详情见LICENSE文件。

## 贡献
欢迎提交Issue和Pull Request来改进这个库。

## 版本历史
### v1.0.0
- 初始版本，支持安全、不安全、严格三种策略模式
- 支持基本类型转换、类继承转换、指针整数、函数指针和成员指针转换支持
- 提供编译时检查和运行时检查
- 支持C++14标准

### v1.1.0
- 重构策略系统，支持继承和自定义
- 改进编译时错误信息
- 添加更多示例和文档
- 提升跨平台兼容性
- 优化类型分类系统，支持更多转换类型

### v1.2.0
- 添加宏模式支持，提供精确类型控制

## 支持和反馈
如果您在使用过程中遇到问题或有建议：
1. 在GitHub Issues中报告问题
2. 参与讨论和改进

**Policy Cast Library - 让类型转换既安全又灵活**

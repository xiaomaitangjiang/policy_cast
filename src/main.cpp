#include <cstdint>
#include <iostream>

#include "../inc/policy_cast.hpp"

/**
 * @file main.cpp
 * @brief 演示 policy_cast 库的不同转换策略
 *
 * 本文件展示了如何使用 policy_cast 库进行类型安全转换，
 * 包括默认安全策略、不安全策略、严格策略和自定义策略。
 */

/**
 * @class Base
 * @brief 基类，具有虚函数用于演示动态转换
 */
class Base
{
public:
  virtual ~Base() = default;
  virtual void foo() { std::cout << "Base::foo()\n"; }
};

/**
 * @class Derived
 * @brief 派生类，重写基类的虚函数
 */
class Derived : public Base
{
public:
  void foo() override { std::cout << "Derived::foo()\n"; }
};

/**
 * @class NonPolymorphicBase
 * @brief 非多态基类，用于演示非动态向下转换
 */
class NonPolymorphicBase
{
};

/**
 * @class NonPolymorphicDerived
 * @brief 非多态派生类，继承自非多态基类
 */
class NonPolymorphicDerived : public NonPolymorphicBase
{
};

/**
 * @struct my_policy
 * @brief 自定义策略，继承自安全策略基类
 *
 * 该策略允许去除 const 限定符，但禁止 reinterpret 转换和非多态向下转换。
 */
struct my_policy : policy_cast::cast_policy_base<policy_cast::safe_cast_tag>
{
  static constexpr bool allow_reinterpret =
      false;  ///< 禁止 reinterpret_cast 转换
  static constexpr bool allow_const_removal = true;  ///< 允许去除 const 限定符
  static constexpr bool allow_non_polymorphic_downcast =
      false;  ///< 禁止非多态向下转换
  static constexpr bool allow_standard_pointer_integer_cast =
      true;  ///< 允许指针与标准整数类型之间的转换
  static constexpr bool allow_user_explicit = true;  ///< 允许显式用户定义转换
};

/**
 * @brief 演示不同策略下的类型转换
 *
 * 该函数展示了 policy_cast 库的多种使用场景：
 * 1. 默认安全策略
 * 2. 不安全策略（允许所有转换）
 * 3. 严格策略（限制某些转换）
 * 4. 非多态向下转换
 * 5. 运行时安全转换（返回 optional）
 * 6. 自定义策略
 */
void demonstrate_different_policies()
{
  std::cout << "=== 演示不同策略的auto_cast ===\n";

  /// 1. 默认安全策略（安全模式）
  std::cout << "1. 默认策略（安全模式）:" << "\n";

  int x = 42;
  int* ptr = &x;

  // 1.标准转换:指针->地址长度整数
  uintptr_t int_ptr1 =
      policy_cast::policy_cast<uintptr_t>(ptr);  // ��׼ת��
  std::cout << "  标准转换 ptr的地址: " << int_ptr1 << "\n";

  // 安全模式禁止reinterpret_cast
  // 以下代码在编译时会报错：
  // int* ptr2 = policy_cast::policy_cast<int*>(int_ptr1);
  // //错误：不允许reinterpret_cast

  /// 2. 不安全策略
  std::cout << "\n2. 不安全模式:\n";

  // 明确使用不安全模式
  int* ptr3 =
      policy_cast::policy_cast<int*, policy_cast::unsafe_policy>(int_ptr1);
  std::cout << "   reinterpret_cast允许: ptr3= " << ptr3 << "\n ";

  // 或者使用便捷别名
  int* ptr4 = policy_cast::policy_cast_unsafe<int*>(int_ptr1);
  std::cout << "   便捷别名: ptr4 = " << ptr4 << "\n";

  /// 3. 严格策略
  std::cout << "\n3. 严格模式:\n";

  const int y = 100;
  const int& h=y;
  // 严格模式允许的转换
  const std::int16_t int_y =
      policy_cast::policy_cast_strict<const int>(y);  // 允许，没有去const
  std::cout << "   标准转换: " << int_y << "\n";
  std::cout << (!std::is_same_v<const int, const int> &&
                std::is_same_v<std::remove_cv_t<const int>,
                               std::remove_cv_t<const int>>)
            << "\n";
  // 严格模式禁止的转换
  // 以下代码在编译时会报错：

  // int& ref_y = policy_cast::policy_cast_strict<int&>(y);  //
  //  错误：不允许去const

  /// 4. 非多态向下转换
  std::cout << "\n4. �Ƕ�̬����ת��:\n";

  NonPolymorphicDerived npd;
  NonPolymorphicBase* npb = &npd;

  // 不安全模式允许
  NonPolymorphicDerived* npd2 =
      policy_cast::policy_cast<NonPolymorphicDerived*,
                               policy_cast::unsafe_policy>(npb);
  std::cout << "   不安全模式允许非多态向下转换\n";
  // 安全模式下不允许非多态向下转换,因为这一行为一般是不安全的

  // 严格模式禁止
  // 以下代码在编译时会报错：
  // NonPolymorphicDerived* npd3 =
  //    policy_cast::policy_cast_strict<NonPolymorphicDerived*>(npb);  //
  //     错误：不允许非多态向下转换

  // 5. 运行时安全版本
  std::cout << "\n5. ����ʱ��ȫ�汾:\n";

  Base* base = new Derived();

// cpp17������,ʹ��try_auto_cast������optional
#if CPP_17
  if (auto derived = policy_cast::try_policy_cast_safe<Derived*>(base)) {
    std::cout << "   ת���ɹ�\n";
    (*derived)->foo();
  }
#else
  if (auto derived = policy_cast::try_policy_cast_safe<Derived*>(base)) {
    std::cout << "   ת���ɹ�\n" << "\n";
    derived->foo();
  }
#endif

  // auto derived3 = policy_cast::try_policy_cast_safe<Derived&>(*base);

  // 错误的向下转换
  Base* base2 = new Base();
  if (auto derived2 = policy_cast::try_policy_cast_safe<Derived*>(base2)) {
    std::cout << "   转换成功（不应该打印）\n";
  }
  else {
    std::cout << "   转换失败，返回空对象\n";
  }

  /// 6. 自定义策略示例
  std::cout << "\n6. �������ʾ��:\n";

  const int z = 200;
  int ref_z = policy_cast::policy_cast<int, my_policy>(z);
  std::cout << "   �Զ�����ԣ�����ȥconst����ֹreinterpret�ͷǶ�̬����ת��:" << ref_z
            << "\n";

  delete base;
  delete base2;
}

/**
 * @brief 主函数，运行策略演示
 * @return 程序退出状态码
 */
int main()
{
  system("chcp 65001>nul");
  demonstrate_different_policies();
  return 0;
}
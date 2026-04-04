#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace policy_cast::type_category {

// 一级分类标签
namespace category {
struct same_type_tag
{
};
struct qualifier_change_tag
{
};
struct standard_tag
{
};
struct pointer_hierarchy_tag
{
};
struct pointer_integer_tag
{
};
struct reinterpret_tag
{
};
struct invalid_tag
{
};
}  // namespace category

// 二级分类标签
namespace subcategory {
// 限定符变化
struct pr_const_removal_tag
{
};
struct volatile_removal_tag
{
};
struct cv_removal_tag
{
};
struct cv_addition_tag
{
};

// 标准转换
struct integral_promotion_tag
{
};
struct integral_conversion_tag
{
};
struct floating_promotion_tag
{
};
struct floating_conversion_tag
{
};
struct arithmetic_conversion_tag
{
};
struct enum_to_integral_tag
{
};
struct null_pointer_tag
{
};
struct void_pointer_tag
{
};
struct derived_to_base_tag
{
};
struct array_to_pointer_tag
{
};
struct function_to_pointer_tag
{
};
struct bool_conversion_tag
{
};

// 指针层级
struct up_cast_tag
{
};
struct down_cast_polymorphic_tag
{
};
struct down_cast_non_polymorphic_tag
{
};
struct cross_cast_tag
{
};

// 指针整数
struct standard_pointer_integer_tag
{
};
struct generic_pointer_integer_tag
{
};

// 重新解释
struct unrelated_pointer_tag
{
};
struct pointer_to_function_tag
{
};
struct function_pointer_to_pointer_tag
{
};
struct member_pointer_tag
{
};
}  // namespace subcategory

}  // namespace policy_cast::type_category

namespace policy_cast::util {
// 指针类型检测
template <typename T>
concept is_pointer_like = std::is_pointer_v<T>;

template <typename T>
constexpr bool is_pointer_like_v = is_pointer_like<T>;

template <typename T1, typename T2>
constexpr bool is_both_pointer = is_pointer_like_v<T1> && is_pointer_like_v<T2>;

// 引用类型检测
template <typename T>
concept is_reference_like = std::is_reference_v<T>;

template <typename T>
constexpr bool is_reference_like_v = is_reference_like<T>;

template <typename T1, typename T2>
constexpr bool is_both_reference =
    is_reference_like_v<T1> && is_reference_like_v<T2>;

template <typename policy>
concept has_policy_bool = requires {
  requires std::same_as<decltype(policy::allow_reinterpret), const bool>;
  requires std::same_as<decltype(policy::allow_const_removal), const bool>;
  requires std::same_as<decltype(policy::allow_non_polymorphic_downcast),
                        const bool>;
  requires std::same_as<decltype(policy::allow_standard_pointer_integer_cast),
                        const bool>;
  requires std::same_as<decltype(policy::allow_user_explicit), const bool>;
};

template <typename T>
using remove_cv_ptr_t = std::remove_cv_t<std::remove_pointer_t<T>>;

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
concept is_cast_policy = requires { requires has_policy_bool<T>; };

template <typename T>
concept standard_pointer_integer =
    std::is_same_v<T, std::intptr_t> || std::is_same_v<T, std::uintptr_t>;

template <typename From, typename To>
concept PointerToStandardInteger =
    std::is_pointer_v<From> && standard_pointer_integer<To>;

template <typename From, typename To>
concept StandardIntegerToPointer =
    standard_pointer_integer<From> && std::is_pointer_v<To>;

template <typename From, typename To>
concept is_standard_pointer_integer_conversion =
    PointerToStandardInteger<From, To> || StandardIntegerToPointer<From, To>;

}  // namespace policy_cast::util

namespace policy_cast::type_check {
/*
/ ͨ������ת���������
*/

template <typename T>
inline constexpr bool is_standard_int_v =
    std::is_same_v<T, std::intptr_t> || std::is_same_v<T, std::uintptr_t>;

template <typename T>
inline constexpr bool is_function_pointer_v =
    std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

template <typename From, typename To>
constexpr bool is_same_type()
{
  return std::is_same_v<From, To>;
}

template <typename From, typename To>
constexpr bool is_const_removal()
{
  return std::is_same_v<std::remove_const_t<From>, std::remove_const_t<To>> &&
         std::is_const_v<From> && !std::is_const_v<To>;
}

template <typename From, typename To>
constexpr bool is_add_const()
{
  return std::is_same_v<std::remove_const_t<From>, std::remove_const_t<To>> &&
         !std::is_const_v<From> && std::is_const_v<To>;
}

// ���η��������
template <typename From, typename To>
constexpr bool is_qualification_adjustment()
{
  // �ײ����ͱ�����ͬ
  if constexpr (!std::is_same_v<util::remove_cvref_t<From>,
                                util::remove_cvref_t<To>>) {
    return false;
  }
  // �����ǿ�����ʽת�����ʸ����
  if constexpr (!std::is_convertible_v<From, To>) {
    return false;
  }

  // ����Ƿ���cv�޶����ı仯
  constexpr bool cv_changed =
      (std::is_const_v<From> != std::is_const_v<To>) ||
      (std::is_volatile_v<From> != std::is_volatile_v<To>);

  // ����Ƿ����������͵ı仯
  constexpr bool ref_changed =
      std::is_reference_v<From> != std::is_reference_v<To>;

  // ����������һ�������仯
  return cv_changed || ref_changed;
}

// �����������
template <typename From, typename To>
constexpr bool is_integral_promotion()
{
  if constexpr (!std::is_integral_v<From> || !std::is_integral_v<To>) {
    return false;
  }

  constexpr size_t from_size = sizeof(From);
  constexpr size_t to_size = sizeof(To);
  constexpr bool from_signed = std::is_signed_v<From>;
  constexpr bool to_signed = std::is_signed_v<To>;

  //  bool ���κ���������
  if constexpr (std::is_same_v<From, bool>) {
    return true;
  }

  //  char �� int/unsigned int
  if constexpr (std::is_same_v<From, char>) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  //  signed char �� int
  if constexpr (std::is_same_v<From, signed char>) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  //  unsigned char �� int (������Ա�ʾ����ֵ)
  if constexpr (std::is_same_v<From, unsigned char>) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  //  short �� int
  if constexpr (std::is_same_v<From, short>) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  //  unsigned short �� int (������Ա�ʾ����ֵ)
  if constexpr (std::is_same_v<From, unsigned short>) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  //  С�������͵�int�ĳ�������
  if constexpr (from_size < sizeof(int)) {
    if constexpr (to_size == sizeof(int) && to_signed) {
      return true;
    }
  }

  return false;
}

//  �����������
template <typename From, typename To>
constexpr bool is_floating_promotion()
{
  // float �� double
  if constexpr (std::is_same_v<From, float> && std::is_same_v<To, double>) {
    return true;
  }

  // double �� long double
  if constexpr (std::is_same_v<From, double> &&
                std::is_same_v<To, long double>) {
    return true;
  }

  return false;
}

//  ���鵽ָ��ת�����
template <typename From, typename To>
constexpr bool is_array_to_pointer()
{
  if constexpr (!std::is_array_v<From> || !std::is_pointer_v<To>) {
    return false;
  }

  // �������Ԫ��������ָ��Ŀ�������Ƿ�ƥ�䣨����cv�޶�����
  using element_type = std::remove_extent_t<From>;
  using pointer_type = std::remove_pointer_t<To>;

  return std::is_same_v<std::remove_cv_t<element_type>,
                        std::remove_cv_t<pointer_type>>;
}

// 6. ������ָ��ת�����
template <typename From, typename To>
constexpr bool is_function_to_pointer()
{
  if constexpr (!std::is_function_v<From> || !std::is_pointer_v<To>) {
    return false;
  }

  // ��麯��������ָ��Ŀ�������Ƿ�ƥ��
  using pointer_type = std::remove_pointer_t<To>;
  return std::is_same_v<From, pointer_type>;
}

// ��ָ��ת�����
template <typename From, typename To>
constexpr bool is_nullptr_conversion()
{
  // std::nullptr_t ���κ�ָ������
  if constexpr (std::is_same_v<From, std::nullptr_t> && std::is_pointer_v<To>) {
    return true;
  }

  // ����0
  // ��ָ�����ͣ�C��񣬵�C++�������ض��������У�
  if constexpr (std::is_integral_v<From> && std::is_pointer_v<To>) {
    // ��Ҫ����ʱ���From�Ƿ���ֵΪ0����������
    // ����Ĭ�Ϸ���false
    return false;
  }

  return false;
}

// ����ת����飨������������
// policy_cast��δʹ��,��Ϊ�䲻��Ҫ�����������ת��
template <typename From, typename To>
constexpr bool is_arithmetic_conversion()
{
  if constexpr (!std::is_arithmetic_v<From> || !std::is_arithmetic_v<To>) {
    return false;
  }

  // �ų��Ѿ�����������
  if constexpr (is_integral_promotion<From, To>() ||
                is_floating_promotion<From, To>()) {
    return false;
  }

  // �ų���ͬ����
  if constexpr (std::is_same_v<util::remove_cvref_t<From>,
                               util::remove_cvref_t<To>>) {
    return false;
  }

  // ����Ƿ����ʽת��
  if constexpr (!std::is_convertible_v<From, To>) {
    return false;
  }

  return true;
}

// ָ��ת����飨�����̳У�
template <typename From, typename To>
constexpr bool is_pointer_conversion()
{
  if constexpr (!std::is_pointer_v<From> || !std::is_pointer_v<To>) {
    return false;
  }

  using PFrom = std::remove_pointer_t<From>;
  using PTo = std::remove_pointer_t<To>;

  // �ų��̳й�ϵ
  if constexpr (std::is_base_of_v<PTo, PFrom> ||
                std::is_base_of_v<PFrom, PTo>) {
    return false;
  }

  // �ų�void*ת��
  if constexpr (std::is_void_v<PFrom> || std::is_void_v<PTo>) {
    return false;
  }

  // ����Ƿ������ʽת������׼ָ��ת����������const��
  if constexpr (std::is_convertible_v<From, To>) {
    return true;
  }

  return false;
}

// ����ת�����
template <typename From, typename To>
constexpr bool is_reference_conversion()
{
  if constexpr (!std::is_reference_v<From> || !std::is_reference_v<To>) {
    return false;
  }

  // ����Ƿ������ð�ת��
  if constexpr (std::is_convertible_v<From, To>) {
    return true;
  }

  return false;
}

// ����ת�����
template <typename From, typename To>
constexpr bool is_up_cast()
{
  // ����ָ��
  if constexpr (std::is_pointer_v<util::remove_cvref_t<From>> &&
                std::is_pointer_v<util::remove_cvref_t<To>>) {
    using PFrom = std::remove_pointer_t<util::remove_cvref_t<From>>;
    using PTo = std::remove_pointer_t<util::remove_cvref_t<To>>;

    // ����Ƿ��������ൽ����
    if constexpr (std::is_base_of_v<PTo, PFrom>) {
      return true;
    }
  }

  // ��������
  if constexpr (std::is_reference_v<From> && std::is_reference_v<To>) {
    using RFrom = std::remove_reference_t<From>;
    using RTo = std::remove_reference_t<To>;
    // ����Ƿ��������ൽ����
    if constexpr (std::is_base_of_v<RTo, RFrom>) {
      return true;
    }
  }

  return false;
}

// ���¼��ת��
template <typename From, typename To>
constexpr bool is_down_cast()
{
  if constexpr (util::is_pointer_like_v<From> && util::is_pointer_like_v<To>) {
    return std::is_base_of_v<std::remove_pointer_t<From>,
                             std::remove_pointer_t<To>>;
  }
  else if constexpr (util::is_reference_like_v<From> &&
                     util::is_reference_like_v<To>) {
    return std::is_base_of_v<std::remove_reference_t<From>,
                             std::remove_reference_t<To>>;
  }
  return false;
}

// ��̬����ת�����
template <typename From, typename To>
constexpr bool is_polymorphic_cast()
{
  if constexpr (is_down_cast<From, To>()) {
    if constexpr (util::is_pointer_like_v<From>) {
      return std::is_polymorphic_v<std::remove_pointer_t<From>>;
    }
    else if constexpr (util::is_reference_like_v<From>) {
      return std::is_polymorphic_v<std::remove_reference_t<From>>;
    }
  }
  return false;
}
// �Ƕ�̬����ת�����
template <typename From, typename To>
constexpr bool is_non_polymorphic_downcast()
{
  return is_down_cast<From, To>() && !is_polymorphic_cast<From, To>();
}

// ��ʽ�û�����ת�����
template <typename From, typename To>
constexpr bool is_user_implicit_conversion()
{
  // ���������ʽת��
  if constexpr (!std::is_convertible_v<From, To>) {
    return false;
  }

  // �ų����б�׼ת��
  if constexpr (is_same_type<From, To>() ||
                is_qualification_adjustment<From, To>() ||
                is_integral_promotion<From, To>() ||
                is_floating_promotion<From, To>() ||
                is_array_to_pointer<From, To>() ||
                is_function_to_pointer<From, To>() ||
                is_arithmetic_conversion<From, To>() ||
                is_pointer_conversion<From, To>() ||
                is_reference_conversion<From, To>() || is_up_cast<From, To>()) {
    return false;
  }

  // ����һ��������������
  if constexpr (!std::is_class_v<util::remove_cvref_t<From>> &&
                !std::is_class_v<util::remove_cvref_t<To>>) {
    return false;
  }

  return true;
}

// ��ʽ�û�����ת�����
template <typename From, typename To>
constexpr bool is_user_explicit_conversion()
{
  // ������ʽת��
  if constexpr (std::is_convertible_v<From, To>) {
    return false;
  }

  // ����Ƿ����ͨ����ʽת������
  bool via_constructor = false;
  if constexpr (std::is_constructible_v<To, From>) {
    via_constructor = true;
  }

  // ����Ƿ����ת�������
  bool via_conversion_op = false;
  if constexpr (std::is_class_v<util::remove_cvref_t<From>>) {
    // ���Լ���Ƿ���operator To
    if constexpr (requires(From f) { static_cast<To>(f.operator To()); }) {
      via_conversion_op = true;
    }
  }

  if (!via_constructor && !via_conversion_op) {
    return false;
  }

  // ����һ��������������
  if constexpr (!std::is_class_v<util::remove_cvref_t<From>> &&
                !std::is_class_v<util::remove_cvref_t<To>>) {
    return false;
  }

  return true;
}

// 16. void*���ת�����
template <typename From, typename To>
constexpr bool is_void_pointer_conversion()
{
  if constexpr (!std::is_pointer_v<From> || !std::is_pointer_v<To>) {
    return false;
  }

  using PFrom = std::remove_pointer_t<From>;
  using PTo = std::remove_pointer_t<To>;

  // From �� void* �� const void* ��
  if constexpr (std::is_void_v<std::remove_cv_t<PFrom>>) {
    // ת��Ϊ����ָ������
    return true;
  }

  // To �� void* �� const void* ��
  if constexpr (std::is_void_v<std::remove_cv_t<PTo>>) {
    // ������ָ������ת��
    return true;
  }

  return false;
}

template <typename From, typename To>
constexpr bool is_pointer_to_standard_integer()
{
  return std::is_pointer_v<From> && (std::is_same_v<To, std::intptr_t> ||
                                     std::is_same_v<To, std::uintptr_t>);
}

template <typename From, typename To>
constexpr bool is_standard_integer_to_pointer()
{
  return (std::is_same_v<From, std::intptr_t> ||
          std::is_same_v<From, std::uintptr_t>) &&
         std::is_pointer_v<To>;
}

template <typename From, typename To>
constexpr bool is_standard_pointer_integer_conversion()
{
  return is_pointer_to_standard_integer<From, To>() ||
         is_standard_integer_to_pointer<From, To>();
}

template <typename From, typename To>
constexpr bool is_pointer_to_integer()
{
  return std::is_pointer_v<From> && std::is_integral_v<To>;
}

template <typename From, typename To>
constexpr bool is_integer_to_pointer()
{
  return std::is_integral_v<From> && std::is_pointer_v<To>;
}

template <typename From, typename To>
constexpr bool is_pointer_integer_conversion()
{
  return is_pointer_to_integer<From, To>() || is_integer_to_pointer<From, To>();
}

template <typename From, typename To>
constexpr bool is_unrelated_pointer_conversion()
{
  return util::is_pointer_like_v<From> && util::is_pointer_like_v<To>();
}

template <typename From, typename To>
constexpr bool is_reinterpret_conversion()
{
  return is_pointer_integer_conversion<From, To>() ||
         is_unrelated_pointer_conversion<From, To>();
}
}  // namespace policy_cast::type_check

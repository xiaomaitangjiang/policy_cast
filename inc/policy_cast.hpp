#pragma once

#include <cassert>
#include <optional>
#include <type_traits>
#include <typeinfo>

#include "policy_cast_util.hpp"

#define CPP_20 __cplusplus >= 202002
#define CPP_17 __cplusplus >= 201703
#define CPP_14 __cplusplus >= 201402
#define CPP_11 __cplusplus >= 201103

#define MACRO_MODE false //是否使用宏代替默认的类型猜测模式

namespace policy_cast {

template <typename From, typename To>
struct conversion_classifier
{
private:
  using raw_from = From;
  using raw_to = To;
  // 相同类型检查
  static constexpr bool is_same_type =
      type_check::is_same_type<raw_from, raw_to>();
  static constexpr bool is_convertible = std::is_convertible_v<From, To>;

  // 限定符变化检查
  static constexpr bool is_const_removal =
      (util::is_both_pointer<raw_from, raw_to> ||
       util::is_both_reference<raw_from, raw_to>) &&
      std::is_const_v<raw_from> && !std::is_const_v<raw_to> &&
      std::is_same_v<std::remove_const_t<raw_from>, raw_to>;

  static constexpr bool is_volatile_removal =
      (util::is_both_pointer<raw_from, raw_to> ||
       util::is_both_reference<raw_from, raw_to>) &&
      std::is_volatile_v<raw_from> && !std::is_volatile_v<raw_to> &&
      std::is_same_v<std::remove_volatile_t<raw_from>, raw_to>;

  static constexpr bool is_qualifier_change =
      is_const_removal || is_volatile_removal ||
      (!std::is_same_v<raw_from, raw_to> &&
       std::is_same_v<std::remove_cv_t<raw_from>, std::remove_cv_t<raw_to>>);

  // 指针类型检查
  static constexpr bool from_is_ptr =
      util::is_pointer_like_v<std::remove_reference_t<raw_from>>;
  static constexpr bool to_is_ptr =
      util::is_pointer_like_v<std::remove_reference_t<raw_to>>;
  static constexpr bool both_are_ptrs = from_is_ptr && to_is_ptr;

  // 指针类型提取
  using from_ptr_t = util::remove_cv_ptr_t<raw_from>;
  using to_ptr_t = util::remove_cv_ptr_t<raw_to>;

  // 多态转换检查
  static constexpr bool is_up_cast =
      both_are_ptrs && std::is_base_of_v<to_ptr_t, from_ptr_t>;
  static constexpr bool is_down_cast =
      both_are_ptrs && std::is_base_of_v<from_ptr_t, to_ptr_t>;
  static constexpr bool is_polymorphic = std::is_polymorphic_v<from_ptr_t>;

  // 指针->整数类型转换检查
  static constexpr bool is_standard_pointer_integer =
      (from_is_ptr && type_check::is_standard_int_v<raw_to>);
  // ||(type_check::is_standard_int_v<raw_from> && to_is_ptr);
  // 我认为整数->指针的转换是不安全的非标准行为

  static constexpr bool is_generic_pointer_integer =
      (from_is_ptr && std::is_integral_v<raw_to> &&
       !type_check::is_standard_int_v<raw_to>);
  //||(std::is_integral_v<raw_from> && to_is_ptr
  //&&!type_check::is_standard_int_v<raw_from>);
  // 我认为整数->指针的转换是不安全的非标准行为
  static constexpr bool is_pointer_integer =
      is_standard_pointer_integer || is_generic_pointer_integer;

  // 整数->指针类型转换检查
  static constexpr bool is_integer_pointer =
      std::is_integral_v<raw_from> && to_is_ptr;

  // 函数指针检查
  static constexpr bool from_is_func_ptr =
      type_check::is_function_pointer_v<raw_from>;
  static constexpr bool to_is_func_ptr =
      type_check::is_function_pointer_v<raw_to>;

  // 标准转换类型检查
  static constexpr bool is_integral_promotion =
      std::is_integral_v<raw_from> && std::is_integral_v<raw_to> &&
      sizeof(raw_from) < sizeof(raw_to) &&
      std::is_signed_v<raw_from> == std::is_signed_v<raw_to>;

  static constexpr bool is_integral_conversion = std::is_integral_v<raw_from> &&
                                                 std::is_integral_v<raw_to> &&
                                                 !is_integral_promotion;

  static constexpr bool is_floating_promotion =
      std::is_floating_point_v<raw_from> && std::is_floating_point_v<raw_to> &&
      sizeof(raw_from) < sizeof(raw_to);

  static constexpr bool is_array_to_pointer =
      std::is_array_v<raw_from> && to_is_ptr;

  static constexpr bool is_function_to_pointer =
      std::is_function_v<raw_from> && to_is_ptr;

  static constexpr bool is_enum_to_integral =
      std::is_enum_v<raw_from> && std::is_integral_v<raw_to>;

  static constexpr bool is_bool_conversion =
      !std::is_same_v<raw_from, bool> && std::is_same_v<raw_to, bool>;

public:
  // 一级分类：确定转换的主要类别
  static constexpr auto get_primary_category()
  {
    using namespace type_category;
    if constexpr (is_same_type) {
      return category::same_type_tag{};
    }
    else if constexpr (is_qualifier_change) {
      return category::qualifier_change_tag{};
    }
    else if constexpr (is_up_cast || is_down_cast) {
      return category::pointer_hierarchy_tag{};
    }
    else if constexpr (is_pointer_integer) {
      return category::pointer_integer_tag{};
    }
    else if constexpr (both_are_ptrs || from_is_func_ptr || to_is_func_ptr ||
                       is_integer_pointer) {
      return category::reinterpret_tag{};
    }
    else if constexpr (is_convertible) {
      return category::standard_tag{};
    }
    else {
      return category::invalid_tag{};
    }
  }

  // 二级分类：在主要分类下进一步细分
  template <typename PrimaryTag>
  static constexpr auto get_secondary_category_impl()
  {
    using namespace type_category;
    if constexpr (std::is_same_v<PrimaryTag, category::qualifier_change_tag>) {
      if constexpr (is_const_removal) {
        return subcategory::pr_const_removal_tag{};
      }
      else if constexpr (is_volatile_removal) {
        return subcategory::volatile_removal_tag{};
      }
      else {
        return subcategory::cv_addition_tag{};
      }
    }
    else if constexpr (std::is_same_v<PrimaryTag, category::standard_tag>) {
      if constexpr (is_integral_promotion) {
        return subcategory::integral_promotion_tag{};
      }
      else if constexpr (is_integral_conversion) {
        return subcategory::integral_conversion_tag{};
      }
      else if constexpr (is_floating_promotion) {
        return subcategory::floating_promotion_tag{};
      }
      else if constexpr (is_array_to_pointer) {
        return subcategory::array_to_pointer_tag{};
      }
      else if constexpr (is_function_to_pointer) {
        return subcategory::function_to_pointer_tag{};
      }
      else if constexpr (is_enum_to_integral) {
        return subcategory::enum_to_integral_tag{};
      }
      else if constexpr (is_bool_conversion) {
        return subcategory::bool_conversion_tag{};
      }
      else {
        return subcategory::arithmetic_conversion_tag{};
      }
    }
    else if constexpr (std::is_same_v<PrimaryTag,
                                      category::pointer_hierarchy_tag>) {
      if constexpr (is_up_cast) {
        return subcategory::up_cast_tag{};
      }
      else if constexpr (is_down_cast && is_polymorphic) {
        return subcategory::down_cast_polymorphic_tag{};
      }
      else if constexpr (is_down_cast) {
        return subcategory::down_cast_non_polymorphic_tag{};
      }
      else {
        return subcategory::cross_cast_tag{};
      }
    }
    else if constexpr (std::is_same_v<PrimaryTag,
                                      category::pointer_integer_tag>) {
      if constexpr (is_standard_pointer_integer) {
        return subcategory::standard_pointer_integer_tag{};
      }
      else {
        return subcategory::generic_pointer_integer_tag{};
      }
    }
    else if constexpr (std::is_same_v<PrimaryTag, category::reinterpret_tag>) {
      if constexpr (from_is_func_ptr) {
        return subcategory::function_pointer_to_pointer_tag{};
      }
      else if constexpr (to_is_func_ptr) {
        return subcategory::pointer_to_function_tag{};
      }
      else if constexpr (to_is_ptr) {
        return subcategory::integer_to_pointer_tag{};
      }
      else if constexpr (std::is_member_pointer_v<raw_from> ||
                         std::is_member_pointer_v<raw_to>) {
        return subcategory::member_pointer_tag{};
      }
      else {
        return subcategory::unrelated_pointer_tag{};
      }
    }
    else {
      struct invalid_subcategory_tag
      {
      };
      return invalid_subcategory_tag{};
    }
  }

  // 类型别名
  using primary_category = decltype(get_primary_category());
  using secondary_category =
      decltype(get_secondary_category_impl<primary_category>());

  // 有效性检查
  static constexpr bool is_valid =
      !std::is_same_v<primary_category, type_category::category::invalid_tag> &&
      !std::is_same_v<secondary_category, struct invalid_subcategory_tag>;
};

/*

*/

template <typename From, typename To>
struct conversion_traits
{
  using primary_category =
      typename conversion_classifier<From, To>::primary_category;
  using secondary_category =
      typename conversion_classifier<From, To>::secondary_category;
  static constexpr bool is_valid = conversion_classifier<From, To>::is_valid;
};

template <typename PolicyTag>
struct cast_policy_base
{
  using tag = PolicyTag;

  // 一级分类控制
  static constexpr bool allow_same_type = true;
  static constexpr bool allow_qualifier_change = true;
  static constexpr bool allow_standard = true;
  static constexpr bool allow_pointer_hierarchy = true;
  static constexpr bool allow_pointer_integer = true;
  static constexpr bool allow_reinterpret = false;

  // 二级分类控制 - 使用默认值允许所有二级分类
  template <typename Subcategory>
  static constexpr bool allow_subcategory()
  {
    return true;
  }

  // ����Ƿ������?��
  template <typename From, typename To>
  static constexpr bool is_allowed()
  {
    using traits = conversion_traits<From, To>;
    using namespace type_category;

    if constexpr (!traits::is_valid) {
      return false;
    }

    // ���һ������?
    using primary = typename traits::primary_category;

    if constexpr (std::is_same_v<primary, category::same_type_tag>) {
      return allow_same_type;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::qualifier_change_tag>) {
      if constexpr (!allow_qualifier_change) return false;
    }
    else if constexpr (std::is_same_v<primary, category::standard_tag>) {
      if constexpr (!allow_standard) return false;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::pointer_hierarchy_tag>) {
      if constexpr (!allow_pointer_hierarchy) return false;
    }
    else if constexpr (std::is_same_v<primary, category::pointer_integer_tag>) {
      if constexpr (!allow_pointer_integer) return false;
    }
    else if constexpr (std::is_same_v<primary, category::reinterpret_tag>) {
      if constexpr (!allow_reinterpret) return false;
    }

    // ����������
    using secondary = typename traits::secondary_category;
    return allow_subcategory<secondary>();
  }
};

// ===============================
// 6. Ԥ������ԣ�ʹ�ü̳У�?
// ===============================

struct safe_cast_tag
{
};
struct unsafe_cast_tag
{
};
struct strict_cast_tag
{
};

// ��ȫ���ԣ�Ĭ�ϣ�
template <>
struct cast_policy_base<safe_cast_tag>
{
  static constexpr bool allow_same_type = true;
  static constexpr bool allow_qualifier_change = true;
  static constexpr bool allow_standard = true;
  static constexpr bool allow_pointer_hierarchy = true;
  static constexpr bool allow_pointer_integer = true;
  static constexpr bool allow_reinterpret = false;
  static constexpr bool allow_user_explicit = true;

  template <typename Subcategory>
  static constexpr bool allow_subcategory()
  {
    using namespace type_category;
    // �������ж������࣬�����ض��ļ���
    if constexpr (std::is_same_v<Subcategory,
                                 subcategory::down_cast_non_polymorphic_tag>) {
      return false;
    }
    else if constexpr (std::is_same_v<
                           Subcategory,
                           subcategory::generic_pointer_integer_tag>) {
      return false;
    }
    else {
      return true;
    }
  }

  template <typename From, typename To>
  static constexpr bool is_allowed()
  {
    using traits = conversion_traits<From, To>;
    using namespace type_category;

    if constexpr (!traits::is_valid) {
      return false;
    }

    // ���һ������?
    using primary = typename traits::primary_category;

    if constexpr (std::is_same_v<primary, category::same_type_tag>) {
      return allow_same_type;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::qualifier_change_tag>) {
      if constexpr (!allow_qualifier_change) return false;
    }
    else if constexpr (std::is_same_v<primary, category::standard_tag>) {
      if constexpr (!allow_standard) return false;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::pointer_hierarchy_tag>) {
      if constexpr (!allow_pointer_hierarchy) return false;
    }
    else if constexpr (std::is_same_v<primary, category::pointer_integer_tag>) {
      if constexpr (!allow_pointer_integer) return false;
    }
    else if constexpr (std::is_same_v<primary, category::reinterpret_tag>) {
      if constexpr (!allow_reinterpret) return false;
    }

    // ����������
    using secondary = typename traits::secondary_category;
    return allow_subcategory<secondary>();
  }
};

// ����ȫ����
template <>
struct cast_policy_base<unsafe_cast_tag>
{
  static constexpr bool allow_same_type = true;
  static constexpr bool allow_qualifier_change = true;
  static constexpr bool allow_standard = true;
  static constexpr bool allow_pointer_hierarchy = true;
  static constexpr bool allow_pointer_integer = true;
  static constexpr bool allow_reinterpret = true;
  static constexpr bool allow_user_explicit = true;

  // �������ж�������
  template <typename Subcategory>
  static constexpr bool allow_subcategory()
  {
    return true;
  }

  template <typename From, typename To>
  static constexpr bool is_allowed()
  {
    using traits = conversion_traits<From, To>;
    using namespace type_category;

    if constexpr (!traits::is_valid) {
      return false;
    }

    // ���һ������?
    using primary = typename traits::primary_category;

    if constexpr (std::is_same_v<primary, category::same_type_tag>) {
      return allow_same_type;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::qualifier_change_tag>) {
      if constexpr (!allow_qualifier_change) return false;
    }
    else if constexpr (std::is_same_v<primary, category::standard_tag>) {
      if constexpr (!allow_standard) return false;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::pointer_hierarchy_tag>) {
      if constexpr (!allow_pointer_hierarchy) return false;
    }
    else if constexpr (std::is_same_v<primary, category::pointer_integer_tag>) {
      if constexpr (!allow_pointer_integer) return false;
    }
    else if constexpr (std::is_same_v<primary, category::reinterpret_tag>) {
      if constexpr (!allow_reinterpret) return false;
    }

    // ����������
    using secondary = typename traits::secondary_category;
    return allow_subcategory<secondary>();
  }
};

// �ϸ����?
template <>
struct cast_policy_base<strict_cast_tag>
{
  static constexpr bool allow_same_type = true;
  static constexpr bool allow_qualifier_change = false;
  static constexpr bool allow_standard = true;
  static constexpr bool allow_pointer_hierarchy = false;
  static constexpr bool allow_pointer_integer = false;
  static constexpr bool allow_reinterpret = false;
  static constexpr bool allow_user_explicit = true;

  template <typename Subcategory>
  static constexpr bool allow_subcategory()
  {
    using namespace type_category;

    if constexpr (std::is_same_v<Subcategory,
                                 subcategory::pr_const_removal_tag>) {
      return false;
    }
    else if constexpr (std::is_same_v<Subcategory,
                                      subcategory::down_cast_polymorphic_tag>) {
      return false;
    }
    else if constexpr (std::is_same_v<Subcategory,
                                      subcategory::bool_conversion_tag>) {
      return false;
    }
    else {
      return true;
    }
  }

  template <typename From, typename To>
  static constexpr bool is_allowed()
  {
    using traits = conversion_traits<From, To>;
    using namespace type_category;

    if constexpr (!traits::is_valid) {
      return false;
    }

    // ���һ������?
    using primary = typename traits::primary_category;

    if constexpr (std::is_same_v<primary, category::same_type_tag>) {
      return allow_same_type;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::qualifier_change_tag>) {
      if constexpr (!allow_qualifier_change) return false;
    }
    else if constexpr (std::is_same_v<primary, category::standard_tag>) {
      if constexpr (!allow_standard) return false;
    }
    else if constexpr (std::is_same_v<primary,
                                      category::pointer_hierarchy_tag>) {
      if constexpr (!allow_pointer_hierarchy) return false;
    }
    else if constexpr (std::is_same_v<primary, category::pointer_integer_tag>) {
      if constexpr (!allow_pointer_integer) return false;
    }
    else if constexpr (std::is_same_v<primary, category::reinterpret_tag>) {
      if constexpr (!allow_reinterpret) return false;
    }

    // ����������
    using secondary = typename traits::secondary_category;
    return allow_subcategory<secondary>();
  }
};

// �û��ӿ� - �������ͱ���
using default_policy = cast_policy_base<safe_cast_tag>;
using unsafe_policy = cast_policy_base<unsafe_cast_tag>;
using strict_policy = cast_policy_base<strict_cast_tag>;

template <typename To, typename From, typename Policy = default_policy>
class policy_cast_impl
{
private:
  using traits = conversion_traits<From, To>;

  static_assert(traits::is_valid, "Invalid type conversion");
  static_assert(Policy::template is_allowed<From, To>(),
                "Conversion not allowed by current policy");

public:
  static To cast(From from)
  {
    using namespace type_category;
    using namespace type_category::category;
    using namespace type_category::subcategory;
    using primary = typename traits::primary_category;
    using secondary = typename traits::secondary_category;

    // ��ͬ����
    if constexpr (std::is_same_v<primary, same_type_tag>) {
      return static_cast<To>(from);
    }
    // �޶����仯
    else if constexpr (std::is_same_v<primary, qualifier_change_tag>) {
      if constexpr (std::is_same_v<secondary, pr_const_removal_tag> ||
                    std::is_same_v<secondary, cv_removal_tag>) {
        return const_cast<To>(from);
      }
      else {
        return static_cast<To>(from);
      }
    }
    // ��׼ת��
    else if constexpr (std::is_same_v<primary, standard_tag>) {
      return static_cast<To>(from);
    }
    // ָ����ת��
    else if constexpr (std::is_same_v<primary, pointer_hierarchy_tag>) {
      if constexpr (std::is_same_v<secondary, up_cast_tag> ||
                    std::is_same_v<secondary, down_cast_non_polymorphic_tag>) {
        return static_cast<To>(from);
      }
      else {
        auto result = dynamic_cast<To>(from);
        if (!result && from) throw std::bad_cast();
        return result;
      }
    }
    // ָ������ת�������½���ת��
    else if constexpr (std::is_same_v<primary, pointer_integer_tag> ||
                       std::is_same_v<primary, reinterpret_tag>) {
      return reinterpret_cast<To>(from);
    }

    // ��Ӧ�õ�������
    else {
      static_assert(sizeof(From) == 0, "Unhandled conversion type");
      return To{};
    }
  }
};

#if MACRO_MODE == true

template <typename To, typename Policy = default_policy, typename From>
To policy_cast(From&& from)
{
  return policy_cast_impl<To, From, Policy>::cast(std::forward<From>(from));
}

#define POLICY_CAST(T, From, Policy) \
  policy_cast<T, Policy, decltype(From)>(From)
#define POLICY_CAST_SAFE(T, From) \
  policy_cast<T, default_policy, decltype(From)>(From)
#define POLICY_CAST_UNSAFE(T, From) \
  policy_cast<T, unsafe_policy, decltype(From)>(From)
#define POLICY_CAST_STRICT(T, From) \
  policy_cast<T, strict_policy, decltype(From)>(From)

#elif MACRO_MODE == false

template <typename To, typename Policy = default_policy, typename From>
To policy_cast(From&& from)
{
  if constexpr (std::is_lvalue_reference_v<To>) {
    using Guess_From_T = std::remove_reference_t<From>&;

    return policy_cast_impl<To, Guess_From_T, Policy>::cast(
        std::forward<From>(from));
  }
  else if constexpr (std::is_rvalue_reference_v<To>) {
    using Guess_From_T = std::remove_reference_t<From>&&;

    return policy_cast_impl<To, Guess_From_T, Policy>::cast(
        std::forward<From>(from));
  }
  else {
    using Guess_From_T = std::remove_reference_t<From>;
    return policy_cast_impl<To, Guess_From_T, Policy>::cast(
        std::forward<From>(from));
  }
}

#endif

template <typename To, typename From>
To policy_cast_safe(From&& from)
{
  return policy_cast<To, default_policy>(std::forward<From>(from));
}

template <typename To, typename From>
To policy_cast_unsafe(From&& from)
{
  return policy_cast<To, unsafe_policy>(std::forward<From>(from));
}

template <typename To, typename From>
To policy_cast_strict(From&& from)
{
  return policy_cast<To, strict_policy>(std::forward<From>(from));
}

#if CPP_17&&MACRO_MODE==false
template <typename To, typename Policy = default_policy, typename From>
std::optional<To> try_policy_cast(From&& from)
{
  try {
    return policy_cast<To, Policy>(std::forward<From>(from));
  } catch (const std::bad_cast&) {
    return std::nullopt;
  }
}

template <typename To, typename From>
std::optional<To> try_policy_cast_safe(From&& from)
{
  return try_policy_cast<To, default_policy>(std::forward<From>(from));
}

template <typename To, typename From>
std::optional<To> try_policy_cast_unsafe(From&& from)
{
  return try_policy_cast<To, unsafe_policy>(std::forward<From>(from));
}

template <typename To, typename From>
std::optional<To> try_policy_cast_strict(From&& from)
{
  return try_policy_cast<To, strict_policy>(std::forward<From>(from));
}
#elif MACRO_MODE == false

template <typename To, typename Policy = default_policy, typename From>
To* try_policy_cast(From from)
{
  static_assert(std::is_pointer_v<To>,
                "try_policy_cast in C++14 mode only works with pointer types");
  try {
    return &policy_cast<To, Policy>(from);
  } catch (const std::bad_cast&) {
    return nullptr;
  }
}

template <typename To, typename From>
To* try_policy_cast_safe(From from)
{
  return try_policy_cast<To, default_policy>(from);
}

template <typename To, typename From>
To* try_policy_cast_unsafe(From from)
{
  return try_policy_cast<To, unsafe_policy>(from);
}

template <typename To, typename From>
To* try_policy_cast_strict(From from)
{
  return try_policy_cast<To, strict_policy>(from);
}
#endif

}  // namespace policy_cast
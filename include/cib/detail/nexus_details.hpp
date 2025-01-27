#pragma once

#include <cib/detail/exports.hpp>
#include <cib/detail/meta.hpp>
#include <cib/detail/type_list.hpp>
#include <cib/set.hpp>
#include <cib/tuple.hpp>

#include <type_traits>
#include <utility>

namespace cib {
struct extract_service_tag {
    template <typename T> using invoke = typename T::Service;
};

template <typename ServiceBuilderList> struct to_tuple;

template <typename... ServiceBuilders>
struct to_tuple<detail::type_list<ServiceBuilders...>> {
    using type =
        cib::tuple<index_metafunc_t<extract_service_tag>, ServiceBuilders...>;
    constexpr static inline type value{};
};

template <typename ServiceBuilderList>
constexpr static auto to_tuple_v = to_tuple<ServiceBuilderList>::value;

struct get_service {
    template <typename T>
    using invoke =
        typename std::remove_cv_t<std::remove_reference_t<T>>::service_type;
};

struct get_service_from_tuple {
    template <typename T>
    using invoke = typename std::remove_cv_t<std::remove_reference_t<
        decltype(std::declval<T>().get(index_<0>))>>::service_type;
};

template <typename Config>
constexpr static auto initialized_builders = transform<extract_service_tag>(
    [](auto extensions) {
        constexpr auto initial_builder = extensions.get(index_<0>).builder;
        using service = get_service_from_tuple::invoke<decltype(extensions)>;
        auto built_service = extensions.fold_right(
            initial_builder, [](auto extension, auto outer_builder) {
                return extension.args_tuple.fold_right(
                    outer_builder, [](auto arg, auto inner_builder) {
                        return inner_builder.add(arg);
                    });
            });

        return detail::service_entry<service, decltype(built_service)>{
            built_service};
    },
    demux<get_service>(Config::config.extends_tuple()));

template <typename Config, typename Tag> struct initialized {
    constexpr static auto value =
        initialized_builders<Config>.get(cib::tag_<Tag>).builder;
};
} // namespace cib

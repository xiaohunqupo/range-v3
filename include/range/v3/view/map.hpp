/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_MAP_HPP
#define RANGES_V3_VIEW_MAP_HPP

#include <utility>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/view/view.hpp>
#include <range/v3/view/transform.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename T>
            RANGES_CXX14_CONSTEXPR T&
            get_first_second_helper(T& t, std::true_type) noexcept
            {
                return t;
            }

            CPP_template(typename T)(
                requires MoveConstructible<T>)
            RANGES_CXX14_CONSTEXPR T
            get_first_second_helper(T& t, std::false_type)
                noexcept(std::is_nothrow_move_constructible<T>::value)
            {
                return std::move(t);
            }

            template<typename P, typename E>
            using get_first_second_tag = meta::bool_<
                std::is_lvalue_reference<P>::value ||
                std::is_lvalue_reference<E>::value>;

            struct get_first
            {
                template<typename Pair>
                RANGES_CXX14_CONSTEXPR auto operator()(Pair &&p) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    get_first_second_helper(p.first,
                        get_first_second_tag<Pair, decltype(p.first)>{})
                )
            };

            struct get_second
            {
                template<typename Pair>
                RANGES_CXX14_CONSTEXPR auto operator()(Pair &&p) const
                RANGES_DECLTYPE_AUTO_RETURN_NOEXCEPT
                (
                    get_first_second_helper(p.second,
                        get_first_second_tag<Pair, decltype(p.second)>{})
                )
            };

            CPP_def
            (
                template(typename T)
                concept PairLike,
                    Invocable<get_first const&, T> &&
                    Invocable<get_second const&, T>
            );
        }
        /// \endcond

        /// \addtogroup group-views
        /// @{
        namespace view
        {
            CPP_def
            (
                template(typename Rng)
                concept KeysViewConcept,
                    InputRange<Rng> &&
                    detail::PairLike<range_reference_t<Rng>>
            );

            struct keys_fn
            {
                CPP_template(typename Rng)(
                    requires KeysViewConcept<Rng>)
                keys_range_view<all_t<Rng>> operator()(Rng &&rng) const
                {
                    return {all(static_cast<Rng &&>(rng)), detail::get_first{}};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng)(
                    requires not KeysViewConcept<Rng>)
                void operator()(Rng &&) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The argument of view::keys must be a model of the InputRange concept.");
                    CPP_assert_msg(detail::PairLike<range_reference_t<Rng>>,
                        "The value type of the range passed to view::keys must look like a std::pair; "
                        "That is, it must have first and second data members.");
                }
            #endif
            };

            CPP_def
            (
                template(typename Rng)
                concept ValuesViewConcept,
                    InputRange<Rng> &&
                    detail::PairLike<range_reference_t<Rng>>
            );

            struct values_fn
            {
                CPP_template(typename Rng)(
                    requires ValuesViewConcept<Rng>)
                values_view<all_t<Rng>> operator()(Rng &&rng) const
                {
                    return {all(static_cast<Rng &&>(rng)), detail::get_second{}};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng)(
                    requires not ValuesViewConcept<Rng>)
                void operator()(Rng &&) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The argument of view::values must be a model of the InputRange concept.");
                    CPP_assert_msg(detail::PairLike<range_reference_t<Rng>>,
                        "The value type of the range passed to view::values must look like a std::pair; "
                        "That is, it must have first and second data members.");
                }
            #endif
            };

            /// \relates keys_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<keys_fn>, keys)

            /// \relates values_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<values_fn>, values)
        }
        /// @}
    }
}

#endif

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

#ifndef RANGES_V3_VIEW_TAKE_WHILE_HPP
#define RANGES_V3_VIEW_TAKE_WHILE_HPP

#include <utility>
#include <functional>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/detail/satisfy_boost_range.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/view_adaptor.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/iterator_concepts.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/view/view.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-views
        /// @{
        template<typename Rng, typename Pred>
        struct iter_take_while_view
          : view_adaptor<
                iter_take_while_view<Rng, Pred>,
                Rng,
                is_finite<Rng>::value ? finite : unknown>
        {
        private:
            friend range_access;
            semiregular_t<Pred> pred_;

            template<bool IsConst>
            struct sentinel_adaptor
              : adaptor_base
            {
            private:
                semiregular_ref_or_val_t<Pred, IsConst> pred_;
            public:
                sentinel_adaptor() = default;
                sentinel_adaptor(semiregular_ref_or_val_t<Pred, IsConst> pred)
                  : pred_(std::move(pred))
                {}
                bool empty(iterator_t<Rng> it, sentinel_t<Rng> end) const
                {
                    return it == end || !invoke(pred_, it);
                }
            };
            sentinel_adaptor<false> end_adaptor()
            {
                return {pred_};
            }
            CPP_member
            auto end_adaptor() const -> CPP_ret(sentinel_adaptor<true>)(
                requires Invocable<Pred const&, iterator_t<Rng>>)
            {
                return {pred_};
            }
        public:
            iter_take_while_view() = default;
            iter_take_while_view(Rng rng, Pred pred)
              : iter_take_while_view::view_adaptor{std::move(rng)}
              , pred_(std::move(pred))
            {}
        };

        template<typename Rng, typename Pred>
        struct take_while_view
          : iter_take_while_view<Rng, indirected<Pred>>
        {
            take_while_view() = default;
            take_while_view(Rng rng, Pred pred)
              : iter_take_while_view<Rng, indirected<Pred>>{std::move(rng),
                    indirect(std::move(pred))}
            {}
        };

        namespace view
        {
            CPP_def
            (
                template(typename Rng, typename Pred)
                concept IterPredicateRange,
                    InputRange<Rng> &&
                    Predicate<Pred&, iterator_t<Rng>> &&
                    CopyConstructible<Pred>
            );

            struct iter_take_while_fn
            {
            private:
                friend view_access;
                template<typename Pred>
                static auto bind(iter_take_while_fn iter_take_while, Pred pred)
                {
                    return make_pipeable(std::bind(iter_take_while, std::placeholders::_1,
                        protect(std::move(pred))));
                }
            public:
                CPP_template(typename Rng, typename Pred)(
                    requires IterPredicateRange<Rng, Pred>)
                iter_take_while_view<all_t<Rng>, Pred> operator()(Rng &&rng, Pred pred) const
                {
                    return {all(static_cast<Rng &&>(rng)), std::move(pred)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng, typename Pred)(
                    requires not IterPredicateRange<Rng, Pred>)
                void operator()(Rng &&, Pred) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The object on which view::take_while operates must be a model of the "
                        "InputRange concept.");
                    CPP_assert_msg(Predicate<Pred&, iterator_t<Rng>>,
                        "The function passed to view::take_while must be callable with objects of "
                        "the range's iterator type, and its result type must be convertible to "
                        "bool.");
                    CPP_assert_msg(CopyConstructible<Pred>,
                        "The function object passed to view::take_while must be CopyConstructible.");
                }
            #endif
            };

            CPP_def
            (
                template(typename Rng, typename Pred)
                concept IndirectPredicateRange,
                    InputRange<Rng> &&
                    IndirectPredicate<Pred&, iterator_t<Rng>>
            );

            struct take_while_fn
            {
            private:
                friend view_access;
                template<typename Pred>
                static auto bind(take_while_fn take_while, Pred pred)
                {
                    return make_pipeable(std::bind(take_while, std::placeholders::_1,
                        protect(std::move(pred))));
                }
            public:
                CPP_template(typename Rng, typename Pred)(
                    requires IndirectPredicateRange<Rng, Pred>)
                take_while_view<all_t<Rng>, Pred> operator()(Rng &&rng, Pred pred) const
                {
                    return {all(static_cast<Rng &&>(rng)), std::move(pred)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng, typename Pred)(
                    requires not IndirectPredicateRange<Rng, Pred>)
                void operator()(Rng &&, Pred) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The object on which view::take_while operates must be a model of the "
                        "InputRange concept.");
                    CPP_assert_msg(IndirectPredicate<Pred, iterator_t<Rng>>,
                        "The function passed to view::take_while must be callable with objects of "
                        "the range's common reference type, and its result type must be "
                        "convertible to bool.");
                }
            #endif
            };

            /// \relates iter_take_while_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<iter_take_while_fn>, iter_take_while)

            /// \relates take_while_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<take_while_fn>, take_while)
        }
        /// @}
    }
}

RANGES_SATISFY_BOOST_RANGE(::ranges::v3::iter_take_while_view)
RANGES_SATISFY_BOOST_RANGE(::ranges::v3::take_while_view)

#endif

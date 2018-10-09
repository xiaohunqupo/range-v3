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

#ifndef RANGES_V3_ACTION_TAKE_WHILE_HPP
#define RANGES_V3_ACTION_TAKE_WHILE_HPP

#include <functional>
#include <range/v3/range_fwd.hpp>
#include <range/v3/algorithm/find_if_not.hpp>
#include <range/v3/action/action.hpp>
#include <range/v3/action/erase.hpp>
#include <range/v3/utility/iterator.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/iterator_concepts.hpp>
#include <range/v3/utility/iterator_traits.hpp>
#include <range/v3/utility/static_const.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-actions
        /// @{
        namespace action
        {
            CPP_def
            (
                template(typename Rng, typename Fun)
                concept TakeWhileActionConcept,
                    ForwardRange<Rng> &&
                    ErasableRange<Rng &, iterator_t<Rng>, sentinel_t<Rng>> &&
                    IndirectPredicate<Fun, iterator_t<Rng>>
            );

            struct take_while_fn
            {
            private:
                friend action_access;
                template<typename Fun>
                static auto CPP_fun(bind)(take_while_fn take_while, Fun fun)(
                    requires not Range<Fun>)
                {
                    return std::bind(take_while, std::placeholders::_1, std::move(fun));
                }
            public:
                CPP_template(typename Rng, typename Fun)(
                    requires TakeWhileActionConcept<Rng, Fun>)
                Rng operator()(Rng &&rng, Fun fun) const
                {
                    ranges::action::erase(rng, find_if_not(begin(rng), end(rng), std::move(fun)),
                        end(rng));
                    return static_cast<Rng &&>(rng);
                }

            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng, typename Fun)(
                    requires not TakeWhileActionConcept<Rng, Fun>)
                void operator()(Rng &&, Fun &&) const
                {
                    CPP_assert_msg(ForwardRange<Rng>,
                        "The object on which action::take_while operates must be a model of the "
                        "ForwardRange concept.");
                    using I = iterator_t<Rng>;
                    using S = sentinel_t<Rng>;
                    CPP_assert_msg(ErasableRange<Rng &, I, S>,
                        "The object on which action::take_while operates must allow element "
                        "removal.");
                    CPP_assert_msg(IndirectPredicate<Fun, I>,
                        "The function passed to action::take_while must be callable with objects "
                        "of the range's common reference type, and it must return something convertible to "
                        "bool.");
                }
            #endif
            };

            /// \ingroup group-actions
            /// \relates take_while_fn
            /// \sa action
            RANGES_INLINE_VARIABLE(action<take_while_fn>, take_while)
        }
        /// @}
    }
}

#endif

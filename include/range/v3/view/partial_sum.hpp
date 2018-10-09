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

#ifndef RANGES_V3_VIEW_PARTIAL_SUM_HPP
#define RANGES_V3_VIEW_PARTIAL_SUM_HPP

#include <utility>
#include <iterator>
#include <functional>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/detail/satisfy_boost_range.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/size.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/view_adaptor.hpp>
#include <range/v3/utility/functional.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/view/view.hpp>
#include <range/v3/view/all.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-views
        /// @{
        template<typename Rng, typename Fun>
        struct partial_sum_view
          : view_adaptor<partial_sum_view<Rng, Fun>, Rng>
        {
        private:
            friend range_access;
            semiregular_t<Fun> fun_;
            using single_pass = meta::bool_<SinglePass<iterator_t<Rng>>>;
            using use_sentinel_t = meta::bool_<!BoundedRange<Rng> || single_pass{}>;

            template<bool IsConst>
            struct adaptor : adaptor_base
            {
            private:
                using partial_sum_view_t = meta::const_if_c<IsConst, partial_sum_view>;
                semiregular_t<range_value_type_t<Rng>> sum_;
                partial_sum_view_t *rng_;
            public:
                using single_pass = partial_sum_view::single_pass;
                adaptor() = default;
                adaptor(partial_sum_view_t &rng)
                  : rng_(&rng)
                {}
                iterator_t<Rng> begin(partial_sum_view_t &)
                {
                    auto &base = rng_->base();
                    auto it = ranges::begin(base);
                    if (it != ranges::end(base))
                        sum_ = *it;
                    return it;
                }
                range_value_type_t<Rng> read(iterator_t<Rng>) const
                {
                    return sum_;
                }
                void next(iterator_t<Rng> &it)
                {
                    if (++it != ranges::end(rng_->base()))
                    {
                        auto &current = static_cast<range_value_type_t<Rng> &>(sum_);
                        sum_ = invoke(rng_->fun_, current, *it);
                    }
                }
                void prev() = delete;
            };

            adaptor<false> begin_adaptor()
            {
                return {*this};
            }
            meta::if_<use_sentinel_t, adaptor_base, adaptor<false>> end_adaptor()
            {
                return {*this};
            }
            CPP_member
            auto begin_adaptor() const -> CPP_ret(adaptor<true>)(
                requires Range<Rng const> &&
                    Invocable<Fun const&, range_common_reference_t<Rng>,
                        range_common_reference_t<Rng>>)
            {
                return {*this};
            }
            CPP_member
            auto end_adaptor() const ->
                CPP_ret(meta::if_<use_sentinel_t, adaptor_base, adaptor<true>>)(
                    requires Range<Rng const> &&
                        Invocable<Fun const&, range_common_reference_t<Rng>,
                            range_common_reference_t<Rng>>)
            {
                return {*this};
            }
        public:
            partial_sum_view() = default;
            partial_sum_view(Rng rng, Fun fun)
              : partial_sum_view::view_adaptor{std::move(rng)}
              , fun_(std::move(fun))
            {}
            CPP_member
            auto size() const -> CPP_ret(range_size_type_t<Rng>)(
                requires SizedRange<Rng>)
            {
                return ranges::size(this->base());
            }
        };

        namespace view
        {
            CPP_def
            (
                template(typename Rng, typename Fun)
                concept PartialSumViewConcept,
                    InputRange<Rng> &&
                    IndirectInvocable<Fun, iterator_t<Rng>, iterator_t<Rng>> &&
                    ConvertibleTo<
                        invoke_result_t<Fun &, range_common_reference_t<Rng>,
                            range_common_reference_t<Rng>>,
                        range_value_type_t<Rng>>
            );

            struct partial_sum_fn
            {
            private:
                friend view_access;
                template<typename Fun = plus>
                static auto bind(partial_sum_fn partial_sum, Fun fun = {})
                {
                    return make_pipeable(std::bind(partial_sum, std::placeholders::_1,
                        protect(std::move(fun))));
                }
            public:
                CPP_template(typename Rng, typename Fun)(
                    requires PartialSumViewConcept<Rng, Fun>)
                partial_sum_view<all_t<Rng>, Fun> operator()(Rng &&rng, Fun fun) const
                {
                    return {all(static_cast<Rng &&>(rng)), std::move(fun)};
                }
            #ifndef RANGES_DOXYGEN_INVOKED
                CPP_template(typename Rng, typename Fun)(
                    requires not PartialSumViewConcept<Rng, Fun>)
                void operator()(Rng &&, Fun) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The first argument passed to view::partial_sum must be a model of the "
                        "InputRange concept.");
                    CPP_assert_msg(IndirectInvocable<Fun, iterator_t<Rng>,
                        iterator_t<Rng>>,
                        "The second argument passed to view::partial_sum must be callable with "
                        "two values from the range passed as the first argument.");
                    CPP_assert_msg(ConvertibleTo<
                        invoke_result_t<Fun &, range_common_reference_t<Rng>,
                            range_common_reference_t<Rng>>,
                        range_value_type_t<Rng>>,
                        "The return type of the function passed to view::partial_sum must be "
                        "convertible to the value type of the range.");
                }
            #endif
            };

            /// \relates partial_sum_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<partial_sum_fn>, partial_sum)
        }
        /// @}
    }
}

RANGES_SATISFY_BOOST_RANGE(::ranges::v3::partial_sum_view)

#endif

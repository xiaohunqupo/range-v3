/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_UTILITY_COUNTED_ITERATOR_HPP
#define RANGES_V3_UTILITY_COUNTED_ITERATOR_HPP

#include <utility>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/utility/iterator.hpp>
#include <range/v3/utility/iterator_traits.hpp>
#include <range/v3/utility/iterator_concepts.hpp>
#include <range/v3/utility/basic_iterator.hpp>

namespace ranges
{
    inline namespace v3
    {
        template<typename I, typename = meta::if_c<Iterator<I>>>
        struct counted_iterator;

        /// \cond
        namespace _counted_iterator_
        {
            struct access
            {
                template<typename I>
                static RANGES_CXX14_CONSTEXPR difference_type_t<counted_iterator<I>> &
                count(counted_iterator<I> &ci) noexcept
                {
                    return ci.cnt_;
                }

                template<typename I>
                static RANGES_CXX14_CONSTEXPR I &
                current(counted_iterator<I> &ci) noexcept
                {
                    return ci.current_;
                }

                template<typename I>
                static constexpr I const &
                current(counted_iterator<I> const &ci) noexcept
                {
                    return ci.current_;
                }
            };

#if RANGES_BROKEN_CPO_LOOKUP
            template<typename> struct adl_hook {};
#endif
        }
        /// \endcond

        template<typename I, typename /*= meta::if_<Iterator<I>>*/>
        struct counted_iterator
#if RANGES_BROKEN_CPO_LOOKUP
          : private _counted_iterator_::adl_hook<counted_iterator<I>>
#endif
        {
        private:
            CPP_assert(Iterator<I>);
            friend _counted_iterator_::access;

            I current_{};
            difference_type_t<I> cnt_{0};

            void post_increment_(std::true_type)
            {
                CPP_assert(std::is_void<decltype(current_++)>());
                ++current_;
            }
            auto post_increment_(std::false_type) -> decltype(current_++)
            {
                CPP_assert(!std::is_void<decltype(current_++)>());
                auto&& tmp = current_++;
                --cnt_;
                return static_cast<decltype(tmp) &&>(tmp);
            }
        public:
            using iterator_type = I;
            using difference_type = difference_type_t<I>;

            counted_iterator() = default;

            counted_iterator(I x, difference_type_t<I> n)
              : current_(std::move(x)), cnt_(n)
            {
                RANGES_EXPECT(n >= 0);
            }

            CPP_template(typename I2)(
                requires ConvertibleTo<I2, I>)
            counted_iterator(const counted_iterator<I2>& i)
              : current_(_counted_iterator_::access::current(i)), cnt_(i.count())
            {}

            CPP_template(typename I2)(
                requires ConvertibleTo<I2, I>)
            counted_iterator& operator=(const counted_iterator<I2>& i)
            {
                current_ = _counted_iterator_::access::current(i);
                cnt_ = i.count();
            }

            I base() const
            {
                return current_;
            }

            difference_type_t<I> count() const
            {
                return cnt_;
            }

            reference_t<I> operator*()
                noexcept(noexcept(reference_t<I>(*current_)))
            {
                RANGES_EXPECT(cnt_ > 0);
                return *current_;
            }
            template<typename I2 = I>
            auto operator*() const
                noexcept(noexcept(reference_t<I>(*current_))) ->
                CPP_ret(reference_t<I2>)(
                    requires Readable<I const>)
            {
                RANGES_EXPECT(cnt_ > 0);
                return *current_;
            }

            counted_iterator& operator++()
            {
                RANGES_EXPECT(cnt_ > 0);
                ++current_;
                --cnt_;
                return *this;
            }

            CPP_member
            auto operator++(int) ->
                CPP_ret(decltype(std::declval<I&>()++))(
                    requires not ForwardIterator<I>)
            {
                RANGES_EXPECT(cnt_ > 0);
                return post_increment_(std::is_void<decltype(current_++)>());
            }

            CPP_member
            auto operator++(int) ->
                CPP_ret(counted_iterator)(
                    requires ForwardIterator<I>)
            {
                auto tmp(*this);
                ++*this;
                return tmp;
            }

            CPP_member
            auto operator--() ->
                CPP_ret(counted_iterator &)(
                    requires BidirectionalIterator<I>)
            {
                --current_;
                ++cnt_;
                return *this;
            }

            CPP_member
            auto operator--(int) ->
                CPP_ret(counted_iterator)(
                    requires BidirectionalIterator<I>)
            {
                auto tmp(*this);
                --*this;
                return tmp;
            }

            CPP_member
            auto operator+=(difference_type n) ->
                CPP_ret(counted_iterator &)(
                    requires RandomAccessIterator<I>)
            {
                RANGES_EXPECT(cnt_ >= n);
                current_ += n;
                cnt_ -= n;
                return *this;
            }

            CPP_member
            auto operator+(difference_type n) const ->
                CPP_ret(counted_iterator)(
                    requires RandomAccessIterator<I>)
            {
                auto tmp(*this);
                tmp += n;
                return tmp;
            }

            CPP_member
            auto operator-=(difference_type n) ->
                CPP_ret(counted_iterator &)(
                    requires RandomAccessIterator<I>)
            {
                RANGES_EXPECT(cnt_ >= -n);
                current_ -= n;
                cnt_ += n;
                return *this;
            }

            CPP_member
            auto operator-(difference_type n) const ->
                CPP_ret(counted_iterator)(
                    requires RandomAccessIterator<I>)
            {
                auto tmp(*this);
                tmp -= n;
                return tmp;
            }

            CPP_member
            auto operator[](difference_type n) const ->
                CPP_ret(reference_t<I>)(
                    requires RandomAccessIterator<I>)
            {
                RANGES_EXPECT(cnt_ >= n);
                return current_[n];
            }

#if !RANGES_BROKEN_CPO_LOOKUP
            CPP_broken_friend_member
            friend RANGES_CXX14_CONSTEXPR
            auto iter_move(counted_iterator const &i)
                noexcept(detail::has_nothrow_iter_move<I>::value) ->
                CPP_broken_friend_ret(rvalue_reference_t<I>)(
                    requires InputIterator<I>)
            {
                return ranges::iter_move(i.current_);
            }
            template<typename I2, typename S2>
            friend auto iter_swap(counted_iterator const &x,
                                  counted_iterator<I2> const &y)
                noexcept(is_nothrow_indirectly_swappable<I, I2>::value) ->
                CPP_broken_friend_ret(void)(
                    requires IndirectlySwappable<I2, I>)
            {
                return ranges::iter_swap(
                    x.current_,
                    _counted_iterator_::access::current(y));
            }
            friend void advance(counted_iterator &i, difference_type_t<I> n)
            {
                RANGES_EXPECT(i.cnt_ >= n);
                ranges::advance(i.current_, n);
                i.cnt_ -= n;
            }
#endif
        };

#if RANGES_BROKEN_CPO_LOOKUP
        namespace _counted_iterator_
        {
            template<typename I>
            RANGES_CXX14_CONSTEXPR
            auto iter_move(counted_iterator<I> const &i)
                noexcept(detail::has_nothrow_iter_move<I>::value) ->
                CPP_broken_friend_ret(rvalue_reference_t<I>)(
                    requires InputIterator<I>)
            {
                return ranges::iter_move(_counted_iterator_::access::current(i));
            }
            template<typename I1, typename I2>
            auto iter_swap(counted_iterator<I1> const &x,
                           counted_iterator<I2> const &y)
                noexcept(is_nothrow_indirectly_swappable<I1, I2>::value) ->
                CPP_broken_friend_ret(void)(
                    requires IndirectlySwappable<I2, I1>)
            {
                return ranges::iter_swap(
                    _counted_iterator_::access::current(x),
                    _counted_iterator_::access::current(y));
            }
            template<typename I>
            void advance(counted_iterator<I> &i, difference_type_t<I> n)
            {
                auto &count = _counted_iterator_::access::count(i);
                RANGES_EXPECT(count >= n);
                ranges::advance(_counted_iterator_::access::current(i), n);
                count -= n;
            }
        }
#endif

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator==(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return x.count() == y.count();
        }

        template<typename I>
        bool operator==(const counted_iterator<I>& x, default_sentinel)
        {
            return x.count() == 0;
        }

        template<typename I>
        bool operator==(default_sentinel, const counted_iterator<I>& x)
        {
            return x.count() == 0;
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator!=(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return !(x == y);
        }

        template<typename I>
        bool operator!=(const counted_iterator<I>& x, default_sentinel y)
        {
            return !(x == y);
        }

        template<typename I>
        bool operator!=(default_sentinel x, const counted_iterator<I>& y)
        {
            return !(x == y);
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator<(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return y.count() < x.count();
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator<=(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return !(y < x);
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator>(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return y < x;
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        bool operator>=(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return !(x < y);
        }

        CPP_template(typename I1, typename I2)(
            requires Common<I1, I2>)
        difference_type_t<I2>
        operator-(const counted_iterator<I1>& x, const counted_iterator<I2>& y)
        {
            return y.count() - x.count();
        }

        template<typename I>
        difference_type_t<I>
        operator-(const counted_iterator<I>& x, default_sentinel)
        {
            return -x.count();
        }

        template<typename I>
        difference_type_t<I>
        operator-(default_sentinel, const counted_iterator<I>& y)
        {
            return y.count();
        }

        CPP_template(typename I)(
            requires RandomAccessIterator<I>)
        counted_iterator<I>
        operator+(difference_type_t<I> n, const counted_iterator<I>& x)
        {
            return x + n;
        }

        namespace _counted_iterator_
        {
            template<typename I, typename = void>
            struct value_type_
            {};

            template<typename I>
            struct value_type_<I, meta::if_c<Readable<I>>>
            {
                using type = value_type_t<I>;
            };

            template<typename I, typename = void>
            struct iterator_category_
            {};

            template<typename I>
            struct iterator_category_<I, meta::if_c<InputIterator<I>>>
            {
                using type = iterator_category_t<I>;
            };

            template<typename I, typename = void>
            struct iterator_traits_
            {
                using iterator_category = std::output_iterator_tag;
                using difference_type = difference_type_t<I>;
                using value_type = void;
                using reference = void;
                using pointer = void;
            };

            template<typename I>
            struct iterator_traits_<I, meta::if_c<InputIterator<I>>>
            {
                using iterator_category =
                    meta::if_c<
                        (bool)ForwardIterator<I> &&
                            std::is_reference<reference_t<I>>::value,
                        std::forward_iterator_tag,
                        std::input_iterator_tag>;
                using difference_type = difference_type_t<I>;
                using value_type = value_type_t<I>;
                using reference = reference_t<I>;
                using pointer = meta::_t<detail::pointer_type_<I>>;
            };
        } // namespace _counted_iterator_

        CPP_template(typename I)(
            requires Iterator<I>)
        counted_iterator<I> make_counted_iterator(I i, difference_type_t<I> n)
        {
            return {std::move(i), n};
        }

        template<typename I>
        struct value_type<counted_iterator<I>>
          : _counted_iterator_::value_type_<I>
        {};

        template<typename I>
        struct iterator_category<counted_iterator<I>>
          : _counted_iterator_::iterator_category_<I>
        {};
    }
}

/// \cond
namespace std
{
    template<typename I>
    struct iterator_traits< ::ranges::counted_iterator<I>>
      : ::ranges::_counted_iterator_::iterator_traits_<I>
    {};
}
/// \endcond

#endif

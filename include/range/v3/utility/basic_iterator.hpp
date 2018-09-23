/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef RANGES_V3_UTILITY_BASIC_ITERATOR_HPP
#define RANGES_V3_UTILITY_BASIC_ITERATOR_HPP

#include <utility>
#include <type_traits>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_access.hpp>
#include <range/v3/utility/box.hpp>
#include <range/v3/utility/move.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/nullptr_v.hpp>
#include <range/v3/utility/semiregular.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/utility/iterator_traits.hpp>
#include <range/v3/utility/iterator_concepts.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename Cur>
            using cursor_reference_t =
                decltype(range_access::read(std::declval<Cur const &>()));

            // Compute the rvalue reference type of a cursor
            template<typename Cur>
            auto cursor_move(Cur const &cur, int) ->
                decltype(range_access::move(cur));
            template<typename Cur>
            auto cursor_move(Cur const &cur, long) ->
                aux::move_t<cursor_reference_t<Cur>>;

            template<typename Cur>
            using cursor_rvalue_reference_t =
                decltype(detail::cursor_move(std::declval<Cur const &>(), 42));

            // Define conversion operators from the proxy reference type
            // to the common reference types, so that basic_iterator can model Readable
            // even with getters/setters.
            template<typename Derived, typename Head>
            struct proxy_reference_conversion
            {
                operator Head() const
                    noexcept(noexcept(Head(Head(std::declval<Derived const &>().read_()))))
                {
                    return Head(static_cast<Derived const *>(this)->read_());
                }
            };

            // Collect the reference types associated with cursors
            template<typename Cur, bool Readable>
            struct cursor_traits_
            {
            private:
                struct private_ {};
            public:
                using value_t_ = private_;
                using reference_t_ = private_;
                using rvalue_reference_t_ = private_;
                using common_refs = meta::list<>;
            };

            template<typename Cur>
            struct cursor_traits_<Cur, true>
            {
                using value_t_ = range_access::cursor_value_t<Cur>;
                using reference_t_ = cursor_reference_t<Cur>;
                using rvalue_reference_t_ = cursor_rvalue_reference_t<Cur>;
            private:
                using R1 = reference_t_;
                using R2 = common_reference_t<reference_t_, value_t_ &>;
                using R3 = common_reference_t<reference_t_, rvalue_reference_t_>;
                using tmp1 = meta::list<value_t_, R1>;
                using tmp2 =
                    meta::if_<meta::in<tmp1, uncvref_t<R2>>, tmp1, meta::push_back<tmp1, R2>>;
                using tmp3 =
                    meta::if_<meta::in<tmp2, uncvref_t<R3>>, tmp2, meta::push_back<tmp2, R3>>;
            public:
                using common_refs = meta::unique<meta::pop_front<tmp3>>;
            };

            template<typename Cur>
            using cursor_traits = cursor_traits_<Cur, (bool) ReadableCursor<Cur>>;

            template<typename Cur>
            using cursor_value_t = typename cursor_traits<Cur>::value_t_;

            template<typename Cur, bool Readable>
            struct basic_proxy_reference_;
            template<typename Cur>
            using basic_proxy_reference = basic_proxy_reference_<Cur, (bool) ReadableCursor<Cur>>;

            // The One Proxy Reference type to rule them all. basic_iterator uses this
            // as the return type of operator* when the cursor type has a set() member
            // function of the correct signature (i.e., if it can accept a value_type &&).
            template<typename Cur, bool Readable /*= (bool) ReadableCursor<Cur>*/>
            struct basic_proxy_reference_
              : cursor_traits<Cur>
                // The following adds conversion operators to the common reference
                // types, so that basic_proxy_reference can model Readable
              , meta::inherit<
                    meta::transform<
                        typename cursor_traits<Cur>::common_refs,
                        meta::bind_front<
                            meta::quote<proxy_reference_conversion>,
                            basic_proxy_reference_<Cur, Readable>>>>
            {
            private:
                Cur *cur_;
                template<typename, bool>
                friend struct basic_proxy_reference_;
                template<typename, typename>
                friend struct proxy_reference_conversion;
                using typename cursor_traits<Cur>::value_t_;
                using typename cursor_traits<Cur>::reference_t_;
                using typename cursor_traits<Cur>::rvalue_reference_t_;
                static_assert((bool) CommonReference<value_t_ &, reference_t_>,
                    "Your readable and writable cursor must have a value type and a reference "
                    "type that share a common reference type. See the ranges::common_reference "
                    "type trait.");
            public:
                // BUGBUG make these private:
                RANGES_CXX14_CONSTEXPR
                reference_t_ read_() const
                    noexcept(noexcept(reference_t_(range_access::read(std::declval<Cur const &>()))))
                {
                    return range_access::read(*cur_);
                }
                template<typename T>
                RANGES_CXX14_CONSTEXPR
                void write_(T &&t) const
                {
                    range_access::write(*cur_, (T &&) t);
                }
                basic_proxy_reference_() = default;
                basic_proxy_reference_(basic_proxy_reference_ const &) = default;
                CPP_template(typename OtherCur)(
                    requires ConvertibleTo<OtherCur *, Cur *>)
                RANGES_CXX14_CONSTEXPR
                basic_proxy_reference_(basic_proxy_reference<OtherCur> const &that) noexcept
                  : cur_(that.cur_)
                {}
                RANGES_CXX14_CONSTEXPR
                explicit basic_proxy_reference_(Cur &cur) noexcept
                  : cur_(&cur)
                {}
                CPP_member
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference_ && that) ->
                    CPP_ret(basic_proxy_reference_ &)(
                        requires ReadableCursor<Cur>)
                {
                    return *this = that;
                }
                CPP_member
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference_ const &that) ->
                    CPP_ret(basic_proxy_reference_ &)(
                        requires ReadableCursor<Cur>)
                {
                    this->write_(that.read_());
                    return *this;
                }
                CPP_member
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference_ && that) const ->
                    CPP_ret(basic_proxy_reference_ const &)(
                        requires ReadableCursor<Cur>)
                {
                    return *this = that;
                }
                CPP_member
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference_ const &that) const ->
                    CPP_ret(basic_proxy_reference_ const &)(
                        requires ReadableCursor<Cur>)
                {
                    this->write_(that.read_());
                    return *this;
                }
                template<typename OtherCur>
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference<OtherCur> && that) ->
                    CPP_ret(basic_proxy_reference_ &)(
                        requires ReadableCursor<OtherCur> &&
                            WritableCursor<Cur, cursor_reference_t<OtherCur>>)
                {
                    return *this = that;
                }
                template<typename OtherCur>
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference<OtherCur> const &that) ->
                    CPP_ret(basic_proxy_reference_ &)(
                        requires ReadableCursor<OtherCur> &&
                            WritableCursor<Cur, cursor_reference_t<OtherCur>>)
                {
                    this->write_(that.read_());
                    return *this;
                }
                template<typename OtherCur>
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference<OtherCur> && that) const ->
                    CPP_ret(basic_proxy_reference_ const &)(
                        requires ReadableCursor<OtherCur> &&
                            WritableCursor<Cur, cursor_reference_t<OtherCur>>)
                {
                    return *this = that;
                }
                template<typename OtherCur>
                RANGES_CXX14_CONSTEXPR
                auto operator=(basic_proxy_reference<OtherCur> const &that) const ->
                    CPP_ret(basic_proxy_reference_ const &)(
                        requires ReadableCursor<OtherCur> &&
                            WritableCursor<Cur, cursor_reference_t<OtherCur>>)
                {
                    this->write_(that.read_());
                    return *this;
                }
                template<typename T>
                RANGES_CXX14_CONSTEXPR
                auto operator=(T &&t) ->
                    CPP_ret(basic_proxy_reference_ &)(
                        requires WritableCursor<Cur, T>)
                {
                    this->write_((T &&) t);
                    return *this;
                }
                template<typename T>
                RANGES_CXX14_CONSTEXPR
                auto operator=(T &&t) const ->
                    CPP_ret(basic_proxy_reference_ const &)(
                        requires WritableCursor<Cur, T>)
                {
                    this->write_((T &&) t);
                    return *this;
                }
            };

            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator==(basic_proxy_reference_<Cur, Readable> const &x,
                cursor_value_t<Cur> const &y) -> CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return x.read_() == y;
            }
            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator!=(basic_proxy_reference_<Cur, Readable> const &x,
                cursor_value_t<Cur> const &y) -> CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return !(x == y);
            }
            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator==(cursor_value_t<Cur> const &x,
                basic_proxy_reference_<Cur, Readable> const &y) ->
                CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return x == y.read_();
            }
            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator!=(cursor_value_t<Cur> const &x,
                basic_proxy_reference_<Cur, Readable> const &y) ->
                CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return !(x == y);
            }
            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator==(basic_proxy_reference_<Cur, Readable> const &x,
                basic_proxy_reference_<Cur, Readable> const &y) ->
                CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return x.read_() == y.read_();
            }
            template<typename Cur, bool Readable>
            RANGES_CXX14_CONSTEXPR
            auto operator!=(basic_proxy_reference_<Cur, Readable> const &x,
                basic_proxy_reference_<Cur, Readable> const &y) ->
                CPP_ret(bool)(
                    requires ReadableCursor<Cur> &&
                        EqualityComparable<cursor_value_t<Cur>>)
            {
                return !(x == y);
            }

            auto iter_cat(input_cursor_tag) ->
                ranges::input_iterator_tag;
            auto iter_cat(forward_cursor_tag) ->
                ranges::forward_iterator_tag;
            auto iter_cat(bidirectional_cursor_tag) ->
                ranges::bidirectional_iterator_tag;
            auto iter_cat(random_access_cursor_tag) ->
                ranges::random_access_iterator_tag;

            template<typename Cur, bool Readable /*= (bool) ReadableCursor<Cur>*/>
            struct iterator_associated_types_base_
            {
            //protected:
                using reference_t = basic_proxy_reference<Cur>;
                using const_reference_t = basic_proxy_reference<Cur const>;
                using cursor_tag_t = concepts::tag<detail::OutputCursorConcept, cursor_tag>;
            public:
                using reference = void;
                using difference_type = range_access::cursor_difference_t<Cur>;
            };

            template<typename Cur>
            using cursor_arrow_t =
                decltype(range_access::arrow(std::declval<Cur const &>()));

            template<typename Cur>
            struct iterator_associated_types_base_<Cur, true>
            {
            //protected:
                using cursor_tag_t = detail::cursor_tag_of<Cur>;
                using reference_t =
                    meta::if_<
                        is_writable_cursor<Cur const>,
                        basic_proxy_reference<Cur const>,
                        meta::if_<
                            is_writable_cursor<Cur>,
                            basic_proxy_reference<Cur>,
                            cursor_reference_t<Cur>>>;
                using const_reference_t =
                    meta::if_<
                        is_writable_cursor<Cur const>,
                        basic_proxy_reference<Cur const>,
                        cursor_reference_t<Cur>>;
            public:
                using difference_type = range_access::cursor_difference_t<Cur>;
                using value_type = range_access::cursor_value_t<Cur>;
                using reference = reference_t;
                using iterator_category =
                    decltype(detail::iter_cat(cursor_tag_t()));
                using pointer = meta::_t<meta::if_c<
                    HasCursorArrow<Cur>,
                    meta::defer<cursor_arrow_t, Cur>,
                    std::add_pointer<reference>>>;
                using common_reference = common_reference_t<reference, value_type &>;
            };

            template<typename Cur>
            using iterator_associated_types_base =
                iterator_associated_types_base_<Cur, (bool) ReadableCursor<Cur>>;
        }
        /// \endcond

        /// \addtogroup group-utility Utility
        /// @{
        ///
        template<typename T>
        struct basic_mixin : private box<T>
        {
            CPP_member
            constexpr CPP_ctor(basic_mixin)()(
                noexcept(std::is_nothrow_default_constructible<T>::value)
                requires DefaultConstructible<T>)
              : box<T>{}
            {}
            CPP_member
            explicit constexpr CPP_ctor(basic_mixin)(T &&t)(
                noexcept(std::is_nothrow_move_constructible<T>::value)
                requires MoveConstructible<T>)
              : box<T>(detail::move(t))
            {}
            CPP_member
            explicit constexpr CPP_ctor(basic_mixin)(T const &t)(
                noexcept(std::is_nothrow_copy_constructible<T>::value)
                requires CopyConstructible<T>)
              : box<T>(t)
            {}
        protected:
            using box<T>::get;
        };

#if RANGES_BROKEN_CPO_LOOKUP
        namespace _basic_iterator_ { template<typename> struct adl_hook {}; }
#endif

        template<typename Cur>
        struct basic_iterator
          : range_access::mixin_base_t<Cur>
          , detail::iterator_associated_types_base<Cur>
#if RANGES_BROKEN_CPO_LOOKUP
          , private _basic_iterator_::adl_hook<basic_iterator<Cur>>
#endif
        {
        private:
            template<typename>
            friend struct basic_iterator;
            friend range_access;
            using mixin_t = range_access::mixin_base_t<Cur>;
            static_assert((bool) detail::Cursor<Cur>, "");
            using assoc_types_ = detail::iterator_associated_types_base<Cur>;
            using typename assoc_types_::cursor_tag_t;
            using typename assoc_types_::reference_t;
            using typename assoc_types_::const_reference_t;
            RANGES_CXX14_CONSTEXPR Cur &pos() noexcept
            {
                return this->mixin_t::get();
            }
            constexpr Cur const &pos() const noexcept
            {
                return this->mixin_t::get();
            }

        public:
            using typename assoc_types_::difference_type;
            constexpr basic_iterator() = default;
            CPP_template(typename OtherCur)(
                requires ConvertibleTo<OtherCur, Cur> &&
                    Constructible<mixin_t, OtherCur>)
            RANGES_CXX14_CONSTEXPR
            basic_iterator(basic_iterator<OtherCur> that)
              : mixin_t{std::move(that.pos())}
            {}
            // Mix in any additional constructors provided by the mixin
            using mixin_t::mixin_t;

            template<typename T>
            RANGES_CXX14_CONSTEXPR
            auto operator=(T &&t)
            noexcept(noexcept(std::declval<Cur &>().write(static_cast<T &&>(t)))) ->
                CPP_ret(basic_iterator &)(
                    requires not defer::Same<uncvref_t<T>, basic_iterator> &&
                        !detail::defer::HasCursorNext<Cur> &&
                        detail::defer::WritableCursor<Cur, T>)
            {
                pos().write(static_cast<T &&>(t));
                return *this;
            }

            template<typename T>
            RANGES_CXX14_CONSTEXPR
            auto operator=(T &&t) const
            noexcept(noexcept(std::declval<Cur const &>().write(static_cast<T &&>(t)))) ->
                CPP_ret(basic_iterator const &)(
                    requires not defer::Same<uncvref_t<T>, basic_iterator> &&
                        !detail::defer::HasCursorNext<Cur> &&
                        detail::defer::WritableCursor<Cur const, T>)
            {
                pos().write(static_cast<T &&>(t));
                return *this;
            }

            CPP_member
            constexpr auto operator*() const
            noexcept(noexcept(range_access::read(std::declval<Cur const &>()))) ->
                CPP_ret(const_reference_t)(
                    requires detail::ReadableCursor<Cur> &&
                        !detail::is_writable_cursor<Cur>::value)
            {
                return range_access::read(pos());
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator*()
            noexcept(noexcept(reference_t{std::declval<Cur &>()})) ->
                CPP_ret(reference_t)(
                    requires detail::HasCursorNext<Cur> &&
                        detail::is_writable_cursor<Cur>::value)
            {
                return reference_t{pos()};
            }
            CPP_member
            constexpr auto operator*() const
            noexcept(noexcept(const_reference_t{std::declval<Cur const &>()})) ->
                CPP_ret(const_reference_t)(
                    requires detail::HasCursorNext<Cur> &&
                        detail::is_writable_cursor<Cur const>::value)
            {
                return const_reference_t{pos()};
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator*() noexcept ->
                CPP_ret(basic_iterator &)(
                    requires not detail::HasCursorNext<Cur>)
            {
                return *this;
            }

            // Use cursor's arrow() member, if any.
            template<typename C = Cur>
            constexpr auto operator->() const
            noexcept(noexcept(range_access::arrow(std::declval<C const &>()))) ->
                CPP_ret(detail::cursor_arrow_t<C>)(
                    requires detail::HasCursorArrow<C>)
            {
                return range_access::arrow(pos());
            }
            // Otherwise, if reference_t is an lvalue reference to cv-qualified
            // value_type_t, return the address of **this.
            template<typename C = Cur>
            constexpr auto operator->() const
            noexcept(noexcept(*std::declval<basic_iterator const &>())) ->
                CPP_ret(meta::_t<std::add_pointer<const_reference_t>>)(
                    requires not detail::HasCursorArrow<Cur> &&
                        detail::ReadableCursor<Cur> &&
                        std::is_lvalue_reference<const_reference_t>::value &&
                        Same<typename detail::iterator_associated_types_base<C>::value_type,
                            uncvref_t<const_reference_t>>)
            {
                return std::addressof(**this);
            }

            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator++() ->
                CPP_ret(basic_iterator &)(
                    requires detail::HasCursorNext<Cur>)
            {
                range_access::next(pos());
                return *this;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator++() noexcept ->
                CPP_ret(basic_iterator &)(
                    requires not detail::HasCursorNext<Cur>)
            {
                return *this;
            }

            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator++(int) ->
                CPP_ret(basic_iterator)(
                    requires not Same<detail::input_cursor_tag, detail::cursor_tag_of<Cur>>)
            {
                basic_iterator tmp{*this};
                ++*this;
                return tmp;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator++(int) ->
                CPP_ret(void)(
                    requires Same<detail::input_cursor_tag, detail::cursor_tag_of<Cur>>)
            {
                ++*this;
            }

            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator--() ->
                CPP_ret(basic_iterator &)(
                    requires detail::BidirectionalCursor<Cur>)
            {
                range_access::prev(pos());
                return *this;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator--(int) ->
                CPP_ret(basic_iterator)(
                    requires detail::BidirectionalCursor<Cur>)
            {
                basic_iterator tmp(*this);
                --*this;
                return tmp;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator+=(difference_type n) ->
                CPP_ret(basic_iterator &)(
                    requires detail::RandomAccessCursor<Cur>)
            {
                range_access::advance(pos(), n);
                return *this;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator-=(difference_type n) ->
                CPP_ret(basic_iterator &)(
                    requires detail::RandomAccessCursor<Cur>)
            {
                range_access::advance(pos(), -n);
                return *this;
            }
            CPP_member
            RANGES_CXX14_CONSTEXPR auto operator[](difference_type n) const ->
                CPP_ret(const_reference_t)(
                    requires detail::RandomAccessCursor<Cur>)
            {
                return *(*this + n);
            }

#if !RANGES_BROKEN_CPO_LOOKUP
            // Optionally support hooking iter_move when the cursor sports a
            // move() member function.
            template<typename C = Cur>
            RANGES_CXX14_CONSTEXPR
            friend auto iter_move(basic_iterator const &it)
            noexcept(noexcept(range_access::move(std::declval<C const &>()))) ->
                CPP_broken_friend_ret(decltype(range_access::move(std::declval<C const &>())))(
                    requires Same<C, Cur> && detail::InputCursor<Cur>)
            {
                return range_access::move(it.pos());
            }
#endif
        };

        template<typename Cur, typename Cur2>
        constexpr auto operator==(basic_iterator<Cur> const &left,
            basic_iterator<Cur2> const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<Cur2, Cur>)
        {
            return range_access::equal(range_access::pos(left), range_access::pos(right));
        }
        template<typename Cur, typename Cur2>
        constexpr auto operator!=(basic_iterator<Cur> const &left,
            basic_iterator<Cur2> const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<Cur2, Cur>)
        {
            return !(left == right);
        }
        template<typename Cur, typename S>
        constexpr auto operator==(basic_iterator<Cur> const &left,
            S const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<S, Cur>)
        {
            return range_access::equal(range_access::pos(left), right);
        }
        template<typename Cur, typename S>
        constexpr auto operator!=(basic_iterator<Cur> const &left,
            S const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<S, Cur>)
        {
            return !(left == right);
        }
        template<typename S, typename Cur>
        constexpr auto operator==(S const &left,
            basic_iterator<Cur> const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<S, Cur>)
        {
            return right == left;
        }
        template<typename S, typename Cur>
        constexpr auto operator!=(S const &left,
            basic_iterator<Cur> const &right) ->
                CPP_ret(bool)(
                    requires detail::CursorSentinel<S, Cur>)
        {
            return right != left;
        }

        template<typename Cur>
        RANGES_CXX14_CONSTEXPR
        auto operator+(basic_iterator<Cur> left,
            typename basic_iterator<Cur>::difference_type n) ->
            CPP_ret(basic_iterator<Cur>)(
                requires detail::RandomAccessCursor<Cur>)
        {
            left += n;
            return left;
        }
        template<typename Cur>
        RANGES_CXX14_CONSTEXPR
        auto operator+(
            typename basic_iterator<Cur>::difference_type n,
            basic_iterator<Cur> right) ->
            CPP_ret(basic_iterator<Cur>)(
                requires detail::RandomAccessCursor<Cur>)
        {
            right += n;
            return right;
        }
        template<typename Cur>
        RANGES_CXX14_CONSTEXPR
        auto operator-(basic_iterator<Cur> left,
            typename basic_iterator<Cur>::difference_type n) ->
            CPP_ret(basic_iterator<Cur>)(
                requires detail::RandomAccessCursor<Cur>)
        {
            left -= n;
            return left;
        }
        template<typename Cur2, typename Cur>
        RANGES_CXX14_CONSTEXPR
        auto operator-(basic_iterator<Cur2> const &left,
            basic_iterator<Cur> const &right) ->
            CPP_ret(typename basic_iterator<Cur>::difference_type)(
                requires detail::SizedCursorSentinel<Cur2, Cur>)
        {
            return range_access::distance_to(range_access::pos(right), range_access::pos(left));
        }
        template<typename S, typename Cur>
        RANGES_CXX14_CONSTEXPR
        auto operator-(S const &left, basic_iterator<Cur> const &right) ->
            CPP_ret(typename basic_iterator<Cur>::difference_type)(
                requires detail::SizedCursorSentinel<S, Cur>)
        {
            return range_access::distance_to(range_access::pos(right), left);
        }
        template<typename Cur, typename S>
        RANGES_CXX14_CONSTEXPR
        auto operator-(basic_iterator<Cur> const &left, S const &right) ->
            CPP_ret(typename basic_iterator<Cur>::difference_type)(
                requires detail::SizedCursorSentinel<S, Cur>)
        {
            return -(right - left);
        }
        // Asymmetric comparisons
        template<typename Left, typename Right>
        RANGES_CXX14_CONSTEXPR
        auto operator<(basic_iterator<Left> const &left,
            basic_iterator<Right> const &right) ->
            CPP_ret(bool)(
                requires detail::SizedCursorSentinel<Right, Left>)
        {
            return 0 < (right - left);
        }
        template<typename Left, typename Right>
        RANGES_CXX14_CONSTEXPR
        auto operator<=(basic_iterator<Left> const &left,
            basic_iterator<Right> const &right) ->
            CPP_ret(bool)(
                requires detail::SizedCursorSentinel<Right, Left>)
        {
            return 0 <= (right - left);
        }
        template<typename Left, typename Right>
        RANGES_CXX14_CONSTEXPR
        auto operator>(basic_iterator<Left> const &left,
            basic_iterator<Right> const &right) ->
            CPP_ret(bool)(
                requires detail::SizedCursorSentinel<Right, Left>)
        {
            return (right - left) < 0;
        }
        template<typename Left, typename Right>
        RANGES_CXX14_CONSTEXPR
        auto operator>=(basic_iterator<Left> const &left,
            basic_iterator<Right> const &right) ->
            CPP_ret(bool)(
                requires detail::SizedCursorSentinel<Right, Left>)
        {
            return (right - left) <= 0;
        }

#if RANGES_BROKEN_CPO_LOOKUP
        namespace _basic_iterator_
        {
            // Optionally support hooking iter_move when the cursor sports a
            // move() member function.
            template<typename Cur>
            RANGES_CXX14_CONSTEXPR
            auto iter_move(basic_iterator<Cur> const &it)
            noexcept(noexcept(range_access::move(std::declval<Cur const &>()))) ->
                CPP_broken_friend_ret(decltype(range_access::move(std::declval<Cur const &>())))(
                    requires detail::InputCursor<Cur>)
            {
                return range_access::move(range_access::pos(it));
            }
        }
#endif

        /// Get a cursor from a basic_iterator
        struct get_cursor_fn
        {
            template<typename Cur>
            RANGES_CXX14_CONSTEXPR
            Cur &operator()(basic_iterator<Cur> &it) const noexcept
            {
                return range_access::pos(it);
            }
            template<typename Cur>
            RANGES_CXX14_CONSTEXPR
            Cur const &operator()(basic_iterator<Cur> const &it) const noexcept
            {
                return range_access::pos(it);
            }
            template<typename Cur>
            RANGES_CXX14_CONSTEXPR
            Cur operator()(basic_iterator<Cur> && it) const
                noexcept(std::is_nothrow_move_constructible<Cur>::value)
            {
                return range_access::pos(std::move(it));
            }
        };

        /// \sa `get_cursor_fn`
        /// \ingroup group-utility
        RANGES_INLINE_VARIABLE(get_cursor_fn, get_cursor)
        /// @}
    }
}

/// \cond
namespace concepts
{
    // common_reference specializations for basic_proxy_reference
    template<typename Cur, typename U, typename TQual, typename UQual>
    struct basic_common_reference<::ranges::detail::basic_proxy_reference_<Cur, true>, U, TQual, UQual>
      : basic_common_reference<::ranges::detail::cursor_reference_t<Cur>, U, TQual, UQual>
    {};
    template<typename T, typename Cur, typename TQual, typename UQual>
    struct basic_common_reference<T, ::ranges::detail::basic_proxy_reference_<Cur, true>, TQual, UQual>
      : basic_common_reference<T, ::ranges::detail::cursor_reference_t<Cur>, TQual, UQual>
    {};
    template<typename Cur1, typename Cur2, typename TQual, typename UQual>
    struct basic_common_reference<::ranges::detail::basic_proxy_reference_<Cur1, true>, ::ranges::detail::basic_proxy_reference_<Cur2, true>, TQual, UQual>
      : basic_common_reference<::ranges::detail::cursor_reference_t<Cur1>, ::ranges::detail::cursor_reference_t<Cur2>, TQual, UQual>
    {};

    // common_type specializations for basic_proxy_reference
    template<typename Cur, typename U>
    struct common_type<::ranges::detail::basic_proxy_reference_<Cur, true>, U>
      : common_type<::ranges::range_access::cursor_value_t<Cur>, U>
    {};
    template<typename T, typename Cur>
    struct common_type<T, ::ranges::detail::basic_proxy_reference_<Cur, true>>
      : common_type<T, ::ranges::range_access::cursor_value_t<Cur>>
    {};
    template<typename Cur1, typename Cur2>
    struct common_type<::ranges::detail::basic_proxy_reference_<Cur1, true>, ::ranges::detail::basic_proxy_reference_<Cur2, true>>
      : common_type<::ranges::range_access::cursor_value_t<Cur1>, ::ranges::range_access::cursor_value_t<Cur2>>
    {};
}

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename Cur, bool IsReadable>
            struct std_iterator_traits_;
            template<typename Cur>
            using std_iterator_traits =
                std_iterator_traits_<Cur, (bool) ReadableCursor<Cur>>;

            template<typename Cur, bool IsReadable>
            struct std_iterator_traits_
            {
                using iterator_category = std::output_iterator_tag;
                using difference_type =
                    typename iterator_associated_types_base<Cur>::difference_type;
                using value_type = void;
                using reference = void;
                using pointer = void;
            };

            template<typename Cur>
            struct std_iterator_traits_<Cur, true>
              : iterator_associated_types_base<Cur>
            {
                using iterator_category =
                    ::meta::_t<
                        downgrade_iterator_category<
                            typename std_iterator_traits_::iterator_category,
                            typename std_iterator_traits_::reference>>;
            };
        }
        /// \endcond
    }
}

namespace std
{
    template<typename Cur>
    struct iterator_traits< ::ranges::basic_iterator<Cur>>
      : ::ranges::detail::std_iterator_traits<Cur>
    {};
}
/// \endcond

#endif

// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2016, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_PROXY_FUNCTIONS_DETAIL_HPP_
#define CHAISCRIPT_PROXY_FUNCTIONS_DETAIL_HPP_

#include <functional>
#include <stdexcept>
#include <vector>
#include <array>

#include "../chaiscript_defines.hpp"
#include "boxed_cast.hpp"
#include "boxed_value.hpp"
#include "handle_return.hpp"
#include "type_info.hpp"
#include "callable_traits.hpp"

namespace chaiscript {
class Type_Conversions_State;
namespace exception {
class bad_boxed_cast;
}  // namespace exception
}  // namespace chaiscript

namespace chaiscript
{
  namespace exception
  {
    /**
     * Exception thrown when there is a mismatch in number of
     * parameters during Proxy_Function execution
     */
    struct arity_error : std::range_error
    {
      arity_error(int t_got, int t_expected)
        : std::range_error("Function dispatch arity mismatch"),
        got(t_got), expected(t_expected)
      {
      }

      arity_error(const arity_error &) = default;

      virtual ~arity_error() noexcept {}

      int got;
      int expected;
    };
  }

  namespace dispatch
  {
    namespace detail
    {
      /**
       * Used by Proxy_Function_Impl to return a list of all param types
       * it contains.
       */
      template<typename Ret, typename ... Params>
        std::vector<Type_Info> build_param_type_list(Ret (*)(Params...))
        {
          /// \note somehow this is responsible for a large part of the code generation
          return { user_type<Ret>(), user_type<Params>()... };
        }



      template<size_t ... I>
      struct Indexes
      {
      };

      template<size_t S, size_t ... I> 
      struct Make_Indexes
      {
        typedef typename Make_Indexes<S-1, I..., sizeof...(I)>::indexes indexes;
      };

      template<size_t ... I>
      struct Make_Indexes<0, I...>
      {
        typedef Indexes<I...> indexes;
      };



      /**
       * Used by Proxy_Function_Impl to determine if it is equivalent to another
       * Proxy_Function_Impl object. This function is primarily used to prevent
       * registration of two functions with the exact same signatures
       */
      template<typename Ret, typename ... Params, size_t ... I>
        bool compare_types_cast(Indexes<I...>, Ret (*)(Params...),
             const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
        {
          try {
            (void)params; (void)t_conversions;
            (void)std::initializer_list<int>{(boxed_cast<Params>(params[I], &t_conversions), 0)...};
            return true;
          } catch (const exception::bad_boxed_cast &) {
            return false;
          }

        }

      template<typename Ret, typename ... Params>
        bool compare_types_cast(Ret (*f)(Params...),
             const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
        {
          typedef typename Make_Indexes<sizeof...(Params)>::indexes indexes;
          return compare_types_cast(indexes(), f, params, t_conversions);
        }


      template<typename Callable, typename Ret, typename ... Params, size_t ... I>
        Ret call_func(const chaiscript::dispatch::detail::Function_Signature<Ret (Params...)> &, Indexes<I...>, const Callable &f,
            const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
        {
          (void)params; (void)t_conversions;
          return f(boxed_cast<Params>(params[I], &t_conversions)...);
        }


      /**
       * Used by Proxy_Function_Impl to perform typesafe execution of a function.
       * The function attempts to unbox each parameter to the expected type.
       * if any unboxing fails the execution of the function fails and
       * the bad_boxed_cast is passed up to the caller.
       */
      template<typename Callable, typename Ret, typename ... Params>
        Ret call_func(const chaiscript::dispatch::detail::Function_Signature<Ret (Params...)> &sig, const Callable &f,
            const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
        {
          typedef typename Make_Indexes<sizeof...(Params)>::indexes indexes;
          return call_func(sig, indexes(), f, params, t_conversions);
        }

    }
  }

}


namespace chaiscript
{
  namespace dispatch
  {
    namespace detail
    {
    template<typename Ret>
      struct Do_Call
      {
        template<typename Signature, typename Callable>
          static Boxed_Value go(const Callable &fun, const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
          {
            return Handle_Return<Ret>::handle(call_func(Function_Signature<Signature>(), fun, params, t_conversions));
          }
      };

    template<>
      struct Do_Call<void>
      {
        template<typename Signature, typename Callable>
          static Boxed_Value go(const Callable &fun, const std::vector<Boxed_Value> &params, const Type_Conversions_State &t_conversions)
          {
            call_func(Function_Signature<Signature>(), fun, params, t_conversions);
            return Handle_Return<void>::handle();
          }
      };
    }
  }
}

#endif

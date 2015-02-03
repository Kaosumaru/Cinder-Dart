// Copyright (c) 2013, Richard Eakin and the Dart project authors.
// Use of this source code (and the Dart VM) is governed by a
// BSD-style license that can be found in the LICENSE.txt file.

#pragma once
#include "cidart/Script.h"
#include "cidart/Types.h"

namespace cidart {
	namespace impl
	{
		template<typename T> struct remove_class { };
		template<typename C, typename R, typename... A>
		struct remove_class < R(C::*)(A...) > { using type = R(typename A...); };
		template<typename C, typename R, typename... A>
		struct remove_class < R(C::*)(A...) const > { using type = R(typename A...); };
		template<typename C, typename R, typename... A>
		struct remove_class < R(C::*)(A...) volatile > { using type = R(typename A...); };
		template<typename C, typename R, typename... A>
		struct remove_class < R(C::*)(A...) const volatile > { using type = R(typename A...); };

		template<typename T>
		struct get_signature_impl {
			using type = typename remove_class <
				decltype(&std::remove_reference<T>::type::operator()) > ::type;
		};
		template<typename R, typename... A>
		struct get_signature_impl < R(A...) > { using type = R(typename A...); };
		template<typename R, typename... A>
		struct get_signature_impl < R(&)(A...) > { using type = R(typename A...); };
		template<typename R, typename... A>
		struct get_signature_impl < R(*)(A...) > { using type = R(typename A...); };
		template<typename T> using get_signature = typename get_signature_impl<T>::type;

		template<typename F> using make_function_type = std::function < get_signature<F> > ;

		//! Function that returns std::function<T> for any callable \a f.
		template<typename F> make_function_type<F> make_function(F &&f) {
			return make_function_type<F>(std::forward<F>(f));
		}



		//! Helper class which can take arbitral number of ints as template parameters
		template<int ...> struct integer_sequence { };


		//! Helper class generating incrementing integer_sequece with numbers from 0 to N (integer_sequece<0, 1, ...N>.)
		template<int N, int ...S>
		struct integer_sequence_generator : integer_sequence_generator < N - 1, N - 1, S... > { };

		template<int ...S>
		struct integer_sequence_generator < 0, S... >
		{
			typedef integer_sequence<S...> sequence;
		};


		template<typename ReturnType, typename ...Args>
		class CallableToNativeCallbackWrapper
		{

		};

		template<typename ReturnType, typename ...Args>
		class CallableToNativeCallbackWrapper<std::function<ReturnType(Args...)>>
		{
		public:
			using Function = std::function < ReturnType(Args...) > ;
			static const unsigned short int arity = sizeof...(Args);

			CallableToNativeCallbackWrapper(Function&& callable) : _callable(std::move(callable)){}
			CallableToNativeCallbackWrapper(const Function& callable) : _callable(callable){}

			void operator () (Dart_NativeArguments args)
			{
				//there are two versions of handleCall, choose one corresponding to ReturnType
				handleCall<ReturnType>(args);
			}


		protected:

			//! version of handleCall when ReturnType != void
			template<typename R>
			R handleCall(Dart_NativeArguments args)
			{
				//build sequence of integers <0 .. arity>
				auto sequence = integer_sequence_generator<arity>::sequence();

				//forward Args, and Index should be deduced to <0 .. arity>, 
				auto value = call<Args...>(sequence, args);

				//set return value
				setReturnValue(args, value);

				return value;
			}

			//! version of handleCall when ReturnType == void
			template<>
			void handleCall(Dart_NativeArguments args)
			{
				//build sequence of integers <0 .. arity>
				auto sequence = integer_sequence_generator<arity>::sequence();

				//forward Args, and Index should be deduced to <0 .. arity>, 
				 call<Args...>(sequence, args);
			}

			template<typename ...Args, int ...Index>
			ReturnType call(integer_sequence<Index...>, Dart_NativeArguments args)
			{
				//order of evaluation of arguments is undetermined, but it shouldn't matter in this case
				return _callable(getArg<std::decay<Args>::type>(args, Index)...);
			}


			Function _callable;
		};



		template<typename T>
		class FunctorToNativeFunctionWrapper
		{
		public:
			using Functor = std::function<void(Dart_NativeArguments args)>;

			static void SetFunctor(Functor &&functor)
			{
				if (_functor)
				{
					assert(false);
					return;
				}
				_functor = std::move(functor);
			}

			static void native_wrapper(Dart_NativeArguments arguments)
			{
				_functor(arguments);
			}
		protected:
			static Functor _functor;
		};

		template<typename T>
		typename FunctorToNativeFunctionWrapper<T>::Functor FunctorToNativeFunctionWrapper<T>::_functor;


		template<typename UniqueType, typename T>
		static Dart_NativeFunction wrapFunctor(const std::function<T>& func)
		{
			impl::FunctorToNativeFunctionWrapper<UniqueType>::SetFunctor(impl::CallableToNativeCallbackWrapper<std::function<T>>(func));
			return impl::FunctorToNativeFunctionWrapper<UniqueType>::native_wrapper;
		}
	}


	template<typename UniqueType>
	static Dart_NativeFunction wrapCallable(UniqueType&& func)
	{
		return impl::wrapFunctor<UniqueType>(impl::make_function(func));
	}

	template<void*, typename T>
	static Dart_NativeFunction wrapFunction(T& func)
	{
		auto lambda = [](){};
		using UniqueType = decltype(lambda);
		impl::FunctorToNativeFunctionWrapper<UniqueType>::SetFunctor(impl::CallableToNativeCallbackWrapper<std::function<T>>(func));
		return impl::FunctorToNativeFunctionWrapper<UniqueType>::native_wrapper;
	}

}
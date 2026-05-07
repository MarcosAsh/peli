/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file peli/runtime/registry.h
 * \brief This file defines the PELI global function registry.
 *
 *  The registered functions will be made available to front-end
 *  as well as backend users.
 *
 *  The registry stores type-erased functions.
 *  Each registered function is automatically exposed
 *  to front-end language(e.g. python).
 *
 *  Front-end can also pass callbacks as PackedFunc, or register
 *  then into the same global registry in C++.
 *  The goal is to mix the front-end language and the PELI back-end.
 *
 * \code
 *   // register the function as MyAPIFuncName
 *   PELI_REGISTER_GLOBAL(MyAPIFuncName)
 *   .set_body([](PELIArgs args, PELIRetValue* rv) {
 *     // my code.
 *   });
 * \endcode
 */
#ifndef PELI_RUNTIME_REGISTRY_H_
#define PELI_RUNTIME_REGISTRY_H_

#include <string>
#include <vector>
#include "packed_func.h"

namespace peli {
namespace runtime {

/*! \brief Registry for global function */
class Registry {
 public:
  /*!
   * \brief set the body of the function to be f
   * \param f The body of the function.
   */
  PELI_DLL Registry& set_body(PackedFunc f);  // NOLINT(*)
  /*!
   * \brief set the body of the function to be f
   * \param f The body of the function.
   */
  Registry& set_body(PackedFunc::FType f) {  // NOLINT(*)
    return set_body(PackedFunc(f));
  }
  /*!
   * \brief set the body of the function to be TypedPackedFunc.
   *
   * \code
   *
   * PELI_REGISTER_API("addone")
   * .set_body_typed<int(int)>([](int x) { return x + 1; });
   *
   * \endcode
   *
   * \param f The body of the function.
   * \tparam FType the signature of the function.
   * \tparam FLambda The type of f.
   */
  template<typename FType, typename FLambda>
  Registry& set_body_typed(FLambda f) {
    return set_body(TypedPackedFunc<FType>(f).packed());
  }

  /*!
   * \brief set the body of the function to the given function pointer.
   *        Note that this doesn't work with lambdas, you need to
   *        explicitly give a type for those.
   *        Note that this will ignore default arg values and always require all arguments to be provided.
   *
   * \code
   * 
   * int multiply(int x, int y) {
   *   return x * y;
   * }
   *
   * PELI_REGISTER_API("multiply")
   * .set_body_typed(multiply); // will have type int(int, int)
   *
   * \endcode
   *
   * \param f The function to forward to.
   * \tparam R the return type of the function (inferred).
   * \tparam Args the argument types of the function (inferred).
   */
  template<typename R, typename ...Args>
  Registry& set_body_typed(R (*f)(Args...)) {
    return set_body(TypedPackedFunc<R(Args...)>(f));
  }

  /*!
   * \brief set the body of the function to be the passed method pointer.
   *        Note that this will ignore default arg values and always require all arguments to be provided.
   *
   * \code
   * 
   * // node subclass:
   * struct Example {
   *    int doThing(int x);
   * }
   * PELI_REGISTER_API("Example_doThing")
   * .set_body_method(&Example::doThing); // will have type int(Example, int)
   *
   * \endcode
   *
   * \param f the method pointer to forward to.
   * \tparam T the type containing the method (inferred).
   * \tparam R the return type of the function (inferred).
   * \tparam Args the argument types of the function (inferred).
   */
  template<typename T, typename R, typename ...Args>
  Registry& set_body_method(R (T::*f)(Args...)) {
    return set_body_typed<R(T, Args...)>([f](T target, Args... params) -> R {
      // call method pointer
      return (target.*f)(params...);
    });
  }

  /*!
   * \brief set the body of the function to be the passed method pointer.
   *        Note that this will ignore default arg values and always require all arguments to be provided.
   *
   * \code
   * 
   * // node subclass:
   * struct Example {
   *    int doThing(int x);
   * }
   * PELI_REGISTER_API("Example_doThing")
   * .set_body_method(&Example::doThing); // will have type int(Example, int)
   *
   * \endcode
   *
   * \param f the method pointer to forward to.
   * \tparam T the type containing the method (inferred).
   * \tparam R the return type of the function (inferred).
   * \tparam Args the argument types of the function (inferred).
   */
  template<typename T, typename R, typename ...Args>
  Registry& set_body_method(R (T::*f)(Args...) const) {
    return set_body_typed<R(T, Args...)>([f](const T target, Args... params) -> R {
      // call method pointer
      return (target.*f)(params...);
    });
  }

  /*!
   * \brief set the body of the function to be the passed method pointer.
   *        Used when calling a method on a Node subclass through a NodeRef subclass.
   *        Note that this will ignore default arg values and always require all arguments to be provided.
   *
   * \code
   * 
   * // node subclass:
   * struct ExampleNode: BaseNode {
   *    int doThing(int x);
   * }
   * 
   * // noderef subclass
   * struct Example; 
   *
   * PELI_REGISTER_API("Example_doThing")
   * .set_body_method<Example>(&ExampleNode::doThing); // will have type int(Example, int)
   * 
   * // note that just doing:
   * // .set_body_method(&ExampleNode::doThing);
   * // wouldn't work, because ExampleNode can't be taken from a PELIArgValue.
   *
   * \endcode
   *
   * \param f the method pointer to forward to.
   * \tparam TNodeRef the node reference type to call the method on
   * \tparam TNode the node type containing the method (inferred).
   * \tparam R the return type of the function (inferred).
   * \tparam Args the argument types of the function (inferred).
   */
  template<typename TNodeRef, typename TNode, typename R, typename ...Args,
    typename = typename std::enable_if<std::is_base_of<NodeRef, TNodeRef>::value>::type>
  Registry& set_body_method(R (TNode::*f)(Args...)) {
    return set_body_typed<R(TNodeRef, Args...)>([f](TNodeRef ref, Args... params) {
      TNode* target = ref.operator->();
      // call method pointer
      return (target->*f)(params...);
    });
  }

  /*!
   * \brief set the body of the function to be the passed method pointer.
   *        Used when calling a method on a Node subclass through a NodeRef subclass.
   *        Note that this will ignore default arg values and always require all arguments to be provided.
   *
   * \code
   * 
   * // node subclass:
   * struct ExampleNode: BaseNode {
   *    int doThing(int x);
   * }
   * 
   * // noderef subclass
   * struct Example; 
   *
   * PELI_REGISTER_API("Example_doThing")
   * .set_body_method<Example>(&ExampleNode::doThing); // will have type int(Example, int)
   * 
   * // note that just doing:
   * // .set_body_method(&ExampleNode::doThing);
   * // wouldn't work, because ExampleNode can't be taken from a PELIArgValue.
   *
   * \endcode
   *
   * \param f the method pointer to forward to.
   * \tparam TNodeRef the node reference type to call the method on
   * \tparam TNode the node type containing the method (inferred).
   * \tparam R the return type of the function (inferred).
   * \tparam Args the argument types of the function (inferred).
   */
  template<typename TNodeRef, typename TNode, typename R, typename ...Args,
    typename = typename std::enable_if<std::is_base_of<NodeRef, TNodeRef>::value>::type>
  Registry& set_body_method(R (TNode::*f)(Args...) const) {
    return set_body_typed<R(TNodeRef, Args...)>([f](TNodeRef ref, Args... params) {
      const TNode* target = ref.operator->();
      // call method pointer
      return (target->*f)(params...);
    });
  }

  /*!
   * \brief Register a function with given name
   * \param name The name of the function.
   * \param override Whether allow oveeride existing function.
   * \return Reference to theregistry.
   */
  PELI_DLL static Registry& Register(const std::string& name, bool override = false);  // NOLINT(*)
  /*!
   * \brief Erase global function from registry, if exist.
   * \param name The name of the function.
   * \return Whether function exist.
   */
  PELI_DLL static bool Remove(const std::string& name);
  /*!
   * \brief Get the global function by name.
   * \param name The name of the function.
   * \return pointer to the registered function,
   *   nullptr if it does not exist.
   */
  PELI_DLL static const PackedFunc* Get(const std::string& name);  // NOLINT(*)
  /*!
   * \brief Get the names of currently registered global function.
   * \return The names
   */
  PELI_DLL static std::vector<std::string> ListNames();

  // Internal class.
  struct Manager;

 protected:
  /*! \brief name of the function */
  std::string name_;
  /*! \brief internal packed function */
  PackedFunc func_;
  friend struct Manager;
};

/*! \brief helper macro to supress unused warning */
#if defined(__GNUC__)
#define PELI_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define PELI_ATTRIBUTE_UNUSED
#endif

#define PELI_STR_CONCAT_(__x, __y) __x##__y
#define PELI_STR_CONCAT(__x, __y) PELI_STR_CONCAT_(__x, __y)

#define PELI_FUNC_REG_VAR_DEF                                            \
  static PELI_ATTRIBUTE_UNUSED ::peli::runtime::Registry& __mk_ ## PELI

#define PELI_TYPE_REG_VAR_DEF                                            \
  static PELI_ATTRIBUTE_UNUSED ::peli::runtime::ExtTypeVTable* __mk_ ## PELIT

/*!
 * \brief Register a function globally.
 * \code
 *   PELI_REGISTER_GLOBAL("MyPrint")
 *   .set_body([](PELIArgs args, PELIRetValue* rv) {
 *   });
 * \endcode
 */
#define PELI_REGISTER_GLOBAL(OpName)                              \
  PELI_STR_CONCAT(PELI_FUNC_REG_VAR_DEF, __COUNTER__) =            \
      ::peli::runtime::Registry::Register(OpName)

/*!
 * \brief Macro to register extension type.
 *  This must be registered in a cc file
 *  after the trait extension_type_info is defined.
 */
#define PELI_REGISTER_EXT_TYPE(T)                                 \
  PELI_STR_CONCAT(PELI_TYPE_REG_VAR_DEF, __COUNTER__) =            \
      ::peli::runtime::ExtTypeVTable::Register_<T>()

}  // namespace runtime
}  // namespace peli
#endif  // PELI_RUNTIME_REGISTRY_H_

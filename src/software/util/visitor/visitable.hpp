#pragma once

#include <type_traits>

#include "software/util/visitor/visitor.hpp"

/**
 * Refer to the docs about why we use the Visitor Design Pattern.
 * 
 * A class T should inherit Visitable<T> to indicate that it can be visited by a
 * Visitor<T>. Class T will inherit an `accept` method that allows any Visitor<T> 
 * to visit it.
 *
 * See visitor_test.cpp for an example on how to use Visitor and Visitable.
 *
 * =========================================================================================
 *
 * Implementation explanation:
 *
 * We take advantage of the curiously recurring template pattern (CRTP) to avoid
 * having to forward declare visitable classes in a visitor's header file (which is
 * necessary in the traditional implementation of the visitor pattern).
 *
 * See: https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 *
 * To explain how CRTP works here, assume we have some class T that inherits
 * Visitable<T>. The function Visitable<T>::accept will not be instantiated until it
 * is called by some later code *after* T is declared. Hence, the declaration of T is
 * actually known to the compiler at the time Visitable<T>::accept is instantiated.
 *
 * We have effectively created a virtual function `accept` inherited by all classes
 * that derive from Visitable, but this "virtual" function can be a function template
 * (normally, member function templates cannot be declared virtual). This allows us to
 * make the Visitor type a template parameter, so a concrete Visitor class does not
 * need to be referenced in the declaration of classes that derive from Visitable.
 * This removes all circular dependencies we would normally have in a traditional
 * visitor pattern implementation.
 * 
 * This code is adapted from https://stackoverflow.com/a/7877397.
 *
 * @tparam T the class that should be made visitable
 */
template <typename T>
class Visitable
{
   public:
    /**
     * Accepts a Visitor and calls the visit function on itself.
     *
     * @tparam TVisitor the Visitor which has a visit method for T
     *
     * @param visitor a Visitor of type TVisitor
     */
    template <typename TVisitor>
    requires std::is_base_of<Visitor<T>, TVisitor>::value
    void accept(TVisitor& visitor) 
    {
        visitor.visit(static_cast<T&>(*this));
    }
};

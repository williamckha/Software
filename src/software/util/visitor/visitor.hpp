#pragma once

/**
 * This "acyclic visitor pattern" code is adapted from the
 * Loki C++ Template Library, developed by Andrei Alexandrescu.
 *
 * https://github.com/dutor/loki/blob/master/Reference/Visitor.h
 *
 * Refer to the docs about why we use the Visitor Design Pattern.
 */

/**
 * The base class that all visitors should inherit.
 */
class BaseVisitor
{
   public:
    virtual ~BaseVisitor() = default;
};

/**
 * A class should inherit Visitor<T> to indicate that it can visit an instance of T.
 * The inheriting class must override and implement the virtual `visit` method.
 *
 * @example See visitor_test.cpp for an example on how to use Visitor and Visitable.
 *
 * @tparam T the class that this Visitor can visit
 * @tparam R the return type of the Visitor's visit method
 */
template <typename T, typename R = void>
class Visitor
{
   public:
    /**
     * Visits an instance of T to perform an operation.
     *
     * @param visitable the instance of T to visit
     *
     * @return the result of the operation on T
     */
    virtual R visit(T& visitable) = 0;
};

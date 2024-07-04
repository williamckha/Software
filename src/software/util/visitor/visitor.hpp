#pragma once

/**
 * Refer to the docs about why we use the Visitor Design Pattern.
 * 
 * A class should inherit Visitor<T> to indicate that it can visit an instance of T.
 * The inheriting class must override and implement Visitor<T>::visit.
 * 
 * See visitor_test.cpp for an example on how to use Visitor and Visitable.
 *
 * @tparam T the class that this Visitor can visit
 */
template <typename T>
class Visitor
{
   public:
    /**
     * Visits an instance of T to perform an operation.
     * 
     * @param visitable the instance of T to visit
     */
    virtual void visit(T& visitable) = 0;
};

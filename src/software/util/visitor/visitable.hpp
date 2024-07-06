#pragma once

#include "software/logger/logger.h"
#include "software/util/typename/typename.h"
#include "software/util/visitor/visitor.hpp"

/**
 * This "acyclic visitor pattern" code is adapted from the
 * Loki C++ Template Library, developed by Andrei Alexandrescu.
 *
 * https://github.com/dutor/loki/blob/master/Reference/Visitor.h
 *
 * Refer to the docs about why we use the Visitor Design Pattern.
 */

/**
 * A class should inherit Visitable<R> to indicate that it can be visited by a
 * Visitor<R>. The class will inherit and must implement a virtual `accept` method
 * that allows any Visitor<R> to visit it.
 *
 * @example See visitor_test.cpp for an example on how to use Visitor and Visitable.
 *
 * @tparam R the return type of the visit operation
 */
template <typename R = void>
class Visitable
{
   public:
    /**
     * Accepts a visitor and calls the visit method on itself.
     *
     * NOTE: You can use the DEFINE_VISITABLE macro to quickly implement this method.
     *
     * @param visitor the visitor to accept
     *
     * @return the result of the operation that the visitor performed
     * on this instance
     */
    virtual R accept(BaseVisitor& visitor) = 0;

   protected:
    // Type alias for R so that it can be used in DEFINE_VISITABLE
    using VisitReturnType = R;

    /**
     * Internal implementation of the `accept` method.
     * If the visitor has a `visit` method for the visitable instance, it will
     * call `visit` on the instance. Otherwise, this method will fail.
     *
     * @tparam T the type of the visitable instance
     *
     * @param visitable the instance that the visitor should visit
     * @param visitor the visitor
     *
     * @return the result of the operation that the visitor performed
     * on the visitable
     */
    template <typename T>
    R acceptImpl(T& visitable, BaseVisitor& visitor)
    {
        if (Visitor<T, R>* v = dynamic_cast<Visitor<T, R>*>(&visitor))
        {
            return v->visit(visitable);
        }

        LOG(FATAL) << "Visitor " << TYPENAME(visitor)
                   << " does not have a visit method for " << TYPENAME(visitable);
        __builtin_unreachable();
    }
};

#define DEFINE_VISITABLE                                                                 \
    VisitReturnType accept(BaseVisitor& visitor) override                                \
    {                                                                                    \
        return acceptImpl(*this, visitor);                                               \
    }

#include "software/util/visitor/visitor.hpp"
#include "software/util/visitor/visitable.hpp"

#include <gtest/gtest.h>

#include <string>

struct A : public Visitable<A>
{
    int foo = 1;
};

struct B : public Visitable<B>
{
    double bar = 2;
};

struct ABVisitor : public Visitor<A>, public Visitor<B>
{
    void visit(A& visitable)
    {
        value = std::to_string(visitable.foo);
    }

    void visit(B& visitable)
    {
        value = std::to_string(visitable.bar);
    }

    std::string value;
};

TEST(VisitorTest, test_visitor_and_visitable)
{
    ABVisitor visitor;
    A a;
    B b;

    a.accept(visitor);
    EXPECT_EQ(std::to_string(a.foo), visitor.value);

    b.accept(visitor);
    EXPECT_EQ(std::to_string(b.bar), visitor.value);
}

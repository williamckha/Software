#include "software/util/visitor/visitor.hpp"

#include <gtest/gtest.h>

#include <string>
#include <memory>

#include "software/util/visitor/visitable.hpp"

struct Animal : public Visitable<int>
{
};

struct Dog : public Animal
{
    DEFINE_VISITABLE

    int foo = 1;
};

struct Cat : public Animal
{
    DEFINE_VISITABLE

    int bar = 2;
};

struct AnimalVisitor : public BaseVisitor,
                       public Visitor<Dog, int>,
                       public Visitor<Cat, int>
{
    int visit(Dog& visitable) override
    {
        return visitable.foo;
    }

    int visit(Cat& visitable) override
    {
        return visitable.bar;
    }
};

TEST(VisitorTest, test_visitor_and_visitable)
{
    AnimalVisitor visitor;
    std::unique_ptr<Animal> dog = std::make_unique<Dog>();
    std::unique_ptr<Animal> cat = std::make_unique<Cat>();

    EXPECT_EQ(dog->accept(visitor), 1);
    EXPECT_EQ(cat->accept(visitor), 2);
}

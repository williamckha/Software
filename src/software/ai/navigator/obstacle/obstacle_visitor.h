#pragma once

#include "software/ai/navigator/obstacle/geom_obstacle.hpp"
#include "software/geom/circle.h"
#include "software/geom/polygon.h"
#include "software/geom/rectangle.h"
#include "software/geom/stadium.h"
#include "software/util/visitor/visitor.hpp"

/**
 * This class provides an interface for all Obstacle Visitors. The Visitor design pattern
 * allows us to perform operations on Obstacle objects without needing to check which
 * concrete type it is with an if/else statement, and we don't need to pollute the
 * Obstacle classes with information or functions that are specific to the task we
 * want to perform.
 */
class ObstacleVisitor : public BaseVisitor,
                        public Visitor<GeomObstacle<Circle>>,
                        public Visitor<GeomObstacle<Polygon>>,
                        public Visitor<GeomObstacle<Rectangle>>,
                        public Visitor<GeomObstacle<Stadium>>
{
   public:
    virtual ~ObstacleVisitor() = default;
};

#pragma once

#include "software/util/make_enum/make_enum.h"

MAKE_ENUM(GpioState, LOW, HIGH);
MAKE_ENUM(GpioDirection, INPUT, OUTPUT);

class Gpio
{
    public:
        /**
         * Set the value to the provided state
         *
         * @param state The state
         */
        virtual void setValue(GpioState state) = 0;

        /**
         * Get the current state of the gpio
         */
        virtual GpioState getValue() = 0;
};

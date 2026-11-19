/**
 * @file main.cpp
 * @author Alan Abraham P Kochumon
 * @date Created on: May 29, 2026
 *
 * @brief Demo application main entry point.
 *
 * @copyright Copyright (c) 2026 Alan Abraham P Kochumon
 */


#include "colloid/Application.h"

int main()
{
    colloid::Application application("Colloid", "1.0.0", "com.colloid.rasterizer");
    application.run();
}

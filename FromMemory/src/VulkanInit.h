//
// Created by johnm on 8/7/2026.
//

#ifndef VULKANINIT_H
#define VULKANINIT_H

#include <vector>
#include <string>
#include "volk.h"

// #JOHN - figure out if I even need the required layers
VkInstance CreateInstance(const std::vector<std::string> RequiredLayers,
                          const std::vector<const char*>& RequiredExtensions);



#endif //VULKANINIT_H

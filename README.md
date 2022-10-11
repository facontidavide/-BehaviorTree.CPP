![License MIT](https://img.shields.io/github/license/BehaviorTree/BehaviorTree.CPP?color=blue)
![Version](https://img.shields.io/badge/version-3.8-blue.svg)
[![cmake](https://github.com/BehaviorTree/BehaviorTree.CPP/actions/workflows/cmake.yml/badge.svg)](https://github.com/BehaviorTree/BehaviorTree.CPP/actions/workflows/cmake.yml)
[![ros1](https://github.com/BehaviorTree/BehaviorTree.CPP/workflows/ros1/badge.svg?branch=master)](https://github.com/BehaviorTree/BehaviorTree.CPP/actions?query=workflow%3Aros1)
[![ros2](https://github.com/BehaviorTree/BehaviorTree.CPP/workflows/ros2/badge.svg?branch=master)](https://github.com/BehaviorTree/BehaviorTree.CPP/actions?query=workflow%3Aros2)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/BehaviorTree/BehaviorTree.CPP)](https://lgtm.com/projects/g/BehaviorTree/BehaviorTree.CPP/context:cpp)
![Discourse topics](https://img.shields.io/discourse/topics?server=https%3A%2F%2Fdiscourse.behaviortree.dev)

# BehaviorTree.CPP

<p align="center"><img src="docs/images/ReadTheDocs.png"></p>

This  __C++ 14__ library provides a framework to create BehaviorTrees.
It was designed to be flexible, easy to use, reactive and fast.

Even if our main use-case is __robotics__, you can use this library to build
__AI for games__, or to replace Finite State Machines in your application.

There are few features that make __BehaviorTree.CPP__ unique, when compared to other implementations:

- It makes __asynchronous Actions__, i.e. non-blocking, a first-class citizen.

- You can build __reactive__ behaviors that execute multiple Actions concurrently.

- Trees are defined using a Domain Specific Scripting __scripting language__ (based on XML), and can be loaded at run-time; in other words, even if written in C++, Trees are _not_ hard-coded.

- You can statically link your custom TreeNodes or convert them into __plugins__
which can be loaded at run-time.

- It provides a type-safe and flexible mechanism to do __Dataflow__ between
  Nodes of the Tree.

- It includes a __logging/profiling__ infrastructure that allows the user 
to visualize, record, replay and analyze state transitions.

- Last but not least: it is well [documented](https://www.behaviortree.dev/)!

## Documentation

You can learn about the main concepts, the API and the tutorials here: https://www.behaviortree.dev/

To find more details about the conceptual ideas that make this implementation different from others,
you can read the [final deliverable of the project MOOD2Be](https://github.com/BehaviorTree/BehaviorTree.CPP/blob/master/MOOD2Be_final_report.pdf).

## Forum and Community

If the documentation doesn't answer your questions and/or you want to
connect with the other **BT.CPP** users, visit https://discourse.behaviortree.dev/

## Commercial support

Are you using BT.CPP in your commercial product and you need technical support / consulting?
You can get in touch at dfaconti@aurynrobotics.com and we will happy to discuss your use case and needs.

# Design principles

The main goal of this project is to create a Behavior Tree implementation
that uses the principles of Model Driven Development to separate the role 
of the __Component Developer__ from the __Behavior Designer__.

In practice, this means that:

- Custom TreeNodes must be reusable building blocks. 
 You should be able to implement them once and reuse them to build many behaviors.

- To build a Behavior Tree out of TreeNodes, the Behavior Designer must 
not need to read nor modify the C++ source code of a given TreeNode.

- Complex Behaviours must be composable using Subtrees

Many of the features and, sometimes, the apparent limitations of this library, might be a consequence 
of this design principle. 

For instance, having a scoped BlackBoard, visible only in a portion of the tree, is particularly important 
to avoid "name pollution" and allow the creation of large scale trees.

# GUI Editor

Editing a BehaviorTree is as simple as editing a XML file in your favourite text editor.

If you are looking for a more fancy graphical user interface (and I know you do) check 
[Groot](https://github.com/BehaviorTree/Groot) out.

![Groot screenshot](docs/groot-screenshot.png)

## Watch Groot and BehaviorTree.CPP in action

Click on the following image to see a short video of how the C++ library and
the graphic user interface are used to design and monitor a Behavior Tree.

[![MOOD2Be](docs/video_MOOD2Be.png)](https://vimeo.com/304651183)

# How to compile (plain old cmake)

On Ubuntu, you are encourage to install the following dependencies:

     sudo apt-get install libzmq3-dev libboost-dev
     
Other dependencies are already included in the __3rdparty__ folder.

To compile and install the library, from the BehaviorTree.CPP folder, execute:

     mkdir build; cd build
     cmake ..
     make
     sudo make install

If you want to use BT.CPp in your application a typical **CMakeLists.txt** file 
will look like this:

```cmake
cmake_minimum_required(VERSION 3.5)

project(hello_BT)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(behaviortree_cpp_v3)

add_executable(${PROJECT_NAME} "hello_BT.cpp")
target_link_libraries(${PROJECT_NAME} BT::behaviortree_cpp_v3)
```

## ROS1 or ROS2 users (Catkin/Ament)

You can easily install the package with the command

      sudo apt-get install ros-$ROS_DISTRO-behaviortree-cpp-v3
      
If you want to compile it with catkin, you __must__ include this package 
to your catkin workspace.

## Installing BehaviorTree.CPP (vcpkg)

Alternatively, you can build and install behaviortree-cpp using [vcpkg](https://github.com/Microsoft/vcpkg/) dependency manager:

    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg integrate install
    ./vcpkg install behaviortree-cpp

The behaviortree-cpp port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

# Acknowledgement

This library was initially developed at  **Eurecat - https://eurecat.org/en/** (main author, Davide Faconti) in a joint effort
with the **Italian Institute of Technology** (Michele Colledanchise).

This software is one of the main components of [MOOD2Be](https://eurecat.org/en/portfolio-items/mood2be/),
which is one of the six **Integrated Technical Projects (ITPs)** selected from the
[RobMoSys first open call](https://robmosys.eu/itp/). Therefore, MOOD2Be has been supported by the European Horizon2020 project RobMoSys. This software is RobMoSys conformant. 

![RobMoSys Conformant](docs/robmosys_conformant_logo.png)

# Further readings

- Introductory article: [Behavior trees for AI: How they work](http://www.gamasutra.com/blogs/ChrisSimpson/20140717/221339/Behavior_trees_for_AI_How_they_work.php)

- **How Behavior Trees Modularize Hybrid Control Systems and Generalize 
Sequential Behavior Compositions, the Subsumption Architecture,
and Decision Trees.** 
Michele Colledanchise and Petter Ogren. IEEE Transaction on Robotics 2017.

- **Behavior Trees in Robotics and AI**, 
published by CRC Press Taylor & Francis, available for purchase
(ebook and hardcover) on the CRC Press Store or Amazon.

The Preprint version (free) is available here: https://arxiv.org/abs/1709.00084

# License

The MIT License (MIT)

Copyright (c) 2014-2018 Michele Colledanchise

Copyright (c) 2018-2019 Davide Faconti, Eurecat

Copyright (c) 2019-2021 Davide Faconti

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

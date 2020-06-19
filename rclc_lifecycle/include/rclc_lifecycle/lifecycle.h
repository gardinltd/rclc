// Copyright (c) 2019 - for information on the respective copyright owner
// see the NOTICE file and/or the repository https://github.com/ros2/rclc.
// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rclc/node.h"
#include <rcl_lifecycle/rcl_lifecycle.h>

#include <rcl/error_handling.h>
#include <rcutils/logging_macros.h>


typedef struct rclc_lifecycle_callback_map_t
{
  bool goal_states[100];  // highest transition id: 99
  int (*fun_ptrs[100])(void);
} rclc_lifecycle_callback_map_t;

/// Structure which encapsulates a ROS Lifecycle Node.
typedef struct rclc_lifecycle_node_t
{
  rcl_node_t *node;
  rcl_lifecycle_state_machine_t *state_machine;
  rclc_lifecycle_callback_map_t callbacks;
} rclc_lifecycle_node_t;


rclc_lifecycle_node_t
rclc_make_node_a_lifecycle_node(
  rcl_node_t *node,
  rcl_lifecycle_state_machine_t *state_machine,
  const rcl_node_options_t *options);
  
rcl_ret_t
rclc_lifecycle_change_state(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int transition_id,
  bool publish_update);

rcl_ret_t
rclc_lifecycle_register_callback(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int goal_state,
  int (*cb)(void)
  );

rcl_ret_t
rclc_lifecycle_register_on_configure(
  rclc_lifecycle_node_t *node,
  int (*cb)(void));

rcl_ret_t
rclc_lifecycle_register_on_activate(
  rclc_lifecycle_node_t *node,
  int (*cb)(void));

rcl_ret_t
rclc_lifecycle_register_on_deactivate(
  rclc_lifecycle_node_t *node,
  int (*cb)(void));

rcl_ret_t
rclc_lifecycle_register_on_cleanup(
  rclc_lifecycle_node_t *node,
  int (*cb)(void));

rcl_ret_t
rclc_lifecycle_execute_callback(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int transition_id);

rcl_ret_t
rcl_lifecycle_node_fini(
  rclc_lifecycle_node_t * node,
  const rcl_node_options_t *options);

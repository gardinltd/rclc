#include "rclc_lifecycle/lifecycle.h"

#include <rcl/error_handling.h>
#include <rcutils/logging_macros.h>

#include <rcl_lifecycle/rcl_lifecycle.h>
#include <rcl_lifecycle/transition_map.h>

#include <lifecycle_msgs/msg/transition_description.h>
#include <lifecycle_msgs/msg/transition_event.h>
#include <lifecycle_msgs/srv/change_state.h>
#include <lifecycle_msgs/srv/get_state.h>
#include <lifecycle_msgs/srv/get_available_states.h>
#include <lifecycle_msgs/srv/get_available_transitions.h>


rclc_lifecycle_node_t
rclc_make_node_a_lifecycle_node(
  rcl_node_t *node,
  rcl_lifecycle_state_machine_t *state_machine,
  const rcl_node_options_t *node_ops)
{   
  rclc_lifecycle_node_t lifecycle_node;
  
  rcl_ret_t rcl_ret = rcl_lifecycle_state_machine_init(
    state_machine,
    node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(lifecycle_msgs, msg, TransitionEvent),
    ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, ChangeState),
    ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetState),
    ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetAvailableStates),
    ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetAvailableTransitions),
    ROSIDL_GET_SRV_TYPE_SUPPORT(lifecycle_msgs, srv, GetAvailableTransitions),
    true,
    &node_ops->allocator);
  if (rcl_ret != RCL_RET_OK) {
    // state machine not initialized, return uninitilized
    // @todo(anordman): how/what to return in this case?
    RCUTILS_LOG_ERROR("Unable to initialize state machine: %s",
      rcl_get_error_string().str);
    return lifecycle_node;
  }
  
  lifecycle_node.node = node;
  lifecycle_node.state_machine = state_machine;
  
  return lifecycle_node;
}

rcl_ret_t
rclc_lifecycle_change_state(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int transition_id,
  bool publish_update)
{
  if (rcl_lifecycle_state_machine_is_initialized(lifecycle_node->state_machine) != RCL_RET_OK) {
    RCUTILS_LOG_ERROR(
      "Unable to change state for state machine: %s",
      rcl_get_error_string().str);
    return RCL_RET_ERROR;
  }

  // keep the initial state to pass to a trclc_lifecycle_cbsransition callback
  const rcl_lifecycle_state_t current_state = *(lifecycle_node->state_machine->current_state);
  
  if (
    rcl_lifecycle_trigger_transition_by_id(
      lifecycle_node->state_machine, transition_id, publish_update) != RCL_RET_OK)
  {
    RCUTILS_LOG_ERROR(
      "Unable to start transition %u from current state %s: %s",
      transition_id, lifecycle_node->state_machine->current_state->label, rcl_get_error_string().str);
    rcutils_reset_error();
    return RCL_RET_ERROR;
  }
  
  // Check for callbacks for this transition
  if (rclc_lifecycle_execute_callback(lifecycle_node, transition_id) == RCL_RET_OK) {
    // successfull, so transition do according success transition
    switch(transition_id) {
      case lifecycle_msgs__msg__Transition__TRANSITION_CONFIGURE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_CONFIGURE_SUCCESS,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_ACTIVATE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_ACTIVATE_SUCCESS,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_DEACTIVATE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_DEACTIVATE_SUCCESS,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_CLEANUP:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_CLEANUP_SUCCESS,
          publish_update);
        break;
    }
  } else {
    // failed, so transition to fail
    switch(transition_id) {
      case lifecycle_msgs__msg__Transition__TRANSITION_CONFIGURE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_CONFIGURE_FAILURE,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_ACTIVATE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_ACTIVATE_FAILURE,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_DEACTIVATE:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_DEACTIVATE_FAILURE,
          publish_update);
        break;
      case lifecycle_msgs__msg__Transition__TRANSITION_CLEANUP:
        return rcl_lifecycle_trigger_transition_by_id(
          lifecycle_node->state_machine,
          lifecycle_msgs__msg__Transition__TRANSITION_ON_CLEANUP_FAILURE,
          publish_update);
        break;
    }
  }

  // This true holds in both cases where the actual callback
  // was successful or not, since at this point we have a valid transistion
  // to either a new primary state or error state
  return RCL_RET_OK;
}

rcl_ret_t
rclc_lifecycle_register_callback(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int goal_state,
  int (*cb)(void)
  )
{
  RCL_CHECK_FOR_NULL_WITH_MSG(
    lifecycle_node, "node is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    goal_state, "goal_state is a null pointer", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    cb, "callback is a null pointer", return RCL_RET_INVALID_ARGUMENT);

  lifecycle_node->callbacks.goal_states[goal_state] = true;
  lifecycle_node->callbacks.fun_ptrs[goal_state] = cb;
  
  return 0;
}

rcl_ret_t
rclc_lifecycle_register_on_configure(
  rclc_lifecycle_node_t * node,
  int (*cb)(void))
{
  return rclc_lifecycle_register_callback(
    node,
    lifecycle_msgs__msg__Transition__TRANSITION_CONFIGURE,
    cb);
}

rcl_ret_t
rclc_lifecycle_register_on_activate(
  rclc_lifecycle_node_t * node,
  int (*cb)(void))
{
  return rclc_lifecycle_register_callback(
    node,
    lifecycle_msgs__msg__Transition__TRANSITION_ACTIVATE,
    cb);
}

rcl_ret_t
rclc_lifecycle_register_on_deactivate(
  rclc_lifecycle_node_t * node,
  int (*cb)(void))
{
  return rclc_lifecycle_register_callback(
    node,
    lifecycle_msgs__msg__Transition__TRANSITION_DEACTIVATE,
    cb);
}

rcl_ret_t
rclc_lifecycle_register_on_cleanup(
  rclc_lifecycle_node_t * node,
  int (*cb)(void))
{
  return rclc_lifecycle_register_callback(
    node,
    lifecycle_msgs__msg__Transition__TRANSITION_CLEANUP,
    cb);
}

rcl_ret_t
rcl_lifecycle_node_fini(
  rclc_lifecycle_node_t * lifecycle_node,
  const rcl_node_options_t *options)
{
  rcl_ret_t rcl_ret = RCL_RET_OK;
  
  // Cleanup statemachine
  rcl_ret = rcl_lifecycle_state_machine_fini(
    lifecycle_node->state_machine,
    lifecycle_node->node,
    &options->allocator
    );
  if (rcl_ret != RCL_RET_OK) {
    return RCL_RET_ERROR;
  }
  
  // Cleanup node
  rcl_ret = rcl_node_fini(lifecycle_node->node);
  if (rcl_ret != RCL_RET_OK) {
    return RCL_RET_ERROR;
  }
  
  return rcl_ret;
}

rcl_ret_t
rclc_lifecycle_execute_callback(
  rclc_lifecycle_node_t *lifecycle_node,
  unsigned int transition_id)
{
  if (!lifecycle_node->callbacks.goal_states[transition_id]) {
    // no callback, so process, all good
    return RCL_RET_OK;
  }
  
  return (*lifecycle_node->callbacks.fun_ptrs[transition_id])();
}

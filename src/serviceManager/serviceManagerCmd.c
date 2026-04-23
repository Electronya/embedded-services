/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      serviceManagerCmd.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Shell Commands
 *
 *            Shell command implementations for the service manager module.
 *            Provides commands to query service status and control services.
 *
 * @ingroup   serviceManager
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "serviceManager.h"
#include "serviceManagerUtil.h"

/**
 * @brief   Service state display strings.
 */
static const char *svcStateStr[] = {
  [SVC_STATE_STOPPED]   = "stopped",
  [SVC_STATE_RUNNING]   = "running",
  [SVC_STATE_SUSPENDED] = "suspended",
};

/**
 * @brief   Service priority display strings.
 */
static const char *svcPriorityStr[] = {
  [SVC_PRIORITY_CRITICAL]    = "critical",
  [SVC_PRIORITY_CORE]        = "core",
  [SVC_PRIORITY_APPLICATION] = "application",
};

/**
 * @brief   Execute the list command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of arguments.
 * @param[in]   argv: The vector of arguments.
 *
 * @return  Always returns 0.
 */
static int execLs(const struct shell *shell, size_t argc, char **argv)
{
  size_t index = 0;
  ServiceDescriptor_t *descriptor;

  shell_print(shell, "%-5s %-20s %-12s %-10s %-15s %s",
              "Index", "Name", "Priority", "State", "Heartbeat(ms)", "Missed");

  while((descriptor = serviceMngrUtilGetRegEntryByIndex(index)) != NULL)
  {
    shell_print(shell, "%-5zu %-20s %-12s %-10s %-15u %u",
                index,
                k_thread_name_get(descriptor->threadId),
                svcPriorityStr[descriptor->priority],
                svcStateStr[descriptor->state],
                descriptor->heartbeatIntervalMs,
                descriptor->missedHeartbeats);
    index++;
  }

  return 0;
}

/**
 * @brief   Execute the start command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of arguments.
 * @param[in]   argv: The vector of arguments.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int execStart(const struct shell *shell, size_t argc, char **argv)
{
  int err = 0;
  unsigned long index;

  index = shell_strtoul(argv[1], 10, &err);
  if(err)
  {
    shell_error(shell, "FAIL: invalid service index argument");
    shell_help(shell);
    return -EINVAL;
  }

  err = serviceMngrUtilStartService(index);
  if(err)
  {
    shell_error(shell, "FAIL %d: unable to start service %lu", err, index);
    return err;
  }

  shell_print(shell, "SUCCESS: service %lu started", index);
  return 0;
}

/**
 * @brief   Execute the stop command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of arguments.
 * @param[in]   argv: The vector of arguments.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int execStop(const struct shell *shell, size_t argc, char **argv)
{
  int err = 0;
  unsigned long index;

  index = shell_strtoul(argv[1], 10, &err);
  if(err)
  {
    shell_error(shell, "FAIL: invalid service index argument");
    shell_help(shell);
    return -EINVAL;
  }

  err = serviceMngrUtilStopService(index);
  if(err)
  {
    shell_error(shell, "FAIL %d: unable to stop service %lu", err, index);
    return err;
  }

  shell_print(shell, "SUCCESS: service %lu stopped", index);
  return 0;
}

/**
 * @brief   Execute the suspend command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of arguments.
 * @param[in]   argv: The vector of arguments.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int execSuspend(const struct shell *shell, size_t argc, char **argv)
{
  int err = 0;
  unsigned long index;

  index = shell_strtoul(argv[1], 10, &err);
  if(err)
  {
    shell_error(shell, "FAIL: invalid service index argument");
    shell_help(shell);
    return -EINVAL;
  }

  err = serviceMngrUtilSuspendService(index);
  if(err)
  {
    shell_error(shell, "FAIL %d: unable to suspend service %lu", err, index);
    return err;
  }

  shell_print(shell, "SUCCESS: service %lu suspended", index);
  return 0;
}

/**
 * @brief   Execute the resume command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of arguments.
 * @param[in]   argv: The vector of arguments.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int execResume(const struct shell *shell, size_t argc, char **argv)
{
  int err = 0;
  unsigned long index;

  index = shell_strtoul(argv[1], 10, &err);
  if(err)
  {
    shell_error(shell, "FAIL: invalid service index argument");
    shell_help(shell);
    return -EINVAL;
  }

  err = serviceMngrUtilResumeService(index);
  if(err)
  {
    shell_error(shell, "FAIL %d: unable to resume service %lu", err, index);
    return err;
  }

  shell_print(shell, "SUCCESS: service %lu resumed", index);
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(svcMgr_sub,
  SHELL_CMD(ls, NULL, "List registered services.", execLs),
  SHELL_CMD_ARG(start, NULL, "Start a service. Usage: start <index>", execStart, 2, 0),
  SHELL_CMD_ARG(stop, NULL, "Stop a service. Usage: stop <index>", execStop, 2, 0),
  SHELL_CMD_ARG(suspend, NULL, "Suspend a service. Usage: suspend <index>", execSuspend, 2, 0),
  SHELL_CMD_ARG(resume, NULL, "Resume a service. Usage: resume <index>", execResume, 2, 0),
  SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(srv_mgr, &svcMgr_sub, "Service manager commands.", NULL);

/** @} */

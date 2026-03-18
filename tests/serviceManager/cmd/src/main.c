/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Command Tests
 *
 *            Unit tests for service manager shell command functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DEFINE_FFF_GLOBALS;

/* Prevent shell.h inclusion */
#define SHELL_H__

/* Prevent serviceManagerUtil.h inclusion - we'll mock the functions */
#define SERVICE_MANAGER_UTIL_H

/* Provide shell types */
struct shell;

enum shell_vt100_color {
  SHELL_NORMAL = 0,
  SHELL_INFO,
  SHELL_ERROR,
  SHELL_WARNING
};

/* Captured shell output */
#define MAX_SHELL_CALLS 16
static char captured_shell_outputs[MAX_SHELL_CALLS][256];
static int shell_print_call_count = 0;
static int shell_error_call_count = 0;

/* Mock for shell_fprintf (what shell_print/shell_error expand to) */
void shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
                   const char *fmt, ...)
{
  va_list args;

  ARG_UNUSED(shell);

  if(shell_print_call_count < MAX_SHELL_CALLS)
  {
    va_start(args, fmt);
    vsnprintf(captured_shell_outputs[shell_print_call_count],
              sizeof(captured_shell_outputs[0]), fmt, args);
    va_end(args);
    shell_print_call_count++;
  }

  if(color == SHELL_ERROR)
    shell_error_call_count++;
}

/* Mock Kconfig options */
#define CONFIG_ENYA_SERVICE_MANAGER_SHELL 1
#define CONFIG_ENYA_SERVICE_MANAGER 1
#define CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL 3

/* Include serviceManager.h for type definitions */
#include "serviceManager.h"

/* Mock shell functions */
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);
FAKE_VOID_FUNC(shell_help, const struct shell *);

/* Mock utility functions */
FAKE_VALUE_FUNC(ServiceDescriptor_t *, serviceMngrUtilGetRegEntryByIndex, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilStartService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilStopService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilSuspendService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilResumeService, size_t);

/* Mock kernel functions */
#define k_thread_name_get k_thread_name_get_mock
FAKE_VALUE_FUNC(const char *, k_thread_name_get_mock, k_tid_t);

/* Define shell macros for testing */
#define shell_print(sh, fmt, ...) shell_fprintf(sh, SHELL_NORMAL, fmt, ##__VA_ARGS__)
#define shell_error(sh, fmt, ...) shell_fprintf(sh, SHELL_ERROR, fmt, ##__VA_ARGS__)
#define SHELL_CMD(...)
#define SHELL_CMD_ARG(...)
#define SHELL_SUBCMD_SET_END
#define SHELL_STATIC_SUBCMD_SET_CREATE(...)
#define SHELL_CMD_REGISTER(...)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(serviceManager, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

#define FFF_FAKES_LIST(FAKE) \
  FAKE(shell_strtoul) \
  FAKE(shell_help) \
  FAKE(serviceMngrUtilGetRegEntryByIndex) \
  FAKE(serviceMngrUtilStartService) \
  FAKE(serviceMngrUtilStopService) \
  FAKE(serviceMngrUtilSuspendService) \
  FAKE(serviceMngrUtilResumeService) \
  FAKE(k_thread_name_get_mock)

/* Include command implementation */
#include "serviceManagerCmd.c"

/* Test descriptors */
static ServiceDescriptor_t ls_test_descriptors[3];

/* Custom fake: returns a descriptor for indices 0-2, NULL for index 3+ */
static ServiceDescriptor_t *getRegEntry_withServices(size_t index)
{
  if(index < 3)
    return &ls_test_descriptors[index];
  return NULL;
}

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *cmd_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void cmd_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  memset(captured_shell_outputs, 0, sizeof(captured_shell_outputs));
  shell_print_call_count = 0;
  shell_error_call_count = 0;

  /* Initialize test descriptors */
  memset(ls_test_descriptors, 0, sizeof(ls_test_descriptors));

  ls_test_descriptors[0].threadId = (k_tid_t)0x1000;
  ls_test_descriptors[0].priority = SVC_PRIORITY_CRITICAL;
  ls_test_descriptors[0].state = SVC_STATE_RUNNING;
  ls_test_descriptors[0].heartbeatIntervalMs = 500;
  ls_test_descriptors[0].missedHeartbeats = 0;

  ls_test_descriptors[1].threadId = (k_tid_t)0x2000;
  ls_test_descriptors[1].priority = SVC_PRIORITY_CORE;
  ls_test_descriptors[1].state = SVC_STATE_STOPPED;
  ls_test_descriptors[1].heartbeatIntervalMs = 1000;
  ls_test_descriptors[1].missedHeartbeats = 1;

  ls_test_descriptors[2].threadId = (k_tid_t)0x3000;
  ls_test_descriptors[2].priority = SVC_PRIORITY_APPLICATION;
  ls_test_descriptors[2].state = SVC_STATE_SUSPENDED;
  ls_test_descriptors[2].heartbeatIntervalMs = 2000;
  ls_test_descriptors[2].missedHeartbeats = 2;
}

/**
 * @test The execLs function must print only the header when no services are registered.
 */
ZTEST(serviceMngrCmd, test_ls_emptyRegistry)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"ls"};
  int result;

  /* serviceMngrUtilGetRegEntryByIndex returns NULL by default (empty registry) */

  result = execLs(shell, 1, argv);

  zassert_equal(result, 0,
                "execLs should return 0");
  zassert_equal(serviceMngrUtilGetRegEntryByIndex_fake.call_count, 1,
                "serviceMngrUtilGetRegEntryByIndex should be called once (index 0 returns NULL)");
  zassert_equal(k_thread_name_get_mock_fake.call_count, 0,
                "k_thread_name_get should not be called when no services registered");
  zassert_equal(shell_print_call_count, 1,
                "shell_print should be called once for the header");
  zassert_true(strstr(captured_shell_outputs[0], "Index") != NULL,
               "header should contain 'Index'");
  zassert_true(strstr(captured_shell_outputs[0], "Name") != NULL,
               "header should contain 'Name'");
  zassert_true(strstr(captured_shell_outputs[0], "State") != NULL,
               "header should contain 'State'");
}

/**
 * @test The execLs function must print all registered services.
 */
ZTEST(serviceMngrCmd, test_ls_withServices)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"ls"};
  int result;

  serviceMngrUtilGetRegEntryByIndex_fake.custom_fake = getRegEntry_withServices;
  k_thread_name_get_mock_fake.return_val = "test_service";

  result = execLs(shell, 1, argv);

  zassert_equal(result, 0,
                "execLs should return 0");
  zassert_equal(serviceMngrUtilGetRegEntryByIndex_fake.call_count, 4,
                "serviceMngrUtilGetRegEntryByIndex should be called 4 times (0-2 return descriptors, 3 returns NULL)");
  zassert_equal(k_thread_name_get_mock_fake.call_count, 3,
                "k_thread_name_get should be called once per registered service");
  zassert_equal(k_thread_name_get_mock_fake.arg0_val, (k_tid_t)0x3000,
                "k_thread_name_get last call should use last service thread ID");
  zassert_equal(shell_print_call_count, 4,
                "shell_print should be called 4 times (1 header + 3 services)");
  zassert_true(strstr(captured_shell_outputs[1], "critical") != NULL,
               "first service line should contain priority 'critical'");
  zassert_true(strstr(captured_shell_outputs[1], "running") != NULL,
               "first service line should contain state 'running'");
  zassert_true(strstr(captured_shell_outputs[2], "core") != NULL,
               "second service line should contain priority 'core'");
  zassert_true(strstr(captured_shell_outputs[2], "stopped") != NULL,
               "second service line should contain state 'stopped'");
  zassert_true(strstr(captured_shell_outputs[3], "application") != NULL,
               "third service line should contain priority 'application'");
  zassert_true(strstr(captured_shell_outputs[3], "suspended") != NULL,
               "third service line should contain state 'suspended'");
}

/* Custom fake for shell_strtoul that sets the error parameter */
static unsigned long shell_strtoul_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  *err = -EINVAL;
  return 0;
}

/**
 * @test The execStart function must return error when the index argument is invalid.
 */
ZTEST(serviceMngrCmd, test_start_invalidIndexArg)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"start", "invalid"};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execStart(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execStart should return -EINVAL on invalid index argument");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.call_count, 0,
                "serviceMngrUtilStartService should not be called");
}

/**
 * @test The execStart function must return error when the utility function fails.
 */
ZTEST(serviceMngrCmd, test_start_utilFails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"start", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilStartService_fake.return_val = -EINVAL;

  result = execStart(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execStart should return error from serviceMngrUtilStartService");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.call_count, 1,
                "serviceMngrUtilStartService should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_val, 2,
                "serviceMngrUtilStartService should be called with index 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
}

/**
 * @test The execStart function must print a success message when the service is started.
 */
ZTEST(serviceMngrCmd, test_start_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"start", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilStartService_fake.return_val = 0;

  result = execStart(shell, 2, argv);

  zassert_equal(result, 0,
                "execStart should return 0 on success");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(serviceMngrUtilStartService_fake.call_count, 1,
                "serviceMngrUtilStartService should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_val, 2,
                "serviceMngrUtilStartService should be called with index 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called on success");
  zassert_true(strstr(captured_shell_outputs[0], "SUCCESS") != NULL,
               "output should contain 'SUCCESS'");
  zassert_true(strstr(captured_shell_outputs[0], "2") != NULL,
               "output should contain the service index");
}

/**
 * @test The execStop function must return error when the index argument is invalid.
 */
ZTEST(serviceMngrCmd, test_stop_invalidIndexArg)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"stop", "invalid"};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execStop(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execStop should return -EINVAL on invalid index argument");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.call_count, 0,
                "serviceMngrUtilStopService should not be called");
}

/**
 * @test The execStop function must return error when the utility function fails.
 */
ZTEST(serviceMngrCmd, test_stop_utilFails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"stop", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilStopService_fake.return_val = -EINVAL;

  result = execStop(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execStop should return error from serviceMngrUtilStopService");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.call_count, 1,
                "serviceMngrUtilStopService should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.arg0_val, 2,
                "serviceMngrUtilStopService should be called with index 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
}

/**
 * @test The execStop function must print a success message when the service is stopped.
 */
ZTEST(serviceMngrCmd, test_stop_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"stop", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilStopService_fake.return_val = 0;

  result = execStop(shell, 2, argv);

  zassert_equal(result, 0,
                "execStop should return 0 on success");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(serviceMngrUtilStopService_fake.call_count, 1,
                "serviceMngrUtilStopService should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.arg0_val, 2,
                "serviceMngrUtilStopService should be called with index 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called on success");
  zassert_true(strstr(captured_shell_outputs[0], "SUCCESS") != NULL,
               "output should contain 'SUCCESS'");
  zassert_true(strstr(captured_shell_outputs[0], "2") != NULL,
               "output should contain the service index");
}

/**
 * @test The execSuspend function must return error when the index argument is invalid.
 */
ZTEST(serviceMngrCmd, test_suspend_invalidIndexArg)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"suspend", "invalid"};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execSuspend(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execSuspend should return -EINVAL on invalid index argument");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 0,
                "serviceMngrUtilSuspendService should not be called");
}

/**
 * @test The execSuspend function must return error when the utility function fails.
 */
ZTEST(serviceMngrCmd, test_suspend_utilFails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"suspend", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilSuspendService_fake.return_val = -EINVAL;

  result = execSuspend(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execSuspend should return error from serviceMngrUtilSuspendService");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 1,
                "serviceMngrUtilSuspendService should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.arg0_val, 2,
                "serviceMngrUtilSuspendService should be called with index 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
}

/**
 * @test The execSuspend function must print a success message when the service is suspended.
 */
ZTEST(serviceMngrCmd, test_suspend_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"suspend", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilSuspendService_fake.return_val = 0;

  result = execSuspend(shell, 2, argv);

  zassert_equal(result, 0,
                "execSuspend should return 0 on success");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 1,
                "serviceMngrUtilSuspendService should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.arg0_val, 2,
                "serviceMngrUtilSuspendService should be called with index 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called on success");
  zassert_true(strstr(captured_shell_outputs[0], "SUCCESS") != NULL,
               "output should contain 'SUCCESS'");
  zassert_true(strstr(captured_shell_outputs[0], "2") != NULL,
               "output should contain the service index");
}

/**
 * @test The execResume function must return error when the index argument is invalid.
 */
ZTEST(serviceMngrCmd, test_resume_invalidIndexArg)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"resume", "invalid"};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execResume(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execResume should return -EINVAL on invalid index argument");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 0,
                "serviceMngrUtilResumeService should not be called");
}

/**
 * @test The execResume function must return error when the utility function fails.
 */
ZTEST(serviceMngrCmd, test_resume_utilFails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"resume", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilResumeService_fake.return_val = -EINVAL;

  result = execResume(shell, 2, argv);

  zassert_equal(result, -EINVAL,
                "execResume should return error from serviceMngrUtilResumeService");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 1,
                "serviceMngrUtilResumeService should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.arg0_val, 2,
                "serviceMngrUtilResumeService should be called with index 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
}

/**
 * @test The execResume function must print a success message when the service is resumed.
 */
ZTEST(serviceMngrCmd, test_resume_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"resume", "2"};
  int result;

  shell_strtoul_fake.return_val = 2;
  serviceMngrUtilResumeService_fake.return_val = 0;

  result = execResume(shell, 2, argv);

  zassert_equal(result, 0,
                "execResume should return 0 on success");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_strtoul_fake.arg0_val, argv[1],
                "shell_strtoul should be called with the index argument");
  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 1,
                "serviceMngrUtilResumeService should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.arg0_val, 2,
                "serviceMngrUtilResumeService should be called with index 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called on success");
  zassert_true(strstr(captured_shell_outputs[0], "SUCCESS") != NULL,
               "output should contain 'SUCCESS'");
  zassert_true(strstr(captured_shell_outputs[0], "2") != NULL,
               "output should contain the service index");
}

ZTEST_SUITE(serviceMngrCmd, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);

import collections
import subprocess
import threading
import re
import sys

_TEST_START_RE = re.compile(b'^\[BEGIN TEST "(.*)"\]$')
_TEST_END_RE = re.compile(b'^\[END TEST\]$')
_TEST_FAIL_RE = re.compile(b'^\[FAIL\]$')
_FINISH_RE = re.compile(b'^\[ALL TESTS RUN\]$')

class LogProcessor:
    def __init__(self):
        self.state = 'NONE'
        self.error = None
        self.test_ok = True
        self.failed_tests = []
        self.successed_tests = []
        self.running_test = None

    def is_finished(self):
        return self.state == 'FINISHED' and not self.error

    def process(self, line):
        match_start = _TEST_START_RE.match(line)
        if match_start:
            name = match_start.group(1)
            if self.state != 'NONE':
                self.error = 'Test begin at the wrong moment: %s' % name
                return False
            if name in self.successed_tests or name in self.failed_tests:
                self.error = 'Same test name used more than once: %s' % name
                return False
            self.running_test = name
            self.state = 'TESTING'
            self.test_ok = True
            return True
        match_end = _TEST_END_RE.match(line)
        if match_end:
            if self.state != 'TESTING':
                self.error = 'Test end at the wrong moment'
                return False
            if self.test_ok:
                self.successed_tests.append(self.running_test)
            else:
                self.failed_tests.append(self.running_test)
            self.running_test = None
            self.state = 'NONE'
            return True
        match_fail = _TEST_FAIL_RE.match(line)
        if match_fail:
            if self.state != 'TESTING':
                self.error = 'Test fail at the wrong moment'
                return False
            self.test_ok = False
            return True
        match_finish = _FINISH_RE.match(line)
        if match_finish:
            if self.state != 'NONE':
                self.error = 'End of tests at the wrong moment'
                return False
            self.state = 'FINISHED'
            return True
        return True

def run_and_process(command):
    with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL) as proc:
        try:
            stdout, stderr = proc.communicate(input=b'c\nq\n', timeout=20)
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout, stderr = proc.communicate()

        if proc.returncode != 0:
            print('Finished with error')
            log_processor.error = 'Finished with error'

        stdout = stdout.split(b'\n')

        f = open('/tmp/out', 'wb')

        log_processor = LogProcessor()
        for line in stdout:
            if not log_processor.process(line):
                print('Error while reading logs: %s' % log_processor.error)
                return log_processor

        return log_processor

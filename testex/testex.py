import collections
import subprocess
import threading
import re
import sys

_TEST_START_RE = re.compile(b'^\[BEGIN TEST "(.*)"\]$')
_TEST_END_RE = re.compile(b'^\[END TEST "(.*)" (FAIL|OK)\]$')
_FINISH_RE = re.compile(b'^\[ALL TESTS RUN\]$')

class LogProcessor:
    def __init__(self):
        self.error = None
        self.failed_tests = []
        self.successed_tests = []
        self.running_tests = []
        self.finished = False

    def is_finished(self):
        return self.finished and not self.running_tests and not self.error

    def process(self, line):
        match_start = _TEST_START_RE.match(line)
        if match_start:
            if self.finished:
                self.error = 'Test logs after finish'
                return False
            name = match_start.group(1)
            if name in self.successed_tests or name in self.failed_tests or name in self.running_tests:
                self.error = 'Same test name used more than once: %s' % name
                return False
            self.running_tests.append(name)
            return True
        match_end = _TEST_END_RE.match(line)
        if match_end:
            if self.finished:
                self.error = 'Test logs after finish'
                return False
            name = match_end.group(1)
            if name not in self.running_tests:
                self.error = 'Test has finished without starting: %s' % name
                return False
            self.running_tests.remove(name)
            status = match_end.group(2)
            if status == b'OK':
                self.successed_tests.append(name)
            else:
                self.failed_tests.append(name)
            return True
        match_finish = _FINISH_RE.match(line)
        if match_finish:
            if self.finished:
                self.error = 'Finished twice'
                return False
            if self.running_tests:
                self.error = 'Finished while tests where running'
                return False
            self.finished = True
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

        stdout = stdout.split(b'\n')

        f = open('/tmp/out', 'wb')

        log_processor = LogProcessor()
        for line in stdout:
            if not log_processor.process(line):
                print('Error while reading logs: %s' % log_processor.error)
                return log_processor

        return log_processor

#!/usr/bin/python3

import sys
from testex import *

def main():
    log_processor = run_and_process(sys.argv[1:])
    if log_processor.error:
        print('Error: %s' % log_processor.error)
        sys.exit(1)

    print('Tests ran: %s' %
            (len(log_processor.failed_tests)
                + len(log_processor.successed_tests)))

    print('Failing tests: %s' % len(log_processor.failed_tests))

    for failing in log_processor.failed_tests:
        print('Failed test %s' % failing)

    if not log_processor.is_finished():
        print('Unexpected end of logs')
        sys.exit(1)

    if len(log_processor.failed_tests) != 0:
        sys.exit(1)

if __name__=="__main__":
    main()
